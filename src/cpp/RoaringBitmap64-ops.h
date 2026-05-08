#ifndef ROARING_NODE_ROARINGBITMAP64_OPS_H_
#define ROARING_NODE_ROARINGBITMAP64_OPS_H_

#include "RoaringBitmap64.h"

namespace RoaringBitmap64_ops_internal {
inline RoaringBitmap64 * unwrapOther(
  v8::Isolate * isolate, const v8::FunctionCallbackInfo<v8::Value> & info, const char * methodName) {
  if (info.Length() < 1) {
    auto msg = std::string(methodName) + " expects a RoaringBitmap64 argument";
    isolate->ThrowException(v8::Exception::TypeError(
      v8::String::NewFromUtf8(isolate, msg.c_str(), v8::NewStringType::kNormal).ToLocalChecked()));
    return nullptr;
  }
  RoaringBitmap64 * other = ObjectWrap::TryUnwrap<RoaringBitmap64>(info[0], isolate);
  if (other == nullptr || other->disposed) {
    auto msg = std::string(methodName) + " argument must be a non-disposed RoaringBitmap64";
    isolate->ThrowException(v8::Exception::TypeError(
      v8::String::NewFromUtf8(isolate, msg.c_str(), v8::NewStringType::kNormal).ToLocalChecked()));
    return nullptr;
  }
  return other;
}
}  // namespace RoaringBitmap64_ops_internal

#define ROARINGBITMAP64_INPLACE(NAME, FN, METHODNAME)                                               \
  inline void RoaringBitmap64_##NAME(const v8::FunctionCallbackInfo<v8::Value> & info) {            \
    v8::Isolate * isolate = info.GetIsolate();                                                      \
    RoaringBitmap64 * self = ObjectWrap::TryUnwrap<RoaringBitmap64>(info.This(), isolate);          \
    if (self == nullptr || self->disposed) return v8utils::throwError(isolate, "RoaringBitmap64 is disposed"); \
    RoaringBitmap64 * other = RoaringBitmap64_ops_internal::unwrapOther(isolate, info, METHODNAME); \
    if (other == nullptr) return;                                                                   \
    FN(self->bitmap, other->bitmap);                                                                \
    self->invalidate();                                                                             \
    info.GetReturnValue().Set(info.This());                                                         \
  }

ROARINGBITMAP64_INPLACE(andInPlace, roaring64_bitmap_and_inplace, "RoaringBitmap64.andInPlace")
ROARINGBITMAP64_INPLACE(orInPlace, roaring64_bitmap_or_inplace, "RoaringBitmap64.orInPlace")
ROARINGBITMAP64_INPLACE(xorInPlace, roaring64_bitmap_xor_inplace, "RoaringBitmap64.xorInPlace")
ROARINGBITMAP64_INPLACE(andNotInPlace, roaring64_bitmap_andnot_inplace, "RoaringBitmap64.andNotInPlace")

#undef ROARINGBITMAP64_INPLACE

#endif  // ROARING_NODE_ROARINGBITMAP64_OPS_H_
