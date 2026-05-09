#ifndef ROARING_NODE_ROARINGBITMAP64_ASYNC_WORKERS_
#define ROARING_NODE_ROARINGBITMAP64_ASYNC_WORKERS_

#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#ifdef _WIN32
#  include <io.h>
#  include <windows.h>
#else
#  include <fcntl.h>
#  include <sys/stat.h>
#  include <unistd.h>
#endif

#include "RoaringBitmap64.h"
#include "RoaringBitmap64-bulk.h"
#include "async-workers.h"
#include "bigint-utils.h"
#include "memory.h"
#include "serialization-format.h"

namespace rb64_async_io {

// Configurable upper bound on deserializable file size. Default 16 GiB —
// generous enough for any roaring64 portable file we expect in practice but
// small enough that an adversarial path cannot trivially balloon RSS before
// the parser even runs. Callers with multi-TiB datasets can raise the cap
// or disable it (set to 0). Read once per process via a static lambda; the
// env var is sampled on first call and cached.
inline size_t getMaxDeserializeBytes() {
  static const size_t cached = []() -> size_t {
    const char * env = std::getenv("ROARING_NODE_MAX_DESERIALIZE_BYTES");
    if (env != nullptr && *env != '\0') {
      char * end = nullptr;
      unsigned long long v = std::strtoull(env, &end, 10);
      if (end != env && *end == '\0') {
        return static_cast<size_t>(v);  // 0 disables the cap entirely
      }
    }
    return static_cast<size_t>(16) << 30;  // 16 GiB default
  }();
  return cached;
}

// Read entire file into a heap buffer. On failure, returns nullptr and writes
// an error to *outError. Caller owns the returned buffer (gcaware_free).
inline char * readFileFully(const char * path, size_t * outLen, WorkerError * outError) {
  FILE * f = std::fopen(path, "rb");
  if (!f) {
    *outError = WorkerError("RoaringBitmap64.deserializeFileAsync: cannot open file");
    return nullptr;
  }
  if (std::fseek(f, 0, SEEK_END) != 0) {
    std::fclose(f);
    *outError = WorkerError("RoaringBitmap64.deserializeFileAsync: fseek failed");
    return nullptr;
  }
  long sz = std::ftell(f);
  if (sz < 0) {
    std::fclose(f);
    *outError = WorkerError("RoaringBitmap64.deserializeFileAsync: ftell failed");
    return nullptr;
  }
  const size_t cap = getMaxDeserializeBytes();
  if (cap != 0 && static_cast<size_t>(sz) > cap) {
    std::fclose(f);
    *outError = WorkerError(
      "RoaringBitmap64.deserializeFileAsync: file exceeds "
      "ROARING_NODE_MAX_DESERIALIZE_BYTES (set the env var to a higher value, "
      "or to 0 to disable the cap)");
    return nullptr;
  }
  std::rewind(f);
  size_t size = (size_t)sz;
  char * buf = (size == 0) ? static_cast<char *>(gcaware_malloc(1))
                           : static_cast<char *>(gcaware_malloc(size));
  if (!buf) {
    std::fclose(f);
    *outError = WorkerError("RoaringBitmap64.deserializeFileAsync: alloc failed");
    return nullptr;
  }
  if (size > 0) {
    size_t r = std::fread(buf, 1, size, f);
    std::fclose(f);
    if (r != size) {
      gcaware_free(buf);
      *outError = WorkerError("RoaringBitmap64.deserializeFileAsync: fread short");
      return nullptr;
    }
  } else {
    std::fclose(f);
  }
  *outLen = size;
  return buf;
}

inline bool writeFileFully(const char * path, const char * data, size_t len, WorkerError * outError) {
  // Durable atomic write. The contract:
  //   1. Unique tmp path per call (pid + atomic counter) so concurrent writers
  //      to the same destination do not collide and a hostile pre-existing
  //      `<path>.tmp.<known>` symlink cannot redirect us.
  //   2. Open tmp with O_EXCL+O_NOFOLLOW (POSIX) / CREATE_NEW (Windows) so we
  //      refuse to follow a symlink or overwrite an existing file there.
  //   3. fsync (POSIX) / _commit (Windows) before rename so the bytes hit
  //      stable storage; without this an OS crash between rename and writeback
  //      can leave the destination zero-filled.
  //   4. Atomic rename (rename / MoveFileExA with MOVEFILE_REPLACE_EXISTING).
  //   5. On any failure the tmp is removed so the caller never sees a stub.
  static std::atomic<uint64_t> tmpCounter{0};
  std::string tmpPath;
  tmpPath.reserve(std::strlen(path) + 32);
  tmpPath.assign(path);
  tmpPath.append(".tmp.");
#ifdef _WIN32
  tmpPath.append(std::to_string(static_cast<uint64_t>(::GetCurrentProcessId())));
#else
  tmpPath.append(std::to_string(static_cast<uint64_t>(::getpid())));
#endif
  tmpPath.push_back('.');
  tmpPath.append(std::to_string(tmpCounter.fetch_add(1, std::memory_order_relaxed)));

  FILE * f = nullptr;
#ifdef _WIN32
  HANDLE h = ::CreateFileA(
    tmpPath.c_str(),
    GENERIC_WRITE,
    0,                  // no sharing
    nullptr,
    CREATE_NEW,         // refuse to overwrite (defense vs predicted-path attacks)
    FILE_ATTRIBUTE_NORMAL,
    nullptr);
  if (h == INVALID_HANDLE_VALUE) {
    *outError = WorkerError("RoaringBitmap64.serializeFileAsync: cannot create tmp file");
    return false;
  }
  int fd = ::_open_osfhandle(reinterpret_cast<intptr_t>(h), _O_WRONLY | _O_BINARY);
  if (fd < 0) {
    ::CloseHandle(h);
    ::DeleteFileA(tmpPath.c_str());
    *outError = WorkerError("RoaringBitmap64.serializeFileAsync: _open_osfhandle failed");
    return false;
  }
  f = ::_fdopen(fd, "wb");
  if (!f) {
    ::_close(fd);  // also closes the underlying HANDLE
    ::DeleteFileA(tmpPath.c_str());
    *outError = WorkerError("RoaringBitmap64.serializeFileAsync: _fdopen failed");
    return false;
  }
#else
  // O_NOFOLLOW: open(2) returns ELOOP if tmpPath is a symlink — the only
  //   portable way; fopen("wbx") does not give this guarantee on macOS BSD.
  // O_EXCL+O_CREAT: refuse to open if the path already exists.
  // O_CLOEXEC: don't leak the fd to forked children.
  // Mode 0600: serialized bitmaps may carry confidential data — owner-only.
  int fd = ::open(
    tmpPath.c_str(),
    O_WRONLY | O_CREAT | O_EXCL | O_NOFOLLOW | O_CLOEXEC,
    0600);
  if (fd < 0) {
    *outError = WorkerError("RoaringBitmap64.serializeFileAsync: cannot create tmp file");
    return false;
  }
  f = ::fdopen(fd, "wb");
  if (!f) {
    ::close(fd);
    ::unlink(tmpPath.c_str());
    *outError = WorkerError("RoaringBitmap64.serializeFileAsync: fdopen failed");
    return false;
  }
#endif

  if (len > 0) {
    size_t w = std::fwrite(data, 1, len, f);
    if (w != len) {
      std::fclose(f);
      std::remove(tmpPath.c_str());
      *outError = WorkerError("RoaringBitmap64.serializeFileAsync: fwrite short");
      return false;
    }
  }

  if (std::fflush(f) != 0) {
    std::fclose(f);
    std::remove(tmpPath.c_str());
    *outError = WorkerError("RoaringBitmap64.serializeFileAsync: fflush failed");
    return false;
  }

#ifdef _WIN32
  if (::_commit(::_fileno(f)) != 0) {
    std::fclose(f);
    std::remove(tmpPath.c_str());
    *outError = WorkerError("RoaringBitmap64.serializeFileAsync: _commit failed");
    return false;
  }
#else
  if (::fsync(::fileno(f)) != 0) {
    std::fclose(f);
    std::remove(tmpPath.c_str());
    *outError = WorkerError("RoaringBitmap64.serializeFileAsync: fsync failed");
    return false;
  }
#endif

  if (std::fclose(f) != 0) {
    std::remove(tmpPath.c_str());
    *outError = WorkerError("RoaringBitmap64.serializeFileAsync: fclose failed");
    return false;
  }

#ifdef _WIN32
  // std::rename on Windows fails if dest exists; MoveFileExA replaces atomically.
  if (!::MoveFileExA(tmpPath.c_str(), path,
                     MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
    std::remove(tmpPath.c_str());
    *outError = WorkerError("RoaringBitmap64.serializeFileAsync: MoveFileExA failed");
    return false;
  }
#else
  if (std::rename(tmpPath.c_str(), path) != 0) {
    std::remove(tmpPath.c_str());
    *outError = WorkerError("RoaringBitmap64.serializeFileAsync: rename failed");
    return false;
  }
#endif
  return true;
}

}  // namespace rb64_async_io

class RB64SerializeFileWorker final : public AsyncWorker {
 public:
  v8::Global<v8::Object> bitmapPersistent;
  std::string filePath;
  // Owned clone of the source bitmap. Cloning on the main thread inside the
  // ctor (while the source is guaranteed alive) lets the worker thread read
  // its own copy even if the user calls dispose() before the worker resolves.
  roaring64_bitmap_t * ownedBitmap;
  // Heap-owned snapshot allocated and filled in work(); freed in the dtor.
  char * snapshot;
  size_t snapshotLen;
  bool wantFrozen;

