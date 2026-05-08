#ifndef ROARING_NODE_ROARINGBITMAP64_H_
#define ROARING_NODE_ROARINGBITMAP64_H_

#include "object-wrap.h"
#include "v8utils.h"

class RoaringBitmap64;
inline void RoaringBitmap64_WeakCallback(v8::WeakCallbackInfo<RoaringBitmap64> const & info);

class RoaringBitmap64 final : public ObjectWrap {
 public:
  // Token verified by ObjectWrap::TryUnwrap. Distinct from the 32-bit token.
  static const constexpr uint64_t OBJECT_TOKEN = 0x21524F4152360000ULL;

  roaring64_bitmap_t * bitmap;
  int64_t sizeCache;
  int64_t _version;
  bool disposed;
  v8::Global<v8::Object> persistent;

  inline int64_t getVersion() const { return this->_version; }

  inline void invalidate() {
    this->sizeCache = -1;
    ++this->_version;
  }

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

  explicit RoaringBitmap64(AddonData * addonData) :
    ObjectWrap(addonData),
    bitmap(roaring64_bitmap_create()),
    sizeCache(0),
    _version(0),
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

#endif  // ROARING_NODE_ROARINGBITMAP64_H_
