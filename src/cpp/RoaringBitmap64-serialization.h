#ifndef ROARING_NODE_ROARINGBITMAP64_SERIALIZATION_H_
#define ROARING_NODE_ROARINGBITMAP64_SERIALIZATION_H_

#include "RoaringBitmap64.h"
#include "RoaringBitmap64-static-ops.h"
#include "addon-data.h"
#include "bigint-utils.h"
#include "memory.h"
#include "serialization-format.h"

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

namespace RoaringBitmap64_serialization_internal {

// Returns true on success and writes *outFrozen. Throws and returns false if
// the format string is invalid for RB64 (only portable + unsafe_frozen_croaring
// are supported; unsafe_frozen_portable has no 64-bit C API).
inline bool parseSerializationFormat(
  v8::Isolate * isolate, v8::Local<v8::Value> arg, bool * outFrozen, const char * methodName) {
  if (arg.IsEmpty() || arg->IsUndefined()) {
    *outFrozen = false;
    return true;
  }
  SerializationFormat fmt = tryParseSerializationFormat(arg, isolate);
  if (fmt == SerializationFormat::portable) {
    *outFrozen = false;
    return true;
  }
  if (fmt == SerializationFormat::unsafe_frozen_croaring) {
    *outFrozen = true;
    return true;
  }
  std::string msg(methodName);
  msg += ": format must be 'portable' or 'unsafe_frozen_croaring'";
  v8utils::throwError(isolate, msg.c_str());
  return false;
}

}  // namespace RoaringBitmap64_serialization_internal

inline void RoaringBitmap64_getSerializationSizeInBytes(const v8::FunctionCallbackInfo<v8::Value> & info) {
  v8::Isolate * isolate = info.GetIsolate();
  const RoaringBitmap64 * self = ObjectWrap::TryUnwrap<const RoaringBitmap64>(info.This(), isolate);
  if (self == nullptr || self->disposed) return v8utils::throwError(isolate, "RoaringBitmap64 is disposed");

  bool wantFrozen = false;
  if (info.Length() > 0 &&
      !RoaringBitmap64_serialization_internal::parseSerializationFormat(
        isolate, info[0], &wantFrozen, "RoaringBitmap64.getSerializationSizeInBytes")) {
    return;
  }

  if (wantFrozen) {
    if (!self->isFrozenHard()) {
      // Match serialize(): frozen size requires shrink_to_fit. Skipping the call
      // on a hard-frozen view (which is read-only) keeps the API safe.
      roaring64_bitmap_shrink_to_fit(self->bitmap);
    }
    size_t s = roaring64_bitmap_frozen_size_in_bytes(self->bitmap);
    return info.GetReturnValue().Set(v8::BigInt::NewFromUnsigned(isolate, (uint64_t)s));
  }

  size_t s = roaring64_bitmap_portable_size_in_bytes(self->bitmap);
  info.GetReturnValue().Set(v8::BigInt::NewFromUnsigned(isolate, (uint64_t)s));
}