  explicit RB64SerializeFileWorker(
    const v8::FunctionCallbackInfo<v8::Value> & infoArg, AddonData * addonDataArg) :
    AsyncWorker(infoArg.GetIsolate(), addonDataArg),
    ownedBitmap(nullptr),
    snapshot(nullptr),
    snapshotLen(0),
    wantFrozen(false) {
    _gcaware_adjustAllocatedMemory(this->isolate, sizeof(RB64SerializeFileWorker));

    // All input parsing happens on the main thread. Heavy serialization is
    // deferred to work() so the event loop is not blocked.
    v8::Isolate * iso = this->isolate;
    if (infoArg.Length() < 1 || !infoArg[0]->IsString()) {
      this->setError(WorkerError("RoaringBitmap64.serializeFileAsync: filePath must be a string"));
      return;
    }
    v8::String::Utf8Value pathUtf(iso, infoArg[0]);
    if (!*pathUtf) {
      this->setError(WorkerError("RoaringBitmap64.serializeFileAsync: invalid filePath"));
      return;
    }
    this->filePath.assign(*pathUtf, pathUtf.length());

    if (infoArg.Length() >= 2 && !infoArg[1]->IsUndefined()) {
      SerializationFormat fmt = tryParseSerializationFormat(infoArg[1], iso);
      if (fmt == SerializationFormat::portable) {
        this->wantFrozen = false;
      } else if (fmt == SerializationFormat::unsafe_frozen_croaring) {
        this->wantFrozen = true;
      } else {
        this->setError(WorkerError(
          "RoaringBitmap64.serializeFileAsync: format must be 'portable' or 'unsafe_frozen_croaring'"));
        return;
      }
    }

    RoaringBitmap64 * self = ObjectWrap::TryUnwrap<RoaringBitmap64>(infoArg.This(), iso);
    if (self == nullptr || self->disposed) {
      this->setError(WorkerError("RoaringBitmap64 is disposed"));
      return;
    }
    if (this->maybeAddonData == nullptr) this->maybeAddonData = self->addonData;
    this->bitmapPersistent.Reset(iso, infoArg.This());

    if (this->wantFrozen && self->isFrozenHard()) {
      this->setError(WorkerError(
        "RoaringBitmap64.serializeFileAsync(frozen) cannot operate on a frozen view"));
      return;
    }
    // shrink_to_fit must run on the main thread because it can re-layout
    // container backing stores; defer the heavier serialization to work().
    // Bump _version so any in-flight iterator over self sees the layout
    // change and reports invalidation rather than producing stale values.
    if (this->wantFrozen) {
      roaring64_bitmap_shrink_to_fit(self->bitmap);
      self->invalidate();
    }
    this->ownedBitmap = roaring64_bitmap_copy(self->bitmap);
    if (this->ownedBitmap == nullptr) {
      this->setError(WorkerError("RoaringBitmap64.serializeFileAsync: clone failed"));
      return;
    }
  }

