#ifndef ROARING_NODE_ROARINGBITMAP64_ASYNC_WORKERS_
#define ROARING_NODE_ROARINGBITMAP64_ASYNC_WORKERS_

#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <string>

#include "RoaringBitmap64.h"
#include "async-workers.h"
#include "memory.h"
#include "serialization-format.h"

namespace rb64_async_io {

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
  std::rewind(f);
  size_t size = (size_t)sz;
  char * buf = (size == 0) ? static_cast<char *>(std::malloc(1))
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
  FILE * f = std::fopen(path, "wb");
  if (!f) {
    *outError = WorkerError("RoaringBitmap64.serializeFileAsync: cannot open file");
    return false;
  }
  if (len > 0) {
    size_t w = std::fwrite(data, 1, len, f);
    if (w != len) {
      std::fclose(f);
      *outError = WorkerError("RoaringBitmap64.serializeFileAsync: fwrite short");
      return false;
    }
  }
  std::fclose(f);
  return true;
}

}  // namespace rb64_async_io

class RB64SerializeFileWorker final : public AsyncWorker {
 public:
  const v8::FunctionCallbackInfo<v8::Value> & info;
  v8::Global<v8::Object> bitmapPersistent;
  std::string filePath;
  // Heap-owned snapshot prepared on the main thread; written off-thread.
  char * snapshot;
  size_t snapshotLen;

  explicit RB64SerializeFileWorker(
    const v8::FunctionCallbackInfo<v8::Value> & infoArg, AddonData * addonDataArg) :
    AsyncWorker(infoArg.GetIsolate(), addonDataArg),
    info(infoArg),
    snapshot(nullptr),
    snapshotLen(0) {
    _gcaware_adjustAllocatedMemory(this->isolate, sizeof(RB64SerializeFileWorker));
  }

  ~RB64SerializeFileWorker() override {
    if (snapshot) gcaware_free(snapshot);
    _gcaware_adjustAllocatedMemory(this->isolate, -sizeof(RB64SerializeFileWorker));
  }

 protected:
  void before() final {
    v8::Isolate * iso = this->isolate;
    if (this->info.Length() < 1 || !this->info[0]->IsString()) {
      return this->setError(WorkerError("RoaringBitmap64.serializeFileAsync: filePath must be a string"));
    }
    v8::String::Utf8Value pathUtf(iso, this->info[0]);
    if (!*pathUtf) {
      return this->setError(WorkerError("RoaringBitmap64.serializeFileAsync: invalid filePath"));
    }
    this->filePath.assign(*pathUtf, pathUtf.length());

    bool wantFrozen = false;
    if (this->info.Length() >= 2 && !this->info[1]->IsUndefined()) {
      SerializationFormat fmt = tryParseSerializationFormat(this->info[1], iso);
      if (fmt == SerializationFormat::portable) {
        wantFrozen = false;
      } else if (fmt == SerializationFormat::unsafe_frozen_croaring) {
        wantFrozen = true;
      } else {
        return this->setError(
          WorkerError("RoaringBitmap64.serializeFileAsync: format must be 'portable' or 'unsafe_frozen_croaring'"));
      }
    }

    RoaringBitmap64 * self = ObjectWrap::TryUnwrap<RoaringBitmap64>(this->info.This(), iso);
    if (self == nullptr || self->disposed) {
      return this->setError(WorkerError("RoaringBitmap64 is disposed"));
    }
    if (this->maybeAddonData == nullptr) this->maybeAddonData = self->addonData;
    this->bitmapPersistent.Reset(iso, this->info.This());

    if (wantFrozen) {
      if (self->isFrozenHard()) {
        return this->setError(
          WorkerError("RoaringBitmap64.serializeFileAsync(frozen) cannot operate on a frozen view"));
      }
      roaring64_bitmap_shrink_to_fit(self->bitmap);
      size_t size = roaring64_bitmap_frozen_size_in_bytes(self->bitmap);
      this->snapshot = static_cast<char *>(gcaware_malloc(size == 0 ? 1 : size));
      if (!this->snapshot) {
        return this->setError(WorkerError("RoaringBitmap64.serializeFileAsync: alloc failed"));
      }
      if (size > 0) {
        size_t w = roaring64_bitmap_frozen_serialize(self->bitmap, this->snapshot);
        if (w != size) {
          return this->setError(WorkerError("RoaringBitmap64.serializeFileAsync: frozen size mismatch"));
        }
      }
      this->snapshotLen = size;
    } else {
      size_t size = roaring64_bitmap_portable_size_in_bytes(self->bitmap);
      this->snapshot = static_cast<char *>(gcaware_malloc(size == 0 ? 1 : size));
      if (!this->snapshot) {
        return this->setError(WorkerError("RoaringBitmap64.serializeFileAsync: alloc failed"));
      }
      if (size > 0) {
        size_t w = roaring64_bitmap_portable_serialize(self->bitmap, this->snapshot);
        if (w != size) {
          return this->setError(WorkerError("RoaringBitmap64.serializeFileAsync: portable size mismatch"));
        }
      }
      this->snapshotLen = size;
    }
  }

  void work() final {
    if (this->hasError()) return;
    WorkerError err;
    if (!rb64_async_io::writeFileFully(this->filePath.c_str(), this->snapshot, this->snapshotLen, &err)) {
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
  const v8::FunctionCallbackInfo<v8::Value> & info;
  std::string filePath;
  char * fileBuf;
  size_t fileLen;
  // Worker-thread-produced result; consumed by done() on the main thread.
  std::atomic<roaring64_bitmap_t *> resultBitmap;

  explicit RB64DeserializeFileWorker(
    const v8::FunctionCallbackInfo<v8::Value> & infoArg, AddonData * addonDataArg) :
    AsyncWorker(infoArg.GetIsolate(), addonDataArg),
    info(infoArg),
    fileBuf(nullptr),
    fileLen(0),
    resultBitmap(nullptr) {
    _gcaware_adjustAllocatedMemory(this->isolate, sizeof(RB64DeserializeFileWorker));
  }

  ~RB64DeserializeFileWorker() override {
    if (this->fileBuf) gcaware_free(this->fileBuf);
    roaring64_bitmap_t * b = this->resultBitmap.exchange(nullptr, std::memory_order_acq_rel);
    if (b) roaring64_bitmap_free(b);
    _gcaware_adjustAllocatedMemory(this->isolate, -sizeof(RB64DeserializeFileWorker));
  }

 protected:
  void before() final {
    v8::Isolate * iso = this->isolate;
    if (this->info.Length() < 1 || !this->info[0]->IsString()) {
      return this->setError(
        WorkerError("RoaringBitmap64.deserializeFileAsync: filePath must be a string"));
    }
    v8::String::Utf8Value pathUtf(iso, this->info[0]);
    if (!*pathUtf) {
      return this->setError(
        WorkerError("RoaringBitmap64.deserializeFileAsync: invalid filePath"));
    }
    this->filePath.assign(*pathUtf, pathUtf.length());
    if (this->info.Length() >= 2 && !this->info[1]->IsUndefined()) {
      DeserializationFormat fmt = tryParseDeserializationFormat(this->info[1], iso);
      if (fmt != DeserializationFormat::portable) {
        return this->setError(WorkerError(
          "RoaringBitmap64.deserializeFileAsync: only 'portable' is supported"));
      }
    }
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

#endif  // ROARING_NODE_ROARINGBITMAP64_ASYNC_WORKERS_
