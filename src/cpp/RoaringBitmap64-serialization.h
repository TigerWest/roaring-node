#ifndef ROARING_NODE_ROARINGBITMAP64_SERIALIZATION_H_
#define ROARING_NODE_ROARINGBITMAP64_SERIALIZATION_H_

#include "RoaringBitmap64.h"
#include "RoaringBitmap64-static-ops.h"
#include "addon-data.h"
#include "bigint-utils.h"

namespace RoaringBitmap64_serialization_internal {
inline bool extractBuffer(
  v8::Isolate * isolate, v8::Local<v8::Value> v, const uint8_t ** outData, size_t * outLen) {
  if (roaring_node_bigint::tryGetByteBuffer(isolate, v, outData, outLen)) return true;
  isolate->ThrowException(v8::Exception::TypeError(
    v8::String::NewFromUtf8Literal(
      isolate,
      "RoaringBitmap64.deserialize expects a Buffer, Uint8Array, ArrayBuffer, or DataView",
      v8::NewStringType::kNormal)));
  return false;
}
}  // namespace RoaringBitmap64_serialization_internal

inline void RoaringBitmap64_getSerializationSizeInBytes(const v8::FunctionCallbackInfo<v8::Value> & info) {
  v8::Isolate * isolate = info.GetIsolate();
  const RoaringBitmap64 * self = ObjectWrap::TryUnwrap<const RoaringBitmap64>(info.This(), isolate);
  if (self == nullptr || self->disposed) return v8utils::throwError(isolate, "RoaringBitmap64 is disposed");
  size_t s = roaring64_bitmap_portable_size_in_bytes(self->bitmap);
  info.GetReturnValue().Set(v8::BigInt::NewFromUnsigned(isolate, (uint64_t)s));
}

inline void RoaringBitmap64_serialize(const v8::FunctionCallbackInfo<v8::Value> & info) {
  v8::Isolate * isolate = info.GetIsolate();
  const RoaringBitmap64 * self = ObjectWrap::TryUnwrap<const RoaringBitmap64>(info.This(), isolate);
  if (self == nullptr || self->disposed) return v8utils::throwError(isolate, "RoaringBitmap64 is disposed");

  size_t size = roaring64_bitmap_portable_size_in_bytes(self->bitmap);
  auto bufMaybe = node::Buffer::New(isolate, size);
  v8::Local<v8::Object> buf;
  if (!bufMaybe.ToLocal(&buf)) return;
  size_t written = roaring64_bitmap_portable_serialize(self->bitmap, node::Buffer::Data(buf));
  if (written != size) {
    return v8utils::throwError(isolate, "RoaringBitmap64.serialize size mismatch");
  }
  info.GetReturnValue().Set(buf);
}

inline void RoaringBitmap64_deserializeStatic(const v8::FunctionCallbackInfo<v8::Value> & info) {
  v8::Isolate * isolate = info.GetIsolate();
  AddonData * addonData = AddonData::get(info);
  if (addonData == nullptr) return v8utils::throwError(isolate, ERROR_INVALID_OBJECT);
  if (info.Length() < 1) return v8utils::throwError(isolate, "RoaringBitmap64.deserialize expects 1 argument");

  const uint8_t * data;
  size_t len;
  if (!RoaringBitmap64_serialization_internal::extractBuffer(isolate, info[0], &data, &len)) return;

  roaring64_bitmap_t * r =
    roaring64_bitmap_portable_deserialize_safe(reinterpret_cast<const char *>(data), len);
  if (r == nullptr) return v8utils::throwError(isolate, "RoaringBitmap64.deserialize: invalid roaring buffer");

  auto cons = addonData->RoaringBitmap64_constructor.Get(isolate);
  v8::Local<v8::Object> newObj;
  v8::Local<v8::Value> argv[1] = {v8::Undefined(isolate)};
  if (!cons->NewInstance(isolate->GetCurrentContext(), 1, argv).ToLocal(&newObj)) {
    roaring64_bitmap_free(r);
    return;
  }
  RoaringBitmap64 * out = ObjectWrap::TryUnwrap<RoaringBitmap64>(newObj, isolate);
  if (out == nullptr) {
    roaring64_bitmap_free(r);
    return;
  }
  if (out->bitmap) roaring64_bitmap_free(out->bitmap);
  out->bitmap = r;
  out->invalidate();
  info.GetReturnValue().Set(newObj);
}

inline void RoaringBitmap64_deserializeInstance(const v8::FunctionCallbackInfo<v8::Value> & info) {
  v8::Isolate * isolate = info.GetIsolate();
  RoaringBitmap64 * self = ObjectWrap::TryUnwrap<RoaringBitmap64>(info.This(), isolate);
  if (self == nullptr || self->disposed) return v8utils::throwError(isolate, "RoaringBitmap64 is disposed");
  if (info.Length() < 1) return v8utils::throwError(isolate, "RoaringBitmap64.deserialize expects 1 argument");

  const uint8_t * data;
  size_t len;
  if (!RoaringBitmap64_serialization_internal::extractBuffer(isolate, info[0], &data, &len)) return;

  roaring64_bitmap_t * r =
    roaring64_bitmap_portable_deserialize_safe(reinterpret_cast<const char *>(data), len);
  if (r == nullptr) return v8utils::throwError(isolate, "RoaringBitmap64.deserialize: invalid roaring buffer");

  if (self->bitmap) roaring64_bitmap_free(self->bitmap);
  self->bitmap = r;
  self->invalidate();
  info.GetReturnValue().Set(info.This());
}

inline void RoaringBitmap64_getDeserializationSizeStatic(const v8::FunctionCallbackInfo<v8::Value> & info) {
  v8::Isolate * isolate = info.GetIsolate();
  if (info.Length() < 1) return v8utils::throwError(isolate, "RoaringBitmap64.getDeserializationSize expects 1 argument");
  const uint8_t * data;
  size_t len;
  if (!RoaringBitmap64_serialization_internal::extractBuffer(isolate, info[0], &data, &len)) return;
  size_t s = roaring64_bitmap_portable_deserialize_size(reinterpret_cast<const char *>(data), len);
  info.GetReturnValue().Set(v8::BigInt::NewFromUnsigned(isolate, (uint64_t)s));
}

#endif  // ROARING_NODE_ROARINGBITMAP64_SERIALIZATION_H_