  ~RB64SerializeFileWorker() override {
    if (snapshot) gcaware_free(snapshot);
    if (ownedBitmap) roaring64_bitmap_free(ownedBitmap);
    _gcaware_adjustAllocatedMemory(this->isolate, -sizeof(RB64SerializeFileWorker));
  }

 protected:
  void before() final {
    // All parsing happened in the ctor.
  }

  void work() final {
    if (this->hasError()) return;
    if (this->ownedBitmap == nullptr) {
      return this->setError(WorkerError("RoaringBitmap64.serializeFileAsync: null bitmap"));
    }

    if (this->wantFrozen) {
      size_t size = roaring64_bitmap_frozen_size_in_bytes(this->ownedBitmap);
      this->snapshot = static_cast<char *>(gcaware_malloc(size == 0 ? 1 : size));
      if (!this->snapshot) {
        return this->setError(WorkerError("RoaringBitmap64.serializeFileAsync: alloc failed"));
      }
      if (size > 0) {
        size_t w = roaring64_bitmap_frozen_serialize(this->ownedBitmap, this->snapshot);
        if (w != size) {
          return this->setError(WorkerError("RoaringBitmap64.serializeFileAsync: frozen size mismatch"));
        }
      }
      this->snapshotLen = size;
    } else {
      size_t size = roaring64_bitmap_portable_size_in_bytes(this->ownedBitmap);
      this->snapshot = static_cast<char *>(gcaware_malloc(size == 0 ? 1 : size));
      if (!this->snapshot) {
        return this->setError(WorkerError("RoaringBitmap64.serializeFileAsync: alloc failed"));
      }
      if (size > 0) {
        size_t w = roaring64_bitmap_portable_serialize(this->ownedBitmap, this->snapshot);
        if (w != size) {
          return this->setError(WorkerError("RoaringBitmap64.serializeFileAsync: portable size mismatch"));
        }
      }
      this->snapshotLen = size;
    }

    WorkerError err;
    if (!rb64_async_io::writeFileFully(
          this->filePath.c_str(), this->snapshot, this->snapshotLen, &err)) {
      this->setError(err);
    }
  }

