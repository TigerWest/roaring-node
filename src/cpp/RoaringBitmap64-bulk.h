#ifndef ROARING_NODE_ROARINGBITMAP64_BULK_H_
#define ROARING_NODE_ROARINGBITMAP64_BULK_H_

#include <cstddef>
#include <cstdint>
#include <vector>

#include "RoaringBitmap64.h"
#include "bigint-utils.h"

namespace RoaringBitmap64_bulk_internal {

inline bool drainIterable(
  v8::Isolate * isolate,
  v8::Local<v8::Value> iterable,
  std::vector<uint64_t> & out,
  const char * paramName) {
  auto context = isolate->GetCurrentContext();

  if (iterable->IsArray()) {
    auto arr = iterable.As<v8::Array>();
    uint32_t len = arr->Length();
    out.reserve(out.size() + len);
    for (uint32_t i = 0; i < len; ++i) {
      v8::Local<v8::Value> el;
      if (!arr->Get(context, i).ToLocal(&el)) return false;
      uint64_t v;
      if (!roaring_node_bigint::readUint64BigInt(isolate, el, &v, paramName)) return false;
      out.push_back(v);
    }
    return true;
  }

  if (!iterable->IsObject()) {
    auto msg = std::string(paramName) + " must be a BigUint64Array or Iterable<bigint>";
    v8utils::throwTypeError(isolate, msg);
    return false;
  }
  auto obj = iterable.As<v8::Object>();
  auto iteratorSymbol = v8::Symbol::GetIterator(isolate);
  v8::Local<v8::Value> iterFnVal;
  if (!obj->Get(context, iteratorSymbol).ToLocal(&iterFnVal) || !iterFnVal->IsFunction()) {
    auto msg = std::string(paramName) + " must be iterable";
    v8utils::throwTypeError(isolate, msg);
    return false;
  }
  v8::Local<v8::Value> iterVal;
  if (!iterFnVal.As<v8::Function>()->Call(context, obj, 0, nullptr).ToLocal(&iterVal)) {
    // Underlying call already threw — propagate.
    return false;
  }
  if (!iterVal->IsObject()) {
    auto msg = std::string(paramName) + ": iterator factory must return an object";
    v8utils::throwTypeError(isolate, msg);
    return false;
  }
  auto iter = iterVal.As<v8::Object>();
  auto nextKey = v8::String::NewFromUtf8Literal(isolate, "next", v8::NewStringType::kInternalized);
  v8::Local<v8::Value> nextFnVal;
  if (!iter->Get(context, nextKey).ToLocal(&nextFnVal)) return false;
  if (!nextFnVal->IsFunction()) {
    auto msg = std::string(paramName) + ": iterator must have a next() method";
    v8utils::throwTypeError(isolate, msg);
    return false;
  }
  auto nextFn = nextFnVal.As<v8::Function>();
  auto valueKey = v8::String::NewFromUtf8Literal(isolate, "value", v8::NewStringType::kInternalized);
  auto doneKey = v8::String::NewFromUtf8Literal(isolate, "done", v8::NewStringType::kInternalized);

  // DoS guard: a hostile iterator that never returns done would loop forever.
  const uint64_t kMaxIterations = (uint64_t)1 << 32;
  uint64_t iterationCount = 0;

  while (true) {
    if (++iterationCount > kMaxIterations) {
      auto msg = std::string(paramName) + ": iterator exceeded maximum length";
      isolate->ThrowException(v8::Exception::RangeError(
        v8::String::NewFromUtf8(isolate, msg.c_str(), v8::NewStringType::kNormal).ToLocalChecked()));
      return false;
    }
    v8::Local<v8::Value> stepVal;
    if (!nextFn->Call(context, iter, 0, nullptr).ToLocal(&stepVal)) return false;
    if (!stepVal->IsObject()) {
      auto msg = std::string(paramName) + ": iterator next() must return an object";
      v8utils::throwTypeError(isolate, msg);
      return false;
    }
    auto step = stepVal.As<v8::Object>();
    v8::Local<v8::Value> doneVal;
    if (!step->Get(context, doneKey).ToLocal(&doneVal)) return false;
    if (doneVal->BooleanValue(isolate)) break;
    v8::Local<v8::Value> valueVal;
    if (!step->Get(context, valueKey).ToLocal(&valueVal)) return false;
    uint64_t v;
    if (!roaring_node_bigint::readUint64BigInt(isolate, valueVal, &v, paramName)) return false;
    out.push_back(v);
  }
  return true;
}

}  // namespace RoaringBitmap64_bulk_internal

