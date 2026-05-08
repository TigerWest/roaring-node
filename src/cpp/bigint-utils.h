#ifndef ROARING_NODE_BIGINT_UTILS_H_
#define ROARING_NODE_BIGINT_UTILS_H_

#include "v8utils.h"

namespace roaring_node_bigint {

// Read a BigInt argument as uint64_t. Returns false on type error or range
// loss (and throws a TypeError/RangeError). Returns true and writes *out on
// success.
inline bool readUint64BigInt(
  v8::Isolate * isolate, v8::Local<v8::Value> value, uint64_t * out, const char * paramName) {
  if (!value->IsBigInt()) {
    auto msg = std::string(paramName) + " must be a BigInt";
    isolate->ThrowException(v8::Exception::TypeError(
      v8::String::NewFromUtf8(isolate, msg.c_str(), v8::NewStringType::kNormal).ToLocalChecked()));
    return false;
  }
  bool lossless = false;
  uint64_t v = value.As<v8::BigInt>()->Uint64Value(&lossless);
  if (!lossless) {
    auto msg = std::string(paramName) + " out of uint64 range";
    isolate->ThrowException(v8::Exception::RangeError(
      v8::String::NewFromUtf8(isolate, msg.c_str(), v8::NewStringType::kNormal).ToLocalChecked()));
    return false;
  }
  *out = v;
  return true;
}

inline v8::Local<v8::BigInt> makeUint64BigInt(v8::Isolate * isolate, uint64_t value) {
  return v8::BigInt::NewFromUnsigned(isolate, value);
}

// Try to view a value as a contiguous byte buffer (Buffer / Uint8Array /
// ArrayBuffer / DataView). On success writes *outData and *outLen and
// returns true. On unsupported type returns false WITHOUT throwing.
//
// Detached ArrayBuffers (whether passed directly or via a typed-array /
// DataView view) are rejected (returns false). Their backing store has
// been transferred away; reading would surface stale memory or null.
inline bool tryGetByteBuffer(
  v8::Isolate * /*isolate*/, v8::Local<v8::Value> value, const uint8_t ** outData, size_t * outLen) {
  if (value.IsEmpty()) return false;
  if (node::Buffer::HasInstance(value)) {
    // node::Buffer is backed by an ArrayBuffer; check detachment via the
    // wrapped object before reading Data().
    auto obj = value.As<v8::Object>();
    if (obj->IsArrayBufferView()) {
      auto view = obj.As<v8::ArrayBufferView>();
      if (view->Buffer()->WasDetached()) return false;
    }
    *outData = reinterpret_cast<const uint8_t *>(node::Buffer::Data(value));
    *outLen = node::Buffer::Length(value);
    return true;
  }
  if (value->IsUint8Array()) {
    auto ta = value.As<v8::Uint8Array>();
    auto ab = ta->Buffer();
    if (ab->WasDetached()) return false;
    *outData = static_cast<const uint8_t *>(ab->GetBackingStore()->Data()) + ta->ByteOffset();
    *outLen = ta->ByteLength();
    return true;
  }
  if (value->IsTypedArray()) {
    auto ta = value.As<v8::TypedArray>();
    auto ab = ta->Buffer();
    if (ab->WasDetached()) return false;
    *outData = static_cast<const uint8_t *>(ab->GetBackingStore()->Data()) + ta->ByteOffset();
    *outLen = ta->ByteLength();
    return true;
  }
  if (value->IsDataView()) {
    auto dv = value.As<v8::DataView>();
    auto ab = dv->Buffer();
    if (ab->WasDetached()) return false;
    *outData = static_cast<const uint8_t *>(ab->GetBackingStore()->Data()) + dv->ByteOffset();
    *outLen = dv->ByteLength();
    return true;
  }
  if (value->IsArrayBuffer()) {
    auto ab = value.As<v8::ArrayBuffer>();
    if (ab->WasDetached()) return false;
    *outData = static_cast<const uint8_t *>(ab->GetBackingStore()->Data());
    *outLen = ab->ByteLength();
    return true;
  }
  return false;
}

// View a BigUint64Array as a uint64_t* span. Returns false if not a BigUint64Array.
inline bool tryGetBigUint64Array(
  v8::Local<v8::Value> value, const uint64_t ** outData, size_t * outLen) {
  if (value.IsEmpty() || !value->IsBigUint64Array()) {
    return false;
  }
  auto ta = value.As<v8::BigUint64Array>();
  auto ab = ta->Buffer();
  *outData = reinterpret_cast<const uint64_t *>(
    static_cast<const uint8_t *>(ab->GetBackingStore()->Data()) + ta->ByteOffset());
  *outLen = ta->Length();
  return true;
}

}  // namespace roaring_node_bigint

#endif  // ROARING_NODE_BIGINT_UTILS_H_