  void done(v8::Local<v8::Value> & result) final {
    if (!this->bitmapPersistent.IsEmpty()) {
      result = this->bitmapPersistent.Get(this->isolate);
    } else {
      result = v8::Undefined(this->isolate).As<v8::Value>();
    }
  }
};

class RB64DeserializeFileWorker final : public AsyncWorker {
 public:
  std::string filePath;
  char * fileBuf;
  size_t fileLen;
  // Worker-thread-produced result; consumed by done() on the main thread.
  std::atomic<roaring64_bitmap_t *> resultBitmap;

  explicit RB64DeserializeFileWorker(
    const v8::FunctionCallbackInfo<v8::Value> & infoArg, AddonData * addonDataArg) :
    AsyncWorker(infoArg.GetIsolate(), addonDataArg),
    fileBuf(nullptr),
    fileLen(0),
    resultBitmap(nullptr) {
    _gcaware_adjustAllocatedMemory(this->isolate, sizeof(RB64DeserializeFileWorker));

    // Parse and validate inputs synchronously on the main thread; never stash
    // a reference to `infoArg` for later use.
    v8::Isolate * iso = this->isolate;
    if (infoArg.Length() < 1 || !infoArg[0]->IsString()) {
      this->setError(WorkerError("RoaringBitmap64.deserializeFileAsync: filePath must be a string"));
      return;
    }
    v8::String::Utf8Value pathUtf(iso, infoArg[0]);
    if (!*pathUtf) {
      this->setError(WorkerError("RoaringBitmap64.deserializeFileAsync: invalid filePath"));
      return;
    }
    this->filePath.assign(*pathUtf, pathUtf.length());
    if (infoArg.Length() >= 2 && !infoArg[1]->IsUndefined()) {
      DeserializationFormat fmt = tryParseDeserializationFormat(infoArg[1], iso);
      if (fmt != DeserializationFormat::portable) {
        this->setError(WorkerError(
          "RoaringBitmap64.deserializeFileAsync: only 'portable' is supported"));
      return;
      }
    }
  }

  ~RB64DeserializeFileWorker() override {
    if (this->fileBuf) gcaware_free(this->fileBuf);
    roaring64_bitmap_t * b = this->resultBitmap.exchange(nullptr, std::memory_order_acq_rel);
    if (b) roaring64_bitmap_free(b);
    _gcaware_adjustAllocatedMemory(this->isolate, -sizeof(RB64DeserializeFileWorker));
  }

 protected:
  void before() final {
    // All parsing happened in the ctor.
  }

  void work() final {
    if (this->hasError()) return;
    WorkerError err;
    this->fileBuf = rb64_async_io::readFileFully(this->filePath.c_str(), &this->fileLen, &err);
    if (err.hasError()) return this->setError(err);

    roaring64_bitmap_t * r =
      roaring64_bitmap_portable_deserialize_safe(this->fileBuf, this->fileLen);
    if (r == nullptr) {
      return this->setError(
        WorkerError("RoaringBitmap64.deserializeFileAsync: invalid roaring buffer"));
    }
    this->resultBitmap.store(r, std::memory_order_release);
  }

