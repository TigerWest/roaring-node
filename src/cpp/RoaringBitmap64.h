#ifndef ROARING_NODE_ROARINGBITMAP64_H_
#define ROARING_NODE_ROARINGBITMAP64_H_

#include <string>

#include "object-wrap.h"
#include "v8utils.h"

class RoaringBitmap64;
inline void RoaringBitmap64_WeakCallback(v8::WeakCallbackInfo<RoaringBitmap64> const & info);

class RoaringBitmap64 final : public ObjectWrap {
 public:
  // Token verified by ObjectWrap::TryUnwrap. Distinct from the 32-bit token.
  static const constexpr uint64_t OBJECT_TOKEN = 0x21524F4152360000ULL;

  // Mirrors RoaringBitmap32 frozen lifecycle. >0 = soft frozen counter,
  // 0 = mutable, FROZEN_COUNTER_HARD_FROZEN = frozen view backed by external
  // bytes. RB64 currently uses only HARD_FROZEN (no soft-freeze use case yet).
  static const constexpr int64_t FROZEN_COUNTER_SOFT_FROZEN = -1;
  static const constexpr int64_t FROZEN_COUNTER_HARD_FROZEN = -2;

  roaring64_bitmap_t * bitmap;
  int64_t sizeCache;
  int64_t _version;
  int64_t frozenCounter;
  bool disposed;
  v8::Global<v8::Object> persistent;
  v8utils::TypedArrayContent<uint8_t> frozenStorage;

  inline int64_t getVersion() const { return this->_version; }

  inline void invalidate() {
    this->sizeCache = -1;
    ++this->_version;
  }

  inline bool isFrozen() const { return this->frozenCounter != 0; }
  inline bool isFrozenHard() const { return this->frozenCounter == FROZEN_COUNTER_HARD_FROZEN; }

  inline bool isEmpty() const {
    if (this->sizeCache == 0) return true;
    bool r = this->bitmap == nullptr || roaring64_bitmap_is_empty(this->bitmap);
    if (r) const_cast<RoaringBitmap64 *>(this)->sizeCache = 0;
    return r;
  }

  inline uint64_t getSize() const {
    int64_t s = this->sizeCache;
    if (s < 0) {
      s = this->bitmap ? (int64_t)roaring64_bitmap_get_cardinality(this->bitmap) : 0;
      const_cast<RoaringBitmap64 *>(this)->sizeCache = s;
    }
    return (uint64_t)s;
  }

  // Frees the previous bitmap and installs the new one. Resets sizeCache,
  // bumps _version, clears frozenCounter back to mutable. Caller is
  // responsible for setting frozenCounter / frozenStorage afterwards when
  // installing a frozen view.
  bool replaceBitmapInstance(v8::Isolate * /*isolate*/, roaring64_bitmap_t * newInstance) {
    roaring64_bitmap_t * oldInstance = this->bitmap;
    if (oldInstance == newInstance) return false;
    if (oldInstance != nullptr) {
      roaring64_bitmap_free(oldInstance);
    }
    this->bitmap = newInstance;
    this->frozenCounter = 0;
    this->invalidate();
    return true;
  }

  explicit RoaringBitmap64(AddonData * addonData) :
    ObjectWrap(addonData),
    bitmap(roaring64_bitmap_create()),
    sizeCache(0),
    _version(0),
    frozenCounter(0),
    disposed(false) {
    ++addonData->RoaringBitmap64_instances;
    _gcaware_adjustAllocatedMemory(this->isolate, sizeof(RoaringBitmap64));
  }

  ~RoaringBitmap64() {
    _gcaware_adjustAllocatedMemory(this->isolate, -sizeof(RoaringBitmap64));
    --this->addonData->RoaringBitmap64_instances;
    if (this->bitmap != nullptr) {
      roaring64_bitmap_free(this->bitmap);
      this->bitmap = nullptr;
    }
    if (!this->persistent.IsEmpty()) {
      this->persistent.ClearWeak();
    }
  }
};

// Returns the unwrapped RoaringBitmap64 or null after throwing. Rejects
// disposed and hard-frozen instances. Used by every mutating prototype op
// across main/bulk/ranges/ops headers — defined here so all of them see it.
inline RoaringBitmap64 * RoaringBitmap64_unwrapForMutation(
  v8::Isolate * isolate, v8::Local<v8::Object> obj) {
  RoaringBitmap64 * self = ObjectWrap::TryUnwrap<RoaringBitmap64>(obj, isolate);
  if (self == nullptr || self->disposed) {
    v8utils::throwError(isolate, "RoaringBitmap64 is disposed");
    return nullptr;
  }
  if (self->isFrozenHard()) {
    v8utils::throwError(isolate, "RoaringBitmap64 is frozen and cannot be modified");
    return nullptr;
  }
  return self;
}

// Canonical "unwrap the other RoaringBitmap64" helper used by both prototype
// op-method callbacks (main.h equals/intersects/copyFrom/etc) and the
// in-place macro in ops.h. Returns the borrowed pointer or nullptr after
// throwing a TypeError. Tasks 5-7 (rank/select/cardinality/jaccardIndex)
// use ObjectWrap::TryUnwrap<const RoaringBitmap64> directly with their own
// readonly pattern.
inline RoaringBitmap64 * RoaringBitmap64_unwrapOther(
  v8::Isolate * isolate, const v8::FunctionCallbackInfo<v8::Value> & info, const char * methodName) {
  if (info.Length() < 1) {
    auto msg = std::string(methodName) + " expects a RoaringBitmap64 argument";
    v8utils::throwTypeError(isolate, msg.c_str());
    return nullptr;
  }
  RoaringBitmap64 * other = ObjectWrap::TryUnwrap<RoaringBitmap64>(info[0], isolate);
  if (other == nullptr || other->disposed) {
    auto msg = std::string(methodName) + " argument must be a non-disposed RoaringBitmap64";
    v8utils::throwTypeError(isolate, msg.c_str());
    return nullptr;
  }
  return other;
}

#endif  // ROARING_NODE_ROARINGBITMAP64_H_