inline void RoaringBitmap64_serialize(const v8::FunctionCallbackInfo<v8::Value> & info) {
  v8::Isolate * isolate = info.GetIsolate();
  const RoaringBitmap64 * self = ObjectWrap::TryUnwrap<const RoaringBitmap64>(info.This(), isolate);
  if (self == nullptr || self->disposed) return v8utils::throwError(isolate, "RoaringBitmap64 is disposed");

  bool wantFrozen = false;
  if (info.Length() > 0 &&
      !RoaringBitmap64_serialization_internal::parseSerializationFormat(
        isolate, info[0], &wantFrozen, "RoaringBitmap64.serialize")) {
    return;
  }

  if (wantFrozen) {
    if (self->isFrozenHard()) {
      return v8utils::throwError(
        isolate,
        "RoaringBitmap64.serialize('unsafe_frozen_croaring') cannot operate on a frozen view");
    }
    roaring64_bitmap_shrink_to_fit(self->bitmap);
    size_t size = roaring64_bitmap_frozen_size_in_bytes(self->bitmap);
    auto bufMaybe = node::Buffer::New(isolate, size);
    v8::Local<v8::Object> buf;
    if (!bufMaybe.ToLocal(&buf)) return;
    size_t written = roaring64_bitmap_frozen_serialize(self->bitmap, node::Buffer::Data(buf));
    if (written != size) {
      return v8utils::throwError(isolate, "RoaringBitmap64.serialize frozen size mismatch");
    }
    info.GetReturnValue().Set(buf);
    return;
  }

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

// Throws and returns false if the optional format arg is anything other than
// 'portable'. RB64.deserialize never accepts a frozen format — frozen views go
// through unsafeFrozenView. We special-case the message to redirect users.
inline bool RoaringBitmap64_validateDeserializeFormat(
  v8::Isolate * isolate, const v8::FunctionCallbackInfo<v8::Value> & info, int formatIndex) {
  if (info.Length() <= formatIndex || info[formatIndex]->IsUndefined()) return true;
  DeserializationFormat fmt = tryParseDeserializationFormat(info[formatIndex], isolate);
  if (fmt == DeserializationFormat::portable) return true;
  if (fmt == DeserializationFormat::unsafe_frozen_portable ||
      fmt == DeserializationFormat::unsafe_frozen_croaring) {
    v8utils::throwError(
      isolate,
      "RoaringBitmap64.deserialize: frozen formats are not supported via deserialize. "
      "Use RoaringBitmap64.unsafeFrozenView(format, buffer) instead.");
    return false;
  }
  v8utils::throwError(
    isolate, "RoaringBitmap64.deserialize: invalid format (use 'portable')");
  return false;
}

inline void RoaringBitmap64_deserializeStatic(const v8::FunctionCallbackInfo<v8::Value> & info) {
  v8::Isolate * isolate = info.GetIsolate();
  AddonData * addonData = AddonData::get(info);
  if (addonData == nullptr) return v8utils::throwError(isolate, ERROR_INVALID_OBJECT);
  if (info.Length() < 1) return v8utils::throwError(isolate, "RoaringBitmap64.deserialize expects 1 argument");

  if (!RoaringBitmap64_validateDeserializeFormat(isolate, info, 1)) return;

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
  if (self->isFrozenHard()) return v8utils::throwError(isolate, "RoaringBitmap64 is frozen and cannot be modified");
  if (info.Length() < 1) return v8utils::throwError(isolate, "RoaringBitmap64.deserialize expects 1 argument");

  if (!RoaringBitmap64_validateDeserializeFormat(isolate, info, 1)) return;

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

inline void RoaringBitmap64_unsafeFrozenViewStatic(const v8::FunctionCallbackInfo<v8::Value> & info) {
  v8::Isolate * isolate = info.GetIsolate();
  AddonData * addonData = AddonData::get(info);
  if (addonData == nullptr) return v8utils::throwError(isolate, ERROR_INVALID_OBJECT);

  if (info.Length() < 2) {
    return v8utils::throwError(
      isolate, "RoaringBitmap64.unsafeFrozenView expects a format and a buffer");
  }

  // Accept (format, buffer) in either order to mirror RB32's flexibility.
  int bufferArgIndex = 1;
  FrozenViewFormat format = tryParseFrozenViewFormat(info[0], isolate);
  if (format == FrozenViewFormat::INVALID) {
    bufferArgIndex = 0;
    format = tryParseFrozenViewFormat(info[1], isolate);
  }
  if (format == FrozenViewFormat::unsafe_frozen_portable) {
    return v8utils::throwError(
      isolate,
      "RoaringBitmap64.unsafeFrozenView: 'unsafe_frozen_portable' is not supported "
      "(the 64-bit C API has no portable-frozen format). Use 'unsafe_frozen_croaring'.");
  }
  if (format != FrozenViewFormat::unsafe_frozen_croaring) {
    return v8utils::throwError(
      isolate,
      "RoaringBitmap64.unsafeFrozenView: format must be 'unsafe_frozen_croaring'");
  }

  v8::Local<v8::Function> cons = addonData->RoaringBitmap64_constructor.Get(isolate);
  v8::Local<v8::Object> result;
  v8::Local<v8::Value> argv[1] = {v8::Undefined(isolate)};
  if (!cons->NewInstance(isolate->GetCurrentContext(), 1, argv).ToLocal(&result)) {
    return;
  }

  RoaringBitmap64 * self = ObjectWrap::TryUnwrap<RoaringBitmap64>(result, isolate);
  if (self == nullptr) return v8utils::throwError(isolate, ERROR_INVALID_OBJECT);

  v8::Local<v8::Object> bufferObj;
  if (!info[bufferArgIndex]->ToObject(isolate->GetCurrentContext()).ToLocal(&bufferObj)) {
    return v8utils::throwError(isolate, "RoaringBitmap64.unsafeFrozenView: invalid buffer");
  }
  if (!self->frozenStorage.set(isolate, bufferObj)) {
    return v8utils::throwError(isolate, "RoaringBitmap64.unsafeFrozenView: invalid buffer");
  }

  if (!is_pointer_aligned(self->frozenStorage.data, 64)) {
    return v8utils::throwError(
      isolate,
      "RoaringBitmap64.unsafeFrozenView requires the buffer to be 64-byte aligned. "
      "Use bufferAlignedAlloc(size, 64) or ensureBufferAligned(buf, 64).");
  }

  roaring64_bitmap_t * bitmap = roaring64_bitmap_frozen_view(
    reinterpret_cast<const char *>(self->frozenStorage.data),
    self->frozenStorage.length);

  if (bitmap == nullptr) {
    return v8utils::throwError(
      isolate, "RoaringBitmap64.unsafeFrozenView failed to deserialize the input");
  }

  if (self->bitmap != nullptr) {
    roaring64_bitmap_free(self->bitmap);
  }
  self->bitmap = bitmap;
  self->frozenCounter = RoaringBitmap64::FROZEN_COUNTER_HARD_FROZEN;
  self->invalidate();

  info.GetReturnValue().Set(result);
}

#include "RoaringBitmap64-async-workers.h"

inline void RoaringBitmap64_serializeFileAsync(const v8::FunctionCallbackInfo<v8::Value> & info) {
  AddonData * ad = AddonData::get(info);
  auto * worker = new RB64SerializeFileWorker(info, ad);
  info.GetReturnValue().Set(AsyncWorker::run(worker));
}

inline void RoaringBitmap64_deserializeFileAsyncStatic(const v8::FunctionCallbackInfo<v8::Value> & info) {
  AddonData * ad = AddonData::get(info);
  auto * worker = new RB64DeserializeFileWorker(info, ad);
  info.GetReturnValue().Set(AsyncWorker::run(worker));
}

#endif  // ROARING_NODE_ROARINGBITMAP64_SERIALIZATION_H_