  void done(v8::Local<v8::Value> & out) final {
    v8::Isolate * iso = this->isolate;
    AddonData * ad = this->maybeAddonData;
    if (ad == nullptr) {
      this->setError(WorkerError("Addon data unavailable"));
      return;
    }
    v8::Local<v8::Function> cons = ad->RoaringBitmap64_constructor.Get(iso);
    v8::Local<v8::Object> newObj;
    v8::Local<v8::Value> argv[1] = {v8::Undefined(iso)};
    if (!cons->NewInstance(iso->GetCurrentContext(), 1, argv).ToLocal(&newObj)) {
      this->setError(WorkerError("Failed to instantiate RoaringBitmap64"));
      return;
    }
    RoaringBitmap64 * inst = ObjectWrap::TryUnwrap<RoaringBitmap64>(newObj, iso);
    if (inst == nullptr) {
      this->setError(WorkerError(ERROR_INVALID_OBJECT));
      return;
    }
    roaring64_bitmap_t * r = this->resultBitmap.exchange(nullptr, std::memory_order_acq_rel);
    if (r == nullptr) {
      this->setError(WorkerError("Deserialization produced no bitmap"));
      return;
    }
    if (inst->bitmap) roaring64_bitmap_free(inst->bitmap);
    inst->bitmap = r;
    inst->invalidate();
    out = newObj;
  }
};

// Free callback used by node::Buffer::New when transferring heap ownership of
// a gcaware_malloc-allocated buffer to V8.
inline void rb64_async_buffer_free(char * data, void * /*hint*/) {
  if (data) gcaware_free(data);
}

class RB64SerializeAsyncWorker final : public AsyncWorker {
 public:
  v8::Global<v8::Object> bitmapPersistent;
  // Owned clone of the source bitmap. Cloned on the main thread inside the
  // ctor (where the source is guaranteed alive); the worker thread reads its
  // own copy so that dispose() racing the worker cannot trigger a UAF.
  roaring64_bitmap_t * ownedBitmap;
  // Heap-owned snapshot allocated and filled in work(); done() transfers
  // ownership to a node::Buffer via rb64_async_buffer_free.
  char * snapshot;
  size_t snapshotLen;
  bool wantFrozen;
  bool snapshotOwned;

  explicit RB64SerializeAsyncWorker(
    const v8::FunctionCallbackInfo<v8::Value> & infoArg, AddonData * addonDataArg) :
    AsyncWorker(infoArg.GetIsolate(), addonDataArg),
    ownedBitmap(nullptr),
    snapshot(nullptr),
    snapshotLen(0),
    wantFrozen(false),
    snapshotOwned(true) {
    _gcaware_adjustAllocatedMemory(this->isolate, sizeof(RB64SerializeAsyncWorker));

    v8::Isolate * iso = this->isolate;
    if (infoArg.Length() >= 1 && !infoArg[0]->IsUndefined() && !infoArg[0]->IsFunction()) {
      SerializationFormat fmt = tryParseSerializationFormat(infoArg[0], iso);
      if (fmt == SerializationFormat::portable) {
        this->wantFrozen = false;
      } else if (fmt == SerializationFormat::unsafe_frozen_croaring) {
        this->wantFrozen = true;
      } else {
        this->setError(WorkerError(
          "RoaringBitmap64.serializeAsync: format must be 'portable' or 'unsafe_frozen_croaring'"));
        return;
      }
    }

    RoaringBitmap64 * self = ObjectWrap::TryUnwrap<RoaringBitmap64>(infoArg.This(), iso);
    if (self == nullptr || self->disposed) {
      this->setError(WorkerError("RoaringBitmap64 is disposed"));
      return;
    }
    if (this->maybeAddonData == nullptr) this->maybeAddonData = self->addonData;
    this->bitmapPersistent.Reset(iso, infoArg.This());

    if (this->wantFrozen && self->isFrozenHard()) {
      this->setError(WorkerError(
        "RoaringBitmap64.serializeAsync(frozen) cannot operate on a frozen view"));
      return;
    }
    // shrink_to_fit must run on the main thread because it can re-layout
    // container backing stores; defer the heavier serialization to work().
    // Bump _version so any in-flight iterator over self sees the layout
    // change and reports invalidation rather than producing stale values.
    if (this->wantFrozen) {
      roaring64_bitmap_shrink_to_fit(self->bitmap);
      self->invalidate();
    }
    this->ownedBitmap = roaring64_bitmap_copy(self->bitmap);
    if (this->ownedBitmap == nullptr) {
      this->setError(WorkerError("RoaringBitmap64.serializeAsync: clone failed"));
      return;
    }
  }

  ~RB64SerializeAsyncWorker() override {
    if (snapshotOwned && snapshot) gcaware_free(snapshot);
    if (ownedBitmap) roaring64_bitmap_free(ownedBitmap);
    _gcaware_adjustAllocatedMemory(this->isolate, -sizeof(RB64SerializeAsyncWorker));
  }

 protected:
  void work() final {
    if (this->hasError()) return;
    if (this->ownedBitmap == nullptr) {
      return this->setError(WorkerError("RoaringBitmap64.serializeAsync: null bitmap"));
    }
    if (this->wantFrozen) {
      size_t size = roaring64_bitmap_frozen_size_in_bytes(this->ownedBitmap);
      this->snapshot = static_cast<char *>(gcaware_malloc(size == 0 ? 1 : size));
      if (!this->snapshot) {
        return this->setError(WorkerError("RoaringBitmap64.serializeAsync: alloc failed"));
      }
      if (size > 0) {
        size_t w = roaring64_bitmap_frozen_serialize(this->ownedBitmap, this->snapshot);
        if (w != size) {
          return this->setError(WorkerError("RoaringBitmap64.serializeAsync: frozen size mismatch"));
        }
      }
      this->snapshotLen = size;
    } else {
      size_t size = roaring64_bitmap_portable_size_in_bytes(this->ownedBitmap);
      this->snapshot = static_cast<char *>(gcaware_malloc(size == 0 ? 1 : size));
      if (!this->snapshot) {
        return this->setError(WorkerError("RoaringBitmap64.serializeAsync: alloc failed"));
      }
      if (size > 0) {
        size_t w = roaring64_bitmap_portable_serialize(this->ownedBitmap, this->snapshot);
        if (w != size) {
          return this->setError(WorkerError("RoaringBitmap64.serializeAsync: portable size mismatch"));
        }
      }
      this->snapshotLen = size;
    }
  }

