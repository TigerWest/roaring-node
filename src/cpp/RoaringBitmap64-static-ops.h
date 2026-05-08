#ifndef ROARING_NODE_ROARINGBITMAP64_STATIC_OPS_H_
#define ROARING_NODE_ROARINGBITMAP64_STATIC_OPS_H_

#include "RoaringBitmap64.h"
#include "addon-data.h"

namespace RoaringBitmap64_static_internal {
inline bool unwrapPair(
  v8::Isolate * isolate, const v8::FunctionCallbackInfo<v8::Value> & info, const char * methodName,
  const RoaringBitmap64 ** outA, const RoaringBitmap64 ** outB) {
  if (info.Length() < 2) {
    auto msg = std::string(methodName) + " expects two RoaringBitmap64 arguments";
    isolate->ThrowException(v8::Exception::TypeError(
      v8::String::NewFromUtf8(isolate, msg.c_str(), v8::NewStringType::kNormal).ToLocalChecked()));
    return false;
  }
  *outA = ObjectWrap::TryUnwrap<const RoaringBitmap64>(info[0], isolate);
  *outB = ObjectWrap::TryUnwrap<const RoaringBitmap64>(info[1], isolate);
  if (*outA == nullptr || *outB == nullptr || (*outA)->disposed || (*outB)->disposed) {
    auto msg = std::string(methodName) + " arguments must be non-disposed RoaringBitmap64";
    isolate->ThrowException(v8::Exception::TypeError(
      v8::String::NewFromUtf8(isolate, msg.c_str(), v8::NewStringType::kNormal).ToLocalChecked()));
    return false;
  }
  return true;
}

inline void returnNewBitmap(
  v8::Isolate * isolate, AddonData * addonData,
  const v8::FunctionCallbackInfo<v8::Value> & info, roaring64_bitmap_t * raw) {
  auto cons = addonData->RoaringBitmap64_constructor.Get(isolate);
  v8::Local<v8::Object> newObj;
  v8::Local<v8::Value> argv[1] = {v8::Undefined(isolate)};
  if (!cons->NewInstance(isolate->GetCurrentContext(), 1, argv).ToLocal(&newObj)) {
    if (raw) roaring64_bitmap_free(raw);
    return;
  }
  RoaringBitmap64 * other = ObjectWrap::TryUnwrap<RoaringBitmap64>(newObj, isolate);
  if (other == nullptr) {
    if (raw) roaring64_bitmap_free(raw);
    return;
  }
  if (other->bitmap) roaring64_bitmap_free(other->bitmap);
  other->bitmap = raw;
  other->invalidate();
  info.GetReturnValue().Set(newObj);
}
}  // namespace RoaringBitmap64_static_internal

#define ROARINGBITMAP64_STATIC_OP(NAME, FN, METHODNAME)                                                 \
  inline void RoaringBitmap64_##NAME##Static(const v8::FunctionCallbackInfo<v8::Value> & info) {        \
    v8::Isolate * isolate = info.GetIsolate();                                                          \
    AddonData * addonData = AddonData::get(info);                                                       \
    if (addonData == nullptr) return v8utils::throwError(isolate, ERROR_INVALID_OBJECT);                \
    const RoaringBitmap64 * a;                                                                          \
    const RoaringBitmap64 * b;                                                                          \
    if (!RoaringBitmap64_static_internal::unwrapPair(isolate, info, METHODNAME, &a, &b)) return;        \
    roaring64_bitmap_t * r = FN(a->bitmap, b->bitmap);                                                  \
    RoaringBitmap64_static_internal::returnNewBitmap(isolate, addonData, info, r);                      \
  }

ROARINGBITMAP64_STATIC_OP(and, roaring64_bitmap_and, "RoaringBitmap64.and")
ROARINGBITMAP64_STATIC_OP(or, roaring64_bitmap_or, "RoaringBitmap64.or")
ROARINGBITMAP64_STATIC_OP(xor, roaring64_bitmap_xor, "RoaringBitmap64.xor")
ROARINGBITMAP64_STATIC_OP(andNot, roaring64_bitmap_andnot, "RoaringBitmap64.andNot")

#undef ROARINGBITMAP64_STATIC_OP

#define ROARINGBITMAP64_STATIC_CARD(NAME, FN, METHODNAME)                                                \
  inline void RoaringBitmap64_##NAME##CardinalityStatic(const v8::FunctionCallbackInfo<v8::Value> & info) { \
    v8::Isolate * isolate = info.GetIsolate();                                                           \
    const RoaringBitmap64 * a;                                                                           \
    const RoaringBitmap64 * b;                                                                           \
    if (!RoaringBitmap64_static_internal::unwrapPair(isolate, info, METHODNAME, &a, &b)) return;         \
    uint64_t r = FN(a->bitmap, b->bitmap);                                                               \
    info.GetReturnValue().Set(v8::BigInt::NewFromUnsigned(isolate, r));                                  \
  }

ROARINGBITMAP64_STATIC_CARD(and, roaring64_bitmap_and_cardinality, "RoaringBitmap64.andCardinality")
ROARINGBITMAP64_STATIC_CARD(or, roaring64_bitmap_or_cardinality, "RoaringBitmap64.orCardinality")
ROARINGBITMAP64_STATIC_CARD(xor, roaring64_bitmap_xor_cardinality, "RoaringBitmap64.xorCardinality")
ROARINGBITMAP64_STATIC_CARD(andNot, roaring64_bitmap_andnot_cardinality, "RoaringBitmap64.andNotCardinality")

#undef ROARINGBITMAP64_STATIC_CARD

inline void RoaringBitmap64_jaccardIndexStatic(const v8::FunctionCallbackInfo<v8::Value> & info) {
  v8::Isolate * isolate = info.GetIsolate();
  const RoaringBitmap64 * a;
  const RoaringBitmap64 * b;
  if (!RoaringBitmap64_static_internal::unwrapPair(isolate, info, "RoaringBitmap64.jaccardIndex", &a, &b)) return;
  double r = roaring64_bitmap_jaccard_index(a->bitmap, b->bitmap);
  info.GetReturnValue().Set(r);
}

#endif  // ROARING_NODE_ROARINGBITMAP64_STATIC_OPS_H_