inline void RoaringBitmap64_addMany(const v8::FunctionCallbackInfo<v8::Value> & info) {
  v8::Isolate * isolate = info.GetIsolate();
  RoaringBitmap64 * self = RoaringBitmap64_unwrapForMutation(isolate, info.This());
  if (self == nullptr) return;
  if (info.Length() < 1) return v8utils::throwError(isolate, "RoaringBitmap64.addMany expects 1 argument");

  // RoaringBitmap64 source — fast path via roaring64_bitmap_or_inplace. We
  // detect via the constructor template (HasInstance) so disposed/foreign
  // instances fall through to the iterable path and surface the same errors
  // they would today.
  v8::Local<v8::Value> arg = info[0];
  if (arg->IsObject()) {
    AddonData * addonData = self->addonData;
    if (addonData != nullptr &&
        addonData->RoaringBitmap64_constructorTemplate.Get(isolate)->HasInstance(arg)) {
      const RoaringBitmap64 * other = ObjectWrap::TryUnwrap<const RoaringBitmap64>(arg, isolate);
      if (other != nullptr && !other->disposed && other->bitmap != nullptr) {
        if (other != self) {
          roaring64_bitmap_or_inplace(self->bitmap, other->bitmap);
          self->invalidate();
        }
        info.GetReturnValue().Set(info.This());
        return;
      }
    }
  }

  const uint64_t * data = nullptr;
  size_t n = 0;
  if (roaring_node_bigint::tryGetBigUint64Array(info[0], &data, &n)) {
    if (n > 0) {
      if (data == nullptr) {
        return v8utils::throwError(isolate, "RoaringBitmap64.addMany: BigUint64Array backing store is null (detached?)");
      }
      roaring64_bitmap_add_many(self->bitmap, n, data);
      self->invalidate();
    }
    info.GetReturnValue().Set(info.This());
    return;
  }

  std::vector<uint64_t> values;
  if (!RoaringBitmap64_bulk_internal::drainIterable(isolate, info[0], values, "addMany value")) return;
  if (!values.empty()) {
    roaring64_bitmap_add_many(self->bitmap, values.size(), values.data());
    self->invalidate();
  }
  info.GetReturnValue().Set(info.This());
}

inline void RoaringBitmap64_removeMany(const v8::FunctionCallbackInfo<v8::Value> & info) {
  v8::Isolate * isolate = info.GetIsolate();
  RoaringBitmap64 * self = RoaringBitmap64_unwrapForMutation(isolate, info.This());
  if (self == nullptr) return;
  if (info.Length() < 1) return v8utils::throwError(isolate, "RoaringBitmap64.removeMany expects 1 argument");

  const uint64_t * data = nullptr;
  size_t n = 0;
  if (roaring_node_bigint::tryGetBigUint64Array(info[0], &data, &n)) {
    if (n > 0) {
      if (data == nullptr) {
        return v8utils::throwError(isolate, "RoaringBitmap64.removeMany: BigUint64Array backing store is null (detached?)");
      }
      roaring64_bitmap_remove_many(self->bitmap, n, data);
      self->invalidate();
    }
    info.GetReturnValue().Set(info.This());
    return;
  }

  std::vector<uint64_t> values;
  if (!RoaringBitmap64_bulk_internal::drainIterable(isolate, info[0], values, "removeMany value")) return;
  if (!values.empty()) {
    roaring64_bitmap_remove_many(self->bitmap, values.size(), values.data());
    self->invalidate();
  }
  info.GetReturnValue().Set(info.This());
}