  void done(v8::Local<v8::Value> & out) final {
    v8::Isolate * iso = this->isolate;
    auto bufMaybe = node::Buffer::New(
      iso, this->snapshot, this->snapshotLen, &rb64_async_buffer_free, nullptr);
    v8::Local<v8::Object> buf;
    if (!bufMaybe.ToLocal(&buf)) {
      this->setError(WorkerError("RoaringBitmap64.serializeAsync: Buffer alloc failed"));
      return;
    }
    // Buffer now owns snapshot — don't free again in dtor.
    this->snapshotOwned = false;
    out = buf;
  }
};

class RB64DeserializeBufferAsyncWorker final : public AsyncWorker {
 public:
  // Persistent on the source Buffer/ArrayBufferView so the underlying bytes
  // stay alive across the worker hop.
  v8::Global<v8::Value> bufferPersistent;
  // Borrowed pointer + length captured on the main thread. The persistent
  // above keeps the backing store alive.
  const uint8_t * dataPtr;
  size_t dataLen;
  std::atomic<roaring64_bitmap_t *> resultBitmap;

  explicit RB64DeserializeBufferAsyncWorker(
    const v8::FunctionCallbackInfo<v8::Value> & infoArg, AddonData * addonDataArg) :
    AsyncWorker(infoArg.GetIsolate(), addonDataArg),
    dataPtr(nullptr),
    dataLen(0),
    resultBitmap(nullptr) {
    _gcaware_adjustAllocatedMemory(this->isolate, sizeof(RB64DeserializeBufferAsyncWorker));

    v8::Isolate * iso = this->isolate;
    if (infoArg.Length() < 1) {
      this->setError(WorkerError("RoaringBitmap64.deserializeAsync expects a buffer argument"));
      return;
    }
    if (infoArg.Length() >= 2 && !infoArg[1]->IsUndefined() && !infoArg[1]->IsFunction()) {
      DeserializationFormat fmt = tryParseDeserializationFormat(infoArg[1], iso);
      if (fmt != DeserializationFormat::portable) {
        this->setError(WorkerError(
          "RoaringBitmap64.deserializeAsync: only 'portable' is supported"));
        return;
      }
    }
    if (!roaring_node_bigint::tryGetByteBuffer(iso, infoArg[0], &this->dataPtr, &this->dataLen)) {
      this->setError(WorkerError(
        "RoaringBitmap64.deserializeAsync: expected Buffer, Uint8Array, ArrayBuffer, or DataView"));
      return;
    }
    this->bufferPersistent.Reset(iso, infoArg[0]);
  }

  ~RB64DeserializeBufferAsyncWorker() override {
    roaring64_bitmap_t * b = this->resultBitmap.exchange(nullptr, std::memory_order_acq_rel);
    if (b) roaring64_bitmap_free(b);
    _gcaware_adjustAllocatedMemory(this->isolate, -sizeof(RB64DeserializeBufferAsyncWorker));
  }

 protected:
  void work() final {
    if (this->hasError()) return;
    roaring64_bitmap_t * r = roaring64_bitmap_portable_deserialize_safe(
      reinterpret_cast<const char *>(this->dataPtr), this->dataLen);
    if (r == nullptr) {
      return this->setError(
        WorkerError("RoaringBitmap64.deserializeAsync: invalid roaring buffer"));
    }
    this->resultBitmap.store(r, std::memory_order_release);
  }

