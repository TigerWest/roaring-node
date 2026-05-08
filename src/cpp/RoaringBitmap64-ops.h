#ifndef ROARING_NODE_ROARINGBITMAP64_OPS_H_
#define ROARING_NODE_ROARINGBITMAP64_OPS_H_

#include "RoaringBitmap64.h"

#define ROARINGBITMAP64_INPLACE(NAME, FN, METHODNAME)                                  \
  inline void RoaringBitmap64_##NAME(const v8::FunctionCallbackInfo<v8::Value> & info) {\
    v8::Isolate * isolate = info.GetIsolate();                                          \
    RoaringBitmap64 * self = RoaringBitmap64_unwrapForMutation(isolate, info.This());   \
    if (self == nullptr) return;                                                        \
    RoaringBitmap64 * other = RoaringBitmap64_unwrapOther(isolate, info, METHODNAME);   \
    if (other == nullptr) return;                                                       \
    FN(self->bitmap, other->bitmap);                                                    \
    self->invalidate();                                                                 \
    info.GetReturnValue().Set(info.This());                                             \
  }

ROARINGBITMAP64_INPLACE(andInPlace, roaring64_bitmap_and_inplace, "RoaringBitmap64.andInPlace")
ROARINGBITMAP64_INPLACE(orInPlace, roaring64_bitmap_or_inplace, "RoaringBitmap64.orInPlace")
ROARINGBITMAP64_INPLACE(xorInPlace, roaring64_bitmap_xor_inplace, "RoaringBitmap64.xorInPlace")
ROARINGBITMAP64_INPLACE(andNotInPlace, roaring64_bitmap_andnot_inplace, "RoaringBitmap64.andNotInPlace")

#undef ROARINGBITMAP64_INPLACE

#define ROARINGBITMAP64_INSTANCE_CARD(NAME, FN, METHODNAME)                                          \
  inline void RoaringBitmap64_##NAME##Cardinality(const v8::FunctionCallbackInfo<v8::Value> & info) {\
    v8::Isolate * isolate = info.GetIsolate();                                                       \
    const RoaringBitmap64 * self = ObjectWrap::TryUnwrap<const RoaringBitmap64>(info.This(), isolate);\
    if (self == nullptr || self->disposed) {                                                         \
      return v8utils::throwError(isolate, "RoaringBitmap64 is disposed");                            \
    }                                                                                                \
    RoaringBitmap64 * other = RoaringBitmap64_unwrapOther(isolate, info, METHODNAME);                \
    if (other == nullptr) return;                                                                    \
    uint64_t r = FN(self->bitmap, other->bitmap);                                                    \
    info.GetReturnValue().Set(v8::BigInt::NewFromUnsigned(isolate, r));                              \
  }

ROARINGBITMAP64_INSTANCE_CARD(and, roaring64_bitmap_and_cardinality, "RoaringBitmap64.andCardinality")
ROARINGBITMAP64_INSTANCE_CARD(or, roaring64_bitmap_or_cardinality, "RoaringBitmap64.orCardinality")
ROARINGBITMAP64_INSTANCE_CARD(xor, roaring64_bitmap_xor_cardinality, "RoaringBitmap64.xorCardinality")
ROARINGBITMAP64_INSTANCE_CARD(andNot, roaring64_bitmap_andnot_cardinality, "RoaringBitmap64.andNotCardinality")

#undef ROARINGBITMAP64_INSTANCE_CARD

// jaccardIndex (instance) — mirrors the static jaccardIndex but on prototype.
inline void RoaringBitmap64_jaccardIndex(const v8::FunctionCallbackInfo<v8::Value> & info) {
  v8::Isolate * isolate = info.GetIsolate();
  const RoaringBitmap64 * self = ObjectWrap::TryUnwrap<const RoaringBitmap64>(info.This(), isolate);
  if (self == nullptr || self->disposed) {
    return v8utils::throwError(isolate, "RoaringBitmap64 is disposed");
  }
  RoaringBitmap64 * other = RoaringBitmap64_unwrapOther(isolate, info, "RoaringBitmap64.jaccardIndex");
  if (other == nullptr) return;
  info.GetReturnValue().Set(
    v8::Number::New(isolate, roaring64_bitmap_jaccard_index(self->bitmap, other->bitmap)));
}

#endif  // ROARING_NODE_ROARINGBITMAP64_OPS_H_
