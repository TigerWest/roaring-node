#ifndef ROARING_NODE_ROARINGBITMAP64_RANGES_H_
#define ROARING_NODE_ROARINGBITMAP64_RANGES_H_

#include "RoaringBitmap64.h"
#include "bigint-utils.h"

namespace RoaringBitmap64_ranges_internal {

// Reads (start, end) half-open BigInt args. On error throws and returns false.
// On success: writes *outStart, *outEnd. If start >= end, sets *outEmpty = true
// and the caller should treat the call as a no-op.
inline bool readHalfOpenRange(
  v8::Isolate * isolate,
  const v8::FunctionCallbackInfo<v8::Value> & info,
  uint64_t * outStart,
  uint64_t * outEnd,
  bool * outEmpty) {
  if (info.Length() < 2) {
    v8utils::throwError(isolate, "RoaringBitmap64 range op expects (start, end)");
    return false;
  }
  uint64_t s, e;
  if (!roaring_node_bigint::readUint64BigInt(isolate, info[0], &s, "rangeStart")) return false;
  if (!roaring_node_bigint::readUint64BigInt(isolate, info[1], &e, "rangeEnd")) return false;
  *outStart = s;
  *outEnd = e;
  *outEmpty = (s >= e);
  return true;
}

}  // namespace RoaringBitmap64_ranges_internal

inline void RoaringBitmap64_addRange(const v8::FunctionCallbackInfo<v8::Value> & info) {
  v8::Isolate * isolate = info.GetIsolate();
  RoaringBitmap64 * self = ObjectWrap::TryUnwrap<RoaringBitmap64>(info.This(), isolate);
  if (self == nullptr || self->disposed) {
    return v8utils::throwError(isolate, "RoaringBitmap64 is disposed");
  }
  uint64_t s, e;
  bool empty;
  if (!RoaringBitmap64_ranges_internal::readHalfOpenRange(isolate, info, &s, &e, &empty)) return;
  if (!empty) {
    // Use _closed with e-1 to honour the [s, e) contract. Safe because
    // empty==false implies e > s, so e >= 1 and e-1 cannot underflow.
    roaring64_bitmap_add_range_closed(self->bitmap, s, e - 1);
    self->invalidate();
  }
  info.GetReturnValue().Set(info.This());
}

inline void RoaringBitmap64_removeRange(const v8::FunctionCallbackInfo<v8::Value> & info) {
  v8::Isolate * isolate = info.GetIsolate();
  RoaringBitmap64 * self = ObjectWrap::TryUnwrap<RoaringBitmap64>(info.This(), isolate);
  if (self == nullptr || self->disposed) {
    return v8utils::throwError(isolate, "RoaringBitmap64 is disposed");
  }
  uint64_t s, e;
  bool empty;
  if (!RoaringBitmap64_ranges_internal::readHalfOpenRange(isolate, info, &s, &e, &empty)) return;
  if (!empty) {
    roaring64_bitmap_remove_range_closed(self->bitmap, s, e - 1);
    self->invalidate();
  }
  info.GetReturnValue().Set(info.This());
}

inline void RoaringBitmap64_rangeCardinality(const v8::FunctionCallbackInfo<v8::Value> & info) {
  v8::Isolate * isolate = info.GetIsolate();
  const RoaringBitmap64 * self = ObjectWrap::TryUnwrap<const RoaringBitmap64>(info.This(), isolate);
  if (self == nullptr || self->disposed) {
    return v8utils::throwError(isolate, "RoaringBitmap64 is disposed");
  }
  uint64_t s, e;
  bool empty;
  if (!RoaringBitmap64_ranges_internal::readHalfOpenRange(isolate, info, &s, &e, &empty)) return;
  uint64_t card = empty ? 0 : roaring64_bitmap_range_closed_cardinality(self->bitmap, s, e - 1);
  info.GetReturnValue().Set(roaring_node_bigint::makeUint64BigInt(isolate, card));
}

#endif  // ROARING_NODE_ROARINGBITMAP64_RANGES_H_