  void done(v8::Local<v8::Value> & out) final {
    v8::Isolate * iso = this->isolate;
    AddonData * ad = this->maybeAddonData;
    if (ad == nullptr) {
      return this->setError(WorkerError("Addon data unavailable"));
    }
    v8::Local<v8::Function> cons = ad->RoaringBitmap64_constructor.Get(iso);
    v8::Local<v8::Object> newObj;
    v8::Local<v8::Value> argv[1] = {v8::Undefined(iso)};
    if (!cons->NewInstance(iso->GetCurrentContext(), 1, argv).ToLocal(&newObj)) {
      return this->setError(WorkerError("Failed to instantiate RoaringBitmap64"));
    }
    RoaringBitmap64 * inst = ObjectWrap::TryUnwrap<RoaringBitmap64>(newObj, iso);
    if (inst == nullptr) {
      return this->setError(WorkerError(ERROR_INVALID_OBJECT));
    }
    roaring64_bitmap_t * r = this->resultBitmap.exchange(nullptr, std::memory_order_acq_rel);
    if (r == nullptr) {
      return this->setError(WorkerError("Deserialization produced no bitmap"));
    }
    if (inst->bitmap) roaring64_bitmap_free(inst->bitmap);
    inst->bitmap = r;
    inst->invalidate();
    out = newObj;
  }
};

class RB64ToUint64ArrayWorker final : public AsyncWorker {
 public:
  v8::Global<v8::Object> bitmapPersistent;
  // Owned clone of the source bitmap. Cloning on the main thread inside the
  // ctor (where the source is guaranteed alive) lets the worker thread read
  // its own copy even if the user calls dispose() before the worker resolves.
  roaring64_bitmap_t * ownedBitmap;
  // Heap-owned values. work() allocates and fills; done() wraps and transfers.
  uint64_t * values;
  size_t valuesLen;
  bool valuesOwned;

  explicit RB64ToUint64ArrayWorker(
    const v8::FunctionCallbackInfo<v8::Value> & infoArg, AddonData * addonDataArg) :
    AsyncWorker(infoArg.GetIsolate(), addonDataArg),
    ownedBitmap(nullptr),
    values(nullptr),
    valuesLen(0),
    valuesOwned(true) {
    _gcaware_adjustAllocatedMemory(this->isolate, sizeof(RB64ToUint64ArrayWorker));

    v8::Isolate * iso = this->isolate;
    RoaringBitmap64 * self = ObjectWrap::TryUnwrap<RoaringBitmap64>(infoArg.This(), iso);
    if (self == nullptr || self->disposed) {
      this->setError(WorkerError("RoaringBitmap64 is disposed"));
      return;
    }
    if (this->maybeAddonData == nullptr) this->maybeAddonData = self->addonData;
    this->bitmapPersistent.Reset(iso, infoArg.This());
    this->ownedBitmap = roaring64_bitmap_copy(self->bitmap);
    if (this->ownedBitmap == nullptr) {
      this->setError(WorkerError("RoaringBitmap64.toUint64ArrayAsync: clone failed"));
      return;
    }
  }

  ~RB64ToUint64ArrayWorker() override {
    if (valuesOwned && values) gcaware_free(values);
    if (ownedBitmap) roaring64_bitmap_free(ownedBitmap);
    _gcaware_adjustAllocatedMemory(this->isolate, -sizeof(RB64ToUint64ArrayWorker));
  }

 protected:
  void work() final {
    if (this->hasError()) return;
    if (this->ownedBitmap == nullptr) return;
    uint64_t card = roaring64_bitmap_get_cardinality(this->ownedBitmap);
    if (card > (uint64_t)(SIZE_MAX / sizeof(uint64_t))) {
      return this->setError(WorkerError(
        "RoaringBitmap64.toUint64ArrayAsync: cardinality exceeds size_t limit"));
    }
    if (card == 0) {
      this->valuesLen = 0;
      return;
    }
    this->values = static_cast<uint64_t *>(gcaware_malloc((size_t)card * sizeof(uint64_t)));
    if (!this->values) {
      return this->setError(WorkerError("RoaringBitmap64.toUint64ArrayAsync: alloc failed"));
    }
    roaring64_bitmap_to_uint64_array(this->ownedBitmap, this->values);
    this->valuesLen = (size_t)card;
  }

  void done(v8::Local<v8::Value> & out) final {
    v8::Isolate * iso = this->isolate;
    if (this->valuesLen == 0) {
      auto ab = v8::ArrayBuffer::New(iso, 0);
      out = v8::BigUint64Array::New(ab, 0, 0);
      return;
    }
    size_t byteLen = this->valuesLen * sizeof(uint64_t);
    std::unique_ptr<v8::BackingStore> bs = v8::ArrayBuffer::NewBackingStore(
      this->values, byteLen,
      [](void * data, size_t /*length*/, void * /*deleter_data*/) {
        if (data) gcaware_free(data);
      },
      nullptr);
    auto ab = v8::ArrayBuffer::New(iso, std::move(bs));
    // Ownership transferred to the ArrayBuffer's BackingStore the moment we
    // construct it. Flip the flag now so a later failure (e.g. BigUint64Array
    // construction) doesn't trigger a double-free in the dtor.
    this->valuesOwned = false;
    auto ta = v8::BigUint64Array::New(ab, 0, this->valuesLen);
    out = ta;
  }
};

class RB64FromArrayAsyncWorker final : public AsyncWorker {
 public:
  uint64_t * values;
  size_t valuesLen;
  std::atomic<roaring64_bitmap_t *> resultBitmap;