inline void RoaringBitmap64_toUint64Array(const v8::FunctionCallbackInfo<v8::Value> & info) {
  v8::Isolate * isolate = info.GetIsolate();
  const RoaringBitmap64 * self = ObjectWrap::TryUnwrap<const RoaringBitmap64>(info.This(), isolate);
  if (self == nullptr || self->disposed) return v8utils::throwError(isolate, "RoaringBitmap64 is disposed");
  uint64_t card = roaring64_bitmap_get_cardinality(self->bitmap);
  // Cap at node::Buffer::kMaxLength (~2GiB on 32-bit nodes, larger on 64-bit).
  // The previous 0xFFFFFFFFu cap was a 32-bit-style guard inherited from RB32;
  // RB64's whole point is going beyond 32-bit, so we lift it to the actual
  // V8/Node TypedArray size limit.
  if (card > (uint64_t)(node::Buffer::kMaxLength / sizeof(uint64_t))) {
    return v8utils::throwError(
      isolate, "RoaringBitmap64.toUint64Array: cardinality exceeds maximum array length");
  }
  // Out-buffer overload: callers may pass an existing BigUint64Array to fill
  // (avoids the per-call allocation). Returns the same typed array.
  if (info.Length() >= 1 && info[0]->IsBigUint64Array()) {
    v8::Local<v8::BigUint64Array> typed = info[0].As<v8::BigUint64Array>();
    if ((uint64_t)typed->Length() < card) {
      return v8utils::throwError(
        isolate, "RoaringBitmap64.toUint64Array: provided buffer is too small");
    }
    uint64_t * dst = reinterpret_cast<uint64_t *>(
      static_cast<uint8_t *>(typed->Buffer()->GetBackingStore()->Data()) + typed->ByteOffset());
    if (card > 0) {
      if (dst == nullptr) {
        return v8utils::throwError(
          isolate, "RoaringBitmap64.toUint64Array: provided buffer backing store is null");
      }
      roaring64_bitmap_to_uint64_array(self->bitmap, dst);
    }
    info.GetReturnValue().Set(typed);
    return;
  }
  size_t byteLen = (size_t)card * sizeof(uint64_t);
  auto ab = v8::ArrayBuffer::New(isolate, byteLen);
  if (ab.IsEmpty()) {
    return v8utils::throwError(isolate, "RoaringBitmap64.toUint64Array: ArrayBuffer allocation failed");
  }
  if (card > 0) {
    uint64_t * dst = reinterpret_cast<uint64_t *>(ab->GetBackingStore()->Data());
    if (dst == nullptr) {
      return v8utils::throwError(isolate, "RoaringBitmap64.toUint64Array: ArrayBuffer backing store is null");
    }
    roaring64_bitmap_to_uint64_array(self->bitmap, dst);
  }
  auto ta = v8::BigUint64Array::New(ab, 0, (size_t)card);
  info.GetReturnValue().Set(ta);
}

inline void RoaringBitmap64_toArray(const v8::FunctionCallbackInfo<v8::Value> & info) {
  v8::Isolate * isolate = info.GetIsolate();
  auto context = isolate->GetCurrentContext();
  const RoaringBitmap64 * self = ObjectWrap::TryUnwrap<const RoaringBitmap64>(info.This(), isolate);
  if (self == nullptr || self->disposed) return v8utils::throwError(isolate, "RoaringBitmap64 is disposed");

  uint64_t card = roaring64_bitmap_get_cardinality(self->bitmap);
  if (card > 0xFFFFFFFFu) {
    return v8utils::throwError(isolate, "RoaringBitmap64.toArray: cardinality exceeds JS Array limit");
  }
  uint32_t n = (uint32_t)card;
  v8::Local<v8::Array> arr = v8::Array::New(isolate, n);
  if (n == 0) {
    info.GetReturnValue().Set(arr);
    return;
  }

  std::vector<uint64_t> tmp(n);
  roaring64_bitmap_to_uint64_array(self->bitmap, tmp.data());
  for (uint32_t i = 0; i < n; ++i) {
    auto bi = v8::BigInt::NewFromUnsigned(isolate, tmp[i]);
    if (arr->Set(context, i, bi).IsNothing()) return;
  }
  info.GetReturnValue().Set(arr);
}

#endif  // ROARING_NODE_ROARINGBITMAP64_BULK_H_
