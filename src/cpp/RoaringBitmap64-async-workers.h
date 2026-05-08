#ifndef ROARING_NODE_ROARINGBITMAP64_ASYNC_WORKERS_
#define ROARING_NODE_ROARINGBITMAP64_ASYNC_WORKERS_

#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include "RoaringBitmap64.h"
#include "RoaringBitmap64-bulk.h"
#include "async-workers.h"
#include "bigint-utils.h"
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
  v8::Global<v8::Object> bitmapPersistent;
  std::string filePath;
  // Heap-owned snapshot prepared on the main thread (ctor); written off-thread.
  char * snapshot;
  size_t snapshotLen;

  explicit RB64SerializeFileWorker(
    const v8::FunctionCallbackInfo<v8::Value> & infoArg, AddonData * addonDataArg) :
    AsyncWorker(infoArg.GetIsolate(), addonDataArg),
    snapshot(nullptr),
    snapshotLen(0) {
    _gcaware_adjustAllocatedMemory(this->isolate, sizeof(RB64SerializeFileWorker));

    // All input parsing happens here on the main thread. We must not stash a
    // reference to `infoArg` and read it later: by the time before() runs,
    // the V8 callback frame may already have been torn down.
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

    bool wantFrozen = false;
    if (infoArg.Length() >= 2 && !infoArg[1]->IsUndefined()) {
      SerializationFormat fmt = tryParseSerializationFormat(infoArg[1], iso);
      if (fmt == SerializationFormat::portable) {
        wantFrozen = false;
      } else if (fmt == SerializationFormat::unsafe_frozen_croaring) {
        wantFrozen = true;
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

    if (wantFrozen) {
      if (self->isFrozenHard()) {
        this->setError(WorkerError(
          "RoaringBitmap64.serializeFileAsync(frozen) cannot operate on a frozen view"));
      return;
      }
      roaring64_bitmap_shrink_to_fit(self->bitmap);
      size_t size = roaring64_bitmap_frozen_size_in_bytes(self->bitmap);
      this->snapshot = static_cast<char *>(gcaware_malloc(size == 0 ? 1 : size));
      if (!this->snapshot) {
        this->setError(WorkerError("RoaringBitmap64.serializeFileAsync: alloc failed"));
      return;
      }
      if (size > 0) {
        size_t w = roaring64_bitmap_frozen_serialize(self->bitmap, this->snapshot);
        if (w != size) {
          this->setError(WorkerError("RoaringBitmap64.serializeFileAsync: frozen size mismatch"));
      return;
        }
      }
      this->snapshotLen = size;
    } else {
      size_t size = roaring64_bitmap_portable_size_in_bytes(self->bitmap);
      this->snapshot = static_cast<char *>(gcaware_malloc(size == 0 ? 1 : size));
      if (!this->snapshot) {
        this->setError(WorkerError("RoaringBitmap64.serializeFileAsync: alloc failed"));
      return;
      }
      if (size > 0) {
        size_t w = roaring64_bitmap_portable_serialize(self->bitmap, this->snapshot);
        if (w != size) {
          this->setError(WorkerError("RoaringBitmap64.serializeFileAsync: portable size mismatch"));
      return;
        }
      }
      this->snapshotLen = size;
    }
  }

  ~RB64SerializeFileWorker() override {
    if (snapshot) gcaware_free(snapshot);
    _gcaware_adjustAllocatedMemory(this->isolate, -sizeof(RB64SerializeFileWorker));
  }

 protected:
  void before() final {
    // All parsing happened in the ctor. Any error is already on _error.
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
  // Borrowed bitmap pointer captured on the main thread. The persistent
  // above keeps the wrapper (and its bitmap) alive across the worker hop.
  // CRoaring read-only operations are safe off-thread provided no other
  // thread mutates the bitmap — this is the standard *Async user contract.
  const roaring64_bitmap_t * bitmap;
  // Heap-owned snapshot allocated and filled in work(); done() transfers
  // ownership to a node::Buffer via rb64_async_buffer_free.
  char * snapshot;
  size_t snapshotLen;
  bool wantFrozen;
  bool snapshotOwned;

  explicit RB64SerializeAsyncWorker(
    const v8::FunctionCallbackInfo<v8::Value> & infoArg, AddonData * addonDataArg) :
    AsyncWorker(infoArg.GetIsolate(), addonDataArg),
    bitmap(nullptr),
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
    if (this->wantFrozen) {
      roaring64_bitmap_shrink_to_fit(self->bitmap);
    }
    this->bitmap = self->bitmap;
  }

  ~RB64SerializeAsyncWorker() override {
    if (snapshotOwned && snapshot) gcaware_free(snapshot);
    _gcaware_adjustAllocatedMemory(this->isolate, -sizeof(RB64SerializeAsyncWorker));
  }

 protected:
  void work() final {
    if (this->hasError()) return;
    if (this->bitmap == nullptr) {
      return this->setError(WorkerError("RoaringBitmap64.serializeAsync: null bitmap"));
    }
    if (this->wantFrozen) {
      size_t size = roaring64_bitmap_frozen_size_in_bytes(this->bitmap);
      this->snapshot = static_cast<char *>(gcaware_malloc(size == 0 ? 1 : size));
      if (!this->snapshot) {
        return this->setError(WorkerError("RoaringBitmap64.serializeAsync: alloc failed"));
      }
      if (size > 0) {
        size_t w = roaring64_bitmap_frozen_serialize(this->bitmap, this->snapshot);
        if (w != size) {
          return this->setError(WorkerError("RoaringBitmap64.serializeAsync: frozen size mismatch"));
        }
      }
      this->snapshotLen = size;
    } else {
      size_t size = roaring64_bitmap_portable_size_in_bytes(this->bitmap);
      this->snapshot = static_cast<char *>(gcaware_malloc(size == 0 ? 1 : size));
      if (!this->snapshot) {
        return this->setError(WorkerError("RoaringBitmap64.serializeAsync: alloc failed"));
      }
      if (size > 0) {
        size_t w = roaring64_bitmap_portable_serialize(this->bitmap, this->snapshot);
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
  // Borrowed bitmap pointer captured on the main thread. The persistent
  // above keeps the wrapper (and its bitmap) alive across the hop.
  const roaring64_bitmap_t * bitmap;
  // Heap-owned values. work() allocates and fills; done() wraps and transfers.
  uint64_t * values;
  size_t valuesLen;
  bool valuesOwned;

  explicit RB64ToUint64ArrayWorker(
    const v8::FunctionCallbackInfo<v8::Value> & infoArg, AddonData * addonDataArg) :
    AsyncWorker(infoArg.GetIsolate(), addonDataArg),
    bitmap(nullptr),
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
    this->bitmap = self->bitmap;
  }

  ~RB64ToUint64ArrayWorker() override {
    if (valuesOwned && values) gcaware_free(values);
    _gcaware_adjustAllocatedMemory(this->isolate, -sizeof(RB64ToUint64ArrayWorker));
  }

 protected:
  void work() final {
    if (this->hasError()) return;
    if (this->bitmap == nullptr) return;
    uint64_t card = roaring64_bitmap_get_cardinality(this->bitmap);
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
    roaring64_bitmap_to_uint64_array(this->bitmap, this->values);
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