  explicit RB64FromArrayAsyncWorker(v8::Isolate * iso, AddonData * addonData) :
    AsyncWorker(iso, addonData),
    values(nullptr),
    valuesLen(0),
    resultBitmap(nullptr) {
    _gcaware_adjustAllocatedMemory(this->isolate, sizeof(RB64FromArrayAsyncWorker));
  }

  ~RB64FromArrayAsyncWorker() override {
    if (this->values) gcaware_free(this->values);
    roaring64_bitmap_t * b = this->resultBitmap.exchange(nullptr, std::memory_order_acq_rel);
    if (b) roaring64_bitmap_free(b);
    _gcaware_adjustAllocatedMemory(this->isolate, -sizeof(RB64FromArrayAsyncWorker));
  }

  // Drains values on the main thread synchronously, before the worker is
  // queued. Returns false on V8 exception (caller's TryCatch picks it up)
  // or on a non-V8 setError condition.
  bool extractValues(v8::Local<v8::Value> arg) {
    v8::Isolate * iso = this->isolate;
    if (arg.IsEmpty() || arg->IsNullOrUndefined()) return true;

    const uint64_t * data = nullptr;
    size_t n = 0;
    if (roaring_node_bigint::tryGetBigUint64Array(arg, &data, &n)) {
      if (n == 0) return true;
      if (data == nullptr) {
        this->setError(WorkerError(
          "RoaringBitmap64.fromArrayAsync: BigUint64Array backing store is null (detached?)"));
        return true;
      }
      this->values = static_cast<uint64_t *>(gcaware_malloc(n * sizeof(uint64_t)));
      if (this->values == nullptr) {
        this->setError(WorkerError("RoaringBitmap64.fromArrayAsync: alloc failed"));
        return true;
      }
      std::memcpy(this->values, data, n * sizeof(uint64_t));
      this->valuesLen = n;
      return true;
    }

    std::vector<uint64_t> tmp;
    if (!RoaringBitmap64_bulk_internal::drainIterable(
          iso, arg, tmp, "fromArrayAsync value")) {
      return false;
    }
    if (tmp.empty()) return true;
    this->values = static_cast<uint64_t *>(gcaware_malloc(tmp.size() * sizeof(uint64_t)));
    if (this->values == nullptr) {
      this->setError(WorkerError("RoaringBitmap64.fromArrayAsync: alloc failed"));
      return true;
    }
    std::memcpy(this->values, tmp.data(), tmp.size() * sizeof(uint64_t));
    this->valuesLen = tmp.size();
    return true;
  }

 protected:
  void work() final {
    if (this->hasError()) return;
    roaring64_bitmap_t * r = roaring64_bitmap_create();
    if (r == nullptr) {
      this->setError(WorkerError("RoaringBitmap64.fromArrayAsync: create failed"));
      return;
    }
    if (this->valuesLen > 0 && this->values != nullptr) {
      roaring64_bitmap_add_many(r, this->valuesLen, this->values);
      roaring64_bitmap_run_optimize(r);
      roaring64_bitmap_shrink_to_fit(r);
    }
    this->resultBitmap.store(r, std::memory_order_release);
  }

  void done(v8::Local<v8::Value> & out) final {
    v8::Isolate * iso = this->isolate;
    AddonData * ad = this->maybeAddonData;
    if (ad == nullptr) {
      this->setError(WorkerError("Addon data unavailable"));
      return;
    }
    v8::Local<v8::Function> cons = ad->RoaringBitmap64_constructor.Get(iso);
    v8::Local<v8::Object> newObj;
    v8::Local<v8::Value> argv[1] = {v8::Undefined(iso)};
    if (!cons->NewInstance(iso->GetCurrentContext(), 1, argv).ToLocal(&newObj)) {
      this->setError(WorkerError("Failed to instantiate RoaringBitmap64"));
      return;
    }
    RoaringBitmap64 * inst = ObjectWrap::TryUnwrap<RoaringBitmap64>(newObj, iso);
    if (inst == nullptr) {
      this->setError(WorkerError(ERROR_INVALID_OBJECT));
      return;
    }
    roaring64_bitmap_t * r = this->resultBitmap.exchange(nullptr, std::memory_order_acq_rel);
    if (r == nullptr) {
      out = newObj;
      return;
    }
    if (inst->bitmap) roaring64_bitmap_free(inst->bitmap);
    inst->bitmap = r;
    inst->invalidate();
    out = newObj;
  }
};

#endif  // ROARING_NODE_ROARINGBITMAP64_ASYNC_WORKERS_
