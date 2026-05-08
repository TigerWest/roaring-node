#ifndef ROARING_NODE_ROARINGBITMAP64_MAIN_H_
#define ROARING_NODE_ROARINGBITMAP64_MAIN_H_

#include <cstdio>

#include "RoaringBitmap64.h"
#include "RoaringBitmap64-async-workers.h"
#include "RoaringBitmap64-bulk.h"
#include "RoaringBitmap64-ops.h"
#include "RoaringBitmap64-ranges.h"
#include "RoaringBitmap64-static-ops.h"
#include "RoaringBitmap64-serialization.h"
#include "RoaringBitmap64Iterator.h"
#include "RoaringBitmap64ReverseIterator.h"
#include "addon-data.h"
#include "bigint-utils.h"

inline void RoaringBitmap64_WeakCallback(v8::WeakCallbackInfo<RoaringBitmap64> const & info) {
  RoaringBitmap64 * p = info.GetParameter();
  if (p != nullptr) {
    p->~RoaringBitmap64();
    bare_aligned_free(p);
  }
}

inline void RoaringBitmap64_New(const v8::FunctionCallbackInfo<v8::Value> & info) {
  v8::Isolate * isolate = info.GetIsolate();
  AddonData * addonData = AddonData::get(info);
  if (addonData == nullptr) {
    return v8utils::throwError(isolate, ERROR_INVALID_OBJECT);
  }

  if (!info.IsConstructCall()) {
    v8::Local<v8::Function> cons = addonData->RoaringBitmap64_constructor.Get(isolate);
    v8::MaybeLocal<v8::Object> v;
    if (info.Length() < 1) {
      v = cons->NewInstance(isolate->GetCurrentContext(), 0, nullptr);
    } else {
      v8::Local<v8::Value> argv[1] = {info[0]};
      v = cons->NewInstance(isolate->GetCurrentContext(), 1, argv);
    }
    v8::Local<v8::Object> vlocal;
    if (v.ToLocal(&vlocal)) {
      info.GetReturnValue().Set(vlocal);
    }
    return;
  }

  auto holder = info.This();
  auto * mem = bare_aligned_malloc(32, sizeof(RoaringBitmap64));
  if (mem == nullptr) {
    return v8utils::throwError(isolate, "RoaringBitmap64::ctor - allocation failed");
  }
  RoaringBitmap64 * instance = new (mem) RoaringBitmap64(addonData);
  if (instance->bitmap == nullptr) {
    instance->~RoaringBitmap64();
    bare_aligned_free(instance);
    return v8utils::throwError(isolate, "RoaringBitmap64::ctor - failed to create instance");
  }

  int indices[2] = {0, 1};
  void * values[2] = {instance, (void *)(RoaringBitmap64::OBJECT_TOKEN)};
  holder->SetAlignedPointerInInternalFields(2, indices, values);

  instance->persistent.Reset(isolate, holder);
  instance->persistent.SetWeak(instance, RoaringBitmap64_WeakCallback, v8::WeakCallbackType::kParameter);

  if (info.Length() != 0 && !info[0]->IsUndefined() && !info[0]->IsNull()) {
    // Reject another RoaringBitmap64 instance explicitly: passing a bitmap to
    // the constructor is ambiguous (copy vs. iterate) and was previously a
    // silent no-op. Users should call .clone() instead.
    RoaringBitmap64 * sourceRB64 = ObjectWrap::TryUnwrap<RoaringBitmap64>(info[0], isolate);
    if (sourceRB64 != nullptr) {
      return v8utils::throwTypeError(
        isolate,
        "RoaringBitmap64 constructor does not accept another RoaringBitmap64; use .clone() instead");
    }
    RoaringBitmap64_addMany(info);
  }

  info.GetReturnValue().Set(holder);
}

// ---- Element ops ----

inline void RoaringBitmap64_add(const v8::FunctionCallbackInfo<v8::Value> & info) {
  v8::Isolate * isolate = info.GetIsolate();
  RoaringBitmap64 * self = RoaringBitmap64_unwrapForMutation(isolate, info.This());
  if (self == nullptr) return;
  if (info.Length() < 1) {
    return v8utils::throwError(isolate, "RoaringBitmap64.add expects 1 argument");
  }
  uint64_t v;
  if (!roaring_node_bigint::readUint64BigInt(isolate, info[0], &v, "value")) return;
  roaring64_bitmap_add(self->bitmap, v);
  self->invalidate();
  info.GetReturnValue().Set(info.This());
}

inline void RoaringBitmap64_tryAdd(const v8::FunctionCallbackInfo<v8::Value> & info) {
  v8::Isolate * isolate = info.GetIsolate();
  RoaringBitmap64 * self = RoaringBitmap64_unwrapForMutation(isolate, info.This());
  if (self == nullptr) return;
  if (info.Length() < 1) {
    return v8utils::throwError(isolate, "RoaringBitmap64.tryAdd expects 1 argument");
  }
  uint64_t v;
  if (!roaring_node_bigint::readUint64BigInt(isolate, info[0], &v, "value")) return;
  bool inserted = roaring64_bitmap_add_checked(self->bitmap, v);
  if (inserted) self->invalidate();
  info.GetReturnValue().Set(inserted);
}

inline void RoaringBitmap64_remove(const v8::FunctionCallbackInfo<v8::Value> & info) {
  v8::Isolate * isolate = info.GetIsolate();
  RoaringBitmap64 * self = RoaringBitmap64_unwrapForMutation(isolate, info.This());
  if (self == nullptr) return;
  if (info.Length() < 1) {
    return v8utils::throwError(isolate, "RoaringBitmap64.remove expects 1 argument");
  }
  uint64_t v;
  if (!roaring_node_bigint::readUint64BigInt(isolate, info[0], &v, "value")) return;
  roaring64_bitmap_remove(self->bitmap, v);
  self->invalidate();
  info.GetReturnValue().Set(info.This());
}

inline void RoaringBitmap64_delete(const v8::FunctionCallbackInfo<v8::Value> & info) {
  v8::Isolate * isolate = info.GetIsolate();
  RoaringBitmap64 * self = RoaringBitmap64_unwrapForMutation(isolate, info.This());
  if (self == nullptr) return;
  if (info.Length() < 1) {
    return v8utils::throwError(isolate, "RoaringBitmap64.delete expects 1 argument");
  }
  uint64_t v;
  if (!roaring_node_bigint::readUint64BigInt(isolate, info[0], &v, "value")) return;
  bool removed = roaring64_bitmap_remove_checked(self->bitmap, v);
  if (removed) self->invalidate();
  info.GetReturnValue().Set(removed);
}

inline void RoaringBitmap64_has(const v8::FunctionCallbackInfo<v8::Value> & info) {
  v8::Isolate * isolate = info.GetIsolate();
  const RoaringBitmap64 * self = ObjectWrap::TryUnwrap<const RoaringBitmap64>(info.This(), isolate);
  if (self == nullptr || self->disposed) {
    return v8utils::throwError(isolate, "RoaringBitmap64 is disposed");
  }
  if (info.Length() < 1) {
    info.GetReturnValue().Set(false);
    return;
  }
  uint64_t v;
  if (!roaring_node_bigint::readUint64BigInt(isolate, info[0], &v, "value")) return;
  info.GetReturnValue().Set(roaring64_bitmap_contains(self->bitmap, v));
}

inline void RoaringBitmap64_clear(const v8::FunctionCallbackInfo<v8::Value> & info) {
  v8::Isolate * isolate = info.GetIsolate();
  RoaringBitmap64 * self = RoaringBitmap64_unwrapForMutation(isolate, info.This());
  if (self == nullptr) return;
  roaring64_bitmap_clear(self->bitmap);
  self->invalidate();
}

// ---- Property getters ----

inline void RoaringBitmap64_size_getter(
  v8::Local<v8::Name>, const v8::PropertyCallbackInfo<v8::Value> & info) {
  v8::Isolate * isolate = info.GetIsolate();
  const RoaringBitmap64 * self = ObjectWrap::TryUnwrap<const RoaringBitmap64>(info.This(), isolate);
  uint64_t s = (self == nullptr || self->disposed) ? 0 : self->getSize();
  info.GetReturnValue().Set(v8::BigInt::NewFromUnsigned(isolate, s));
}

inline void RoaringBitmap64_isEmpty_getter(
  v8::Local<v8::Name>, const v8::PropertyCallbackInfo<v8::Value> & info) {
  v8::Isolate * isolate = info.GetIsolate();
  const RoaringBitmap64 * self = ObjectWrap::TryUnwrap<const RoaringBitmap64>(info.This(), isolate);
  info.GetReturnValue().Set(self == nullptr || self->disposed ? true : self->isEmpty());
}

inline void RoaringBitmap64_isDisposed_getter(
  v8::Local<v8::Name>, const v8::PropertyCallbackInfo<v8::Value> & info) {
  v8::Isolate * isolate = info.GetIsolate();
  const RoaringBitmap64 * self = ObjectWrap::TryUnwrap<const RoaringBitmap64>(info.This(), isolate);
  info.GetReturnValue().Set(self == nullptr ? true : self->disposed);
}

inline void RoaringBitmap64_isFrozen_getter(
  v8::Local<v8::Name>, const v8::PropertyCallbackInfo<v8::Value> & info) {
  v8::Isolate * isolate = info.GetIsolate();
  const RoaringBitmap64 * self = ObjectWrap::TryUnwrap<const RoaringBitmap64>(info.This(), isolate);
  info.GetReturnValue().Set(self != nullptr && self->isFrozen());
}

// ---- min / max / clone ----

inline void RoaringBitmap64_minimum(const v8::FunctionCallbackInfo<v8::Value> & info) {
  v8::Isolate * isolate = info.GetIsolate();
  const RoaringBitmap64 * self = ObjectWrap::TryUnwrap<const RoaringBitmap64>(info.This(), isolate);
  if (self == nullptr || self->disposed) {
    return v8utils::throwError(isolate, "RoaringBitmap64 is disposed");
  }
  if (self->isEmpty()) {
    info.GetReturnValue().SetUndefined();
    return;
  }
  uint64_t m = roaring64_bitmap_minimum(self->bitmap);
  info.GetReturnValue().Set(v8::BigInt::NewFromUnsigned(isolate, m));
}

inline void RoaringBitmap64_maximum(const v8::FunctionCallbackInfo<v8::Value> & info) {
  v8::Isolate * isolate = info.GetIsolate();
  const RoaringBitmap64 * self = ObjectWrap::TryUnwrap<const RoaringBitmap64>(info.This(), isolate);
  if (self == nullptr || self->disposed) {
    return v8utils::throwError(isolate, "RoaringBitmap64 is disposed");
  }
  if (self->isEmpty()) {
    info.GetReturnValue().SetUndefined();
    return;
  }
  uint64_t m = roaring64_bitmap_maximum(self->bitmap);
  info.GetReturnValue().Set(v8::BigInt::NewFromUnsigned(isolate, m));
}

inline void RoaringBitmap64_clone(const v8::FunctionCallbackInfo<v8::Value> & info) {
  v8::Isolate * isolate = info.GetIsolate();
  RoaringBitmap64 * self = ObjectWrap::TryUnwrap<RoaringBitmap64>(info.This(), isolate);
  if (self == nullptr || self->disposed) {
    return v8utils::throwError(isolate, "RoaringBitmap64 is disposed");
  }
  AddonData * addonData = self->addonData;
  v8::Local<v8::Function> cons = addonData->RoaringBitmap64_constructor.Get(isolate);
  // Pass no arguments: the constructor rejects RoaringBitmap64 instances
  // explicitly; we copy the bitmap manually below.
  v8::Local<v8::Object> newObj;
  if (!cons->NewInstance(isolate->GetCurrentContext(), 0, nullptr).ToLocal(&newObj)) {
    return v8utils::throwError(isolate, "RoaringBitmap64.clone failed to create instance");
  }
  RoaringBitmap64 * other = ObjectWrap::TryUnwrap<RoaringBitmap64>(newObj, isolate);
  if (other == nullptr) {
    return v8utils::throwError(isolate, "RoaringBitmap64.clone failed to create instance");
  }

  roaring64_bitmap_t * copy = roaring64_bitmap_copy(self->bitmap);
  if (copy == nullptr) {
    return v8utils::throwError(isolate, "RoaringBitmap64.clone: allocation failed");
  }
  if (other->bitmap) roaring64_bitmap_free(other->bitmap);
  other->bitmap = copy;
  other->invalidate();
  info.GetReturnValue().Set(newObj);
}

// ---- Comparisons ----

namespace RoaringBitmap64_main_internal {
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
}  // namespace RoaringBitmap64_main_internal

inline void RoaringBitmap64_equals(const v8::FunctionCallbackInfo<v8::Value> & info) {
  v8::Isolate * isolate = info.GetIsolate();
  const RoaringBitmap64 * self = ObjectWrap::TryUnwrap<const RoaringBitmap64>(info.This(), isolate);
  if (self == nullptr || self->disposed) return v8utils::throwError(isolate, "RoaringBitmap64 is disposed");
  RoaringBitmap64 * other = RoaringBitmap64_main_internal::unwrapOther(isolate, info, "RoaringBitmap64.equals");
  if (other == nullptr) return;
  info.GetReturnValue().Set(roaring64_bitmap_equals(self->bitmap, other->bitmap));
}

inline void RoaringBitmap64_isSubset(const v8::FunctionCallbackInfo<v8::Value> & info) {
  v8::Isolate * isolate = info.GetIsolate();
  const RoaringBitmap64 * self = ObjectWrap::TryUnwrap<const RoaringBitmap64>(info.This(), isolate);
  if (self == nullptr || self->disposed) return v8utils::throwError(isolate, "RoaringBitmap64 is disposed");
  RoaringBitmap64 * other = RoaringBitmap64_main_internal::unwrapOther(isolate, info, "RoaringBitmap64.isSubset");
  if (other == nullptr) return;
  info.GetReturnValue().Set(roaring64_bitmap_is_subset(self->bitmap, other->bitmap));
}

inline void RoaringBitmap64_isStrictSubset(const v8::FunctionCallbackInfo<v8::Value> & info) {
  v8::Isolate * isolate = info.GetIsolate();
  const RoaringBitmap64 * self = ObjectWrap::TryUnwrap<const RoaringBitmap64>(info.This(), isolate);
  if (self == nullptr || self->disposed) return v8utils::throwError(isolate, "RoaringBitmap64 is disposed");
  RoaringBitmap64 * other = RoaringBitmap64_main_internal::unwrapOther(isolate, info, "RoaringBitmap64.isStrictSubset");
  if (other == nullptr) return;
  info.GetReturnValue().Set(roaring64_bitmap_is_strict_subset(self->bitmap, other->bitmap));
}

inline void RoaringBitmap64_intersects(const v8::FunctionCallbackInfo<v8::Value> & info) {
  v8::Isolate * isolate = info.GetIsolate();
  const RoaringBitmap64 * self = ObjectWrap::TryUnwrap<const RoaringBitmap64>(info.This(), isolate);
  if (self == nullptr || self->disposed) return v8utils::throwError(isolate, "RoaringBitmap64 is disposed");
  RoaringBitmap64 * other = RoaringBitmap64_main_internal::unwrapOther(isolate, info, "RoaringBitmap64.intersects");
  if (other == nullptr) return;
  info.GetReturnValue().Set(roaring64_bitmap_intersect(self->bitmap, other->bitmap));
}

inline void RoaringBitmap64_isSuperset(const v8::FunctionCallbackInfo<v8::Value> & info) {
  v8::Isolate * isolate = info.GetIsolate();
  const RoaringBitmap64 * self = ObjectWrap::TryUnwrap<const RoaringBitmap64>(info.This(), isolate);
  if (self == nullptr || self->disposed) return v8utils::throwError(isolate, "RoaringBitmap64 is disposed");
  RoaringBitmap64 * other = RoaringBitmap64_main_internal::unwrapOther(isolate, info, "RoaringBitmap64.isSuperset");
  if (other == nullptr) return;
  // CRoaring 64-bit exposes only is_subset; superset is the swapped-arg form.
  info.GetReturnValue().Set(roaring64_bitmap_is_subset(other->bitmap, self->bitmap));
}

inline void RoaringBitmap64_isStrictSuperset(const v8::FunctionCallbackInfo<v8::Value> & info) {
  v8::Isolate * isolate = info.GetIsolate();
  const RoaringBitmap64 * self = ObjectWrap::TryUnwrap<const RoaringBitmap64>(info.This(), isolate);
  if (self == nullptr || self->disposed) return v8utils::throwError(isolate, "RoaringBitmap64 is disposed");
  RoaringBitmap64 * other = RoaringBitmap64_main_internal::unwrapOther(isolate, info, "RoaringBitmap64.isStrictSuperset");
  if (other == nullptr) return;
  info.GetReturnValue().Set(roaring64_bitmap_is_strict_subset(other->bitmap, self->bitmap));
}

// ---- copyFrom ----

inline void RoaringBitmap64_copyFrom(const v8::FunctionCallbackInfo<v8::Value> & info) {
  v8::Isolate * isolate = info.GetIsolate();
  RoaringBitmap64 * self = RoaringBitmap64_unwrapForMutation(isolate, info.This());
  if (self == nullptr) return;
  RoaringBitmap64 * other =
    RoaringBitmap64_main_internal::unwrapOther(isolate, info, "RoaringBitmap64.copyFrom");
  if (other == nullptr) return;
  if (other == self) {
    self->invalidate();
    info.GetReturnValue().Set(info.This());
    return;
  }
  roaring64_bitmap_t * copy = roaring64_bitmap_copy(other->bitmap);
  if (copy == nullptr) {
    return v8utils::throwError(isolate, "RoaringBitmap64.copyFrom: allocation failed");
  }
  self->replaceBitmapInstance(isolate, copy);
  info.GetReturnValue().Set(info.This());
}

// ---- Dispose ----

inline void RoaringBitmap64_dispose(const v8::FunctionCallbackInfo<v8::Value> & info) {
  v8::Isolate * isolate = info.GetIsolate();
  RoaringBitmap64 * self = ObjectWrap::TryUnwrap<RoaringBitmap64>(info.This(), isolate);
  if (self == nullptr) return;
  if (self->disposed) return;
  self->disposed = true;
  if (self->bitmap != nullptr) {
    roaring64_bitmap_free(self->bitmap);
    self->bitmap = nullptr;
  }
  self->invalidate();
}

// ---- Optimization / introspection ----

inline void RoaringBitmap64_runOptimize(const v8::FunctionCallbackInfo<v8::Value> & info) {
  v8::Isolate * isolate = info.GetIsolate();
  RoaringBitmap64 * self = RoaringBitmap64_unwrapForMutation(isolate, info.This());
  if (self == nullptr) return;
  bool changed = roaring64_bitmap_run_optimize(self->bitmap);
  // Only bump the iterator-version guard when container layout actually
  // changed. When changed==false, no in-flight iterator is invalidated, so
  // forcing them to throw "mutated" would be a false positive.
  if (changed) self->invalidate();
  info.GetReturnValue().Set(v8::Boolean::New(isolate, changed));
}

inline void RoaringBitmap64_removeRunCompression(const v8::FunctionCallbackInfo<v8::Value> & info) {
  v8::Isolate * isolate = info.GetIsolate();
  RoaringBitmap64 * self = RoaringBitmap64_unwrapForMutation(isolate, info.This());
  if (self == nullptr) return;

  // CRoaring 64-bit has no roaring64_bitmap_remove_run_compression. Detect
  // run containers via statistics; if any exist, materialize the values and
  // rebuild the bitmap (clear + add_many). add_many produces only
  // array/bitset containers, so the rebuilt bitmap has runContainers == 0.
  roaring64_statistics_t st;
  roaring64_bitmap_statistics(self->bitmap, &st);
  if (st.n_run_containers == 0) {
    info.GetReturnValue().Set(false);
    return;
  }

  uint64_t card = roaring64_bitmap_get_cardinality(self->bitmap);
  std::vector<uint64_t> values;
  if (card > 0) {
    if (card > (uint64_t)(SIZE_MAX / sizeof(uint64_t))) {
      return v8utils::throwError(
        isolate, "RoaringBitmap64.removeRunCompression: cardinality exceeds size_t limit");
    }
    values.resize((size_t)card);
    roaring64_bitmap_to_uint64_array(self->bitmap, values.data());
  }
  roaring64_bitmap_clear(self->bitmap);
  if (!values.empty()) {
    roaring64_bitmap_add_many(self->bitmap, values.size(), values.data());
  }
  self->invalidate();
  info.GetReturnValue().Set(true);
}

inline void RoaringBitmap64_shrinkToFit(const v8::FunctionCallbackInfo<v8::Value> & info) {
  v8::Isolate * isolate = info.GetIsolate();
  RoaringBitmap64 * self = RoaringBitmap64_unwrapForMutation(isolate, info.This());
  if (self == nullptr) return;
  size_t saved = roaring64_bitmap_shrink_to_fit(self->bitmap);
  // Conservatively bump _version when bytes were saved: shrink can
  // reallocate container backing stores, and our iterators read those
  // pointers directly. Matches the runOptimize "invalidate iff layout
  // moved" rule.
  if (saved > 0) self->invalidate();
  info.GetReturnValue().Set(roaring_node_bigint::makeUint64BigInt(isolate, (uint64_t)saved));
}

inline void RoaringBitmap64_statistics(const v8::FunctionCallbackInfo<v8::Value> & info) {
  v8::Isolate * isolate = info.GetIsolate();
  v8::Local<v8::Context> context = isolate->GetCurrentContext();
  const RoaringBitmap64 * self = ObjectWrap::TryUnwrap<const RoaringBitmap64>(info.This(), isolate);
  if (self == nullptr || self->disposed) {
    return v8utils::throwError(isolate, "RoaringBitmap64 is disposed");
  }

  roaring64_statistics_t st;
  roaring64_bitmap_statistics(self->bitmap, &st);

  v8::Local<v8::Object> obj = v8::Object::New(isolate);

#define SET_NUM(NAME, FIELD)                                                                       \
  ignoreMaybeResult(obj->Set(                                                                      \
    context,                                                                                       \
    NEW_LITERAL_V8_STRING(isolate, NAME, v8::NewStringType::kInternalized),                        \
    v8::Number::New(isolate, static_cast<double>(st.FIELD))))

  SET_NUM("containers", n_containers);
  SET_NUM("arrayContainers", n_array_containers);
  SET_NUM("runContainers", n_run_containers);
  SET_NUM("bitsetContainers", n_bitset_containers);
  SET_NUM("valuesInArrayContainers", n_values_array_containers);
  SET_NUM("valuesInRunContainers", n_values_run_containers);
  SET_NUM("valuesInBitsetContainers", n_values_bitset_containers);
  SET_NUM("bytesInArrayContainers", n_bytes_array_containers);
  SET_NUM("bytesInRunContainers", n_bytes_run_containers);
  SET_NUM("bytesInBitsetContainers", n_bytes_bitset_containers);

#undef SET_NUM

  ignoreMaybeResult(obj->Set(
    context,
    NEW_LITERAL_V8_STRING(isolate, "size", v8::NewStringType::kInternalized),
    roaring_node_bigint::makeUint64BigInt(isolate, st.cardinality)));

  if (st.cardinality == 0) {
    ignoreMaybeResult(obj->Set(
      context,
      NEW_LITERAL_V8_STRING(isolate, "minValue", v8::NewStringType::kInternalized),
      v8::Undefined(isolate)));
    ignoreMaybeResult(obj->Set(
      context,
      NEW_LITERAL_V8_STRING(isolate, "maxValue", v8::NewStringType::kInternalized),
      v8::Undefined(isolate)));
  } else {
    ignoreMaybeResult(obj->Set(
      context,
      NEW_LITERAL_V8_STRING(isolate, "minValue", v8::NewStringType::kInternalized),
      roaring_node_bigint::makeUint64BigInt(isolate, st.min_value)));
    ignoreMaybeResult(obj->Set(
      context,
      NEW_LITERAL_V8_STRING(isolate, "maxValue", v8::NewStringType::kInternalized),
      roaring_node_bigint::makeUint64BigInt(isolate, st.max_value)));
  }

  info.GetReturnValue().Set(obj);
}

inline void RoaringBitmap64_internalValidate(const v8::FunctionCallbackInfo<v8::Value> & info) {
  v8::Isolate * isolate = info.GetIsolate();
  const RoaringBitmap64 * self = ObjectWrap::TryUnwrap<const RoaringBitmap64>(info.This(), isolate);
  if (self == nullptr || self->disposed) {
    return v8utils::throwError(isolate, "RoaringBitmap64 is disposed");
  }
  const char * reason = nullptr;
  bool ok = roaring64_bitmap_internal_validate(self->bitmap, &reason);
  if (!ok) {
    const char * msg = reason ? reason : "RoaringBitmap64 internal validation failed";
    return v8utils::throwError(isolate, msg);
  }
}

// ---- Symbol.iterator ----

inline void RoaringBitmap64_SymbolIterator(const v8::FunctionCallbackInfo<v8::Value> & info) {
  v8::Isolate * isolate = info.GetIsolate();
  AddonData * addonData = AddonData::get(info);
  if (addonData == nullptr) return v8utils::throwError(isolate, ERROR_INVALID_OBJECT);
  auto cons = addonData->RoaringBitmap64Iterator_constructor.Get(isolate);
  v8::Local<v8::Value> argv[1] = {info.This()};
  v8::Local<v8::Object> obj;
  if (cons->NewInstance(isolate->GetCurrentContext(), 1, argv).ToLocal(&obj)) {
    info.GetReturnValue().Set(obj);
  }
}

inline void RoaringBitmap64_reverseIterator(const v8::FunctionCallbackInfo<v8::Value> & info) {
  v8::Isolate * isolate = info.GetIsolate();
  AddonData * addonData = AddonData::get(info);
  if (addonData == nullptr) return v8utils::throwError(isolate, ERROR_INVALID_OBJECT);
  auto cons = addonData->RoaringBitmap64ReverseIterator_constructor.Get(isolate);
  v8::Local<v8::Value> argv[1] = {info.This()};
  v8::Local<v8::Object> obj;
  if (cons->NewInstance(isolate->GetCurrentContext(), 1, argv).ToLocal(&obj)) {
    info.GetReturnValue().Set(obj);
  }
}

// ---- Diagnostics ----

inline void RoaringBitmap64_getInstanceCountStatic(const v8::FunctionCallbackInfo<v8::Value> & info) {
  AddonData * addonData = AddonData::get(info);
  info.GetReturnValue().Set(addonData ? (double)(addonData->RoaringBitmap64_instances) : 0.0);
}

// ---- Section F: ergonomic statics ----

inline void RoaringBitmap64_ofStatic(const v8::FunctionCallbackInfo<v8::Value> & info) {
  v8::Isolate * isolate = info.GetIsolate();
  AddonData * addonData = AddonData::get(info);
  if (addonData == nullptr) return v8utils::throwError(isolate, ERROR_INVALID_OBJECT);

  v8::Local<v8::Function> cons = addonData->RoaringBitmap64_constructor.Get(isolate);
  v8::Local<v8::Object> newObj;
  v8::Local<v8::Value> argv[1] = {v8::Undefined(isolate)};
  if (!cons->NewInstance(isolate->GetCurrentContext(), 1, argv).ToLocal(&newObj)) return;

  RoaringBitmap64 * self = ObjectWrap::TryUnwrap<RoaringBitmap64>(newObj, isolate);
  if (self == nullptr || !self->bitmap) {
    return v8utils::throwError(isolate, ERROR_INVALID_OBJECT);
  }

  int len = info.Length();
  for (int i = 0; i < len; ++i) {
    uint64_t v;
    char nameBuf[24];
    std::snprintf(nameBuf, sizeof(nameBuf), "of[%d]", i);
    if (!roaring_node_bigint::readUint64BigInt(isolate, info[i], &v, nameBuf)) {
      return;  // readUint64BigInt has already thrown
    }
    roaring64_bitmap_add(self->bitmap, v);
  }
  if (len > 0) self->invalidate();
  info.GetReturnValue().Set(newObj);
}

inline void RoaringBitmap64_swapStatic(const v8::FunctionCallbackInfo<v8::Value> & info) {
  v8::Isolate * isolate = info.GetIsolate();
  if (info.Length() < 2) {
    return v8utils::throwTypeError(isolate, "RoaringBitmap64.swap expects 2 arguments");
  }
  RoaringBitmap64 * a = ObjectWrap::TryUnwrap<RoaringBitmap64>(info[0], isolate);
  if (a == nullptr) {
    return v8utils::throwTypeError(
      isolate, "RoaringBitmap64.swap first argument must be a RoaringBitmap64");
  }
  RoaringBitmap64 * b = ObjectWrap::TryUnwrap<RoaringBitmap64>(info[1], isolate);
  if (b == nullptr) {
    return v8utils::throwTypeError(
      isolate, "RoaringBitmap64.swap second argument must be a RoaringBitmap64");
  }
  if (a->isFrozen() || b->isFrozen()) {
    return v8utils::throwError(isolate, ERROR_FROZEN);
  }
  if (a == b) return;

  roaring64_bitmap_t * tmpBitmap = a->bitmap;
  int64_t tmpSize = a->sizeCache;
  a->bitmap = b->bitmap;
  a->sizeCache = b->sizeCache;
  b->bitmap = tmpBitmap;
  b->sizeCache = tmpSize;

  a->invalidate();
  b->invalidate();
}

inline void RoaringBitmap64_fromArrayStaticAsync(const v8::FunctionCallbackInfo<v8::Value> & info) {
  v8::Isolate * isolate = info.GetIsolate();
  AddonData * addonData = AddonData::get(info);
  if (addonData == nullptr) return v8utils::throwError(isolate, ERROR_INVALID_OBJECT);

  v8::Local<v8::Value> firstArg;
  if (info.Length() >= 1) firstArg = info[0];

  if (
    !firstArg.IsEmpty() && !firstArg->IsNullOrUndefined() &&
    !firstArg->IsFunction() && firstArg->IsObject() &&
    addonData->RoaringBitmap64_constructorTemplate.Get(isolate)->HasInstance(firstArg)) {
    return v8utils::throwTypeError(
      isolate,
      "RoaringBitmap64.fromArrayAsync cannot be called with a RoaringBitmap64 instance; use .clone() instead");
  }

  auto * worker = new RB64FromArrayAsyncWorker(isolate, addonData);
  if (worker == nullptr) {
    return v8utils::throwError(isolate, "Failed to allocate async worker");
  }

  if (info.Length() >= 2 && info[1]->IsFunction()) {
    worker->setCallback(info[1]);
  } else if (info.Length() >= 1 && info[0]->IsFunction()) {
    worker->setCallback(info[0]);
  }

  // Extract values synchronously on the main thread (V8 BigInt access is
  // main-thread only). drainIterable throws V8 exceptions on bad elements;
  // we catch them here and return a rejected Promise (or invoke the
  // callback with the error) to match the documented async contract.
  if (!firstArg.IsEmpty() && !firstArg->IsNullOrUndefined() && !firstArg->IsFunction()) {
    v8::TryCatch tryCatch(isolate);
    bool ok = worker->extractValues(firstArg);
    if (!ok && tryCatch.HasCaught()) {
      v8::Local<v8::Value> exc = tryCatch.Exception();
      tryCatch.Reset();
      auto context = isolate->GetCurrentContext();
      v8::MaybeLocal<v8::Promise::Resolver> resolverMaybe = v8::Promise::Resolver::New(context);
      delete worker;
      if (resolverMaybe.IsEmpty()) {
        isolate->ThrowException(exc);
        return;
      }
      v8::Local<v8::Promise::Resolver> resolver = resolverMaybe.ToLocalChecked();
      // If a callback was provided, invoke it node-style; else reject the Promise.
      // We have already consumed the callback into the worker, but the worker
      // is being deleted — so re-read from info.
      v8::Local<v8::Function> cb;
      if (info.Length() >= 2 && info[1]->IsFunction()) {
        cb = info[1].As<v8::Function>();
      } else if (info.Length() >= 1 && info[0]->IsFunction()) {
        cb = info[0].As<v8::Function>();
      }
      if (!cb.IsEmpty()) {
        v8::Local<v8::Value> argv[] = {exc, v8::Undefined(isolate)};
        ignoreMaybeResult(cb->Call(context, context->Global(), 2, argv));
        return;
      }
      ignoreMaybeResult(resolver->Reject(context, exc));
      info.GetReturnValue().Set(resolver->GetPromise());
      return;
    }
  }

  v8::Local<v8::Value> returnValue = AsyncWorker::run(worker);
  info.GetReturnValue().Set(returnValue);
}

// ---- Init ----

inline void RoaringBitmap64_Init(v8::Local<v8::Object> exports, AddonData * addonData) {
  v8::Isolate * isolate = addonData->isolate;
  v8::Local<v8::Context> context = isolate->GetCurrentContext();

  v8::Local<v8::String> className = addonData->strings.RoaringBitmap64.Get(isolate);

  v8::Local<v8::FunctionTemplate> ctor =
    v8::FunctionTemplate::New(isolate, RoaringBitmap64_New, addonData->external.Get(isolate));
  if (ctor.IsEmpty()) return;
  addonData->RoaringBitmap64_constructorTemplate.Reset(isolate, ctor);

  v8::Local<v8::ObjectTemplate> ctorInstanceTemplate = ctor->InstanceTemplate();
  ctor->SetClassName(className);
  ctorInstanceTemplate->SetInternalFieldCount(2);

  ctor->PrototypeTemplate()->Set(v8::Symbol::GetToStringTag(isolate), className);

  ctorInstanceTemplate->SetNativeDataProperty(
    NEW_LITERAL_V8_STRING(isolate, "size", v8::NewStringType::kInternalized),
    RoaringBitmap64_size_getter,
    nullptr,
    v8::Local<v8::Value>(),
    (v8::PropertyAttribute)(v8::ReadOnly),
    v8::SideEffectType::kHasNoSideEffect);

  ctorInstanceTemplate->SetNativeDataProperty(
    NEW_LITERAL_V8_STRING(isolate, "isEmpty", v8::NewStringType::kInternalized),
    RoaringBitmap64_isEmpty_getter,
    nullptr,
    v8::Local<v8::Value>(),
    (v8::PropertyAttribute)(v8::ReadOnly),
    v8::SideEffectType::kHasNoSideEffect);

  ctorInstanceTemplate->SetNativeDataProperty(
    NEW_LITERAL_V8_STRING(isolate, "isDisposed", v8::NewStringType::kInternalized),
    RoaringBitmap64_isDisposed_getter,
    nullptr,
    v8::Local<v8::Value>(),
    (v8::PropertyAttribute)(v8::ReadOnly),
    v8::SideEffectType::kHasNoSideEffect);

  ctorInstanceTemplate->SetNativeDataProperty(
    NEW_LITERAL_V8_STRING(isolate, "isFrozen", v8::NewStringType::kInternalized),
    RoaringBitmap64_isFrozen_getter,
    nullptr,
    v8::Local<v8::Value>(),
    (v8::PropertyAttribute)(v8::ReadOnly),
    v8::SideEffectType::kHasNoSideEffect);

  // Element ops
  NODE_SET_PROTOTYPE_METHOD(ctor, "add", RoaringBitmap64_add);
  NODE_SET_PROTOTYPE_METHOD(ctor, "tryAdd", RoaringBitmap64_tryAdd);
  NODE_SET_PROTOTYPE_METHOD(ctor, "remove", RoaringBitmap64_remove);
  NODE_SET_PROTOTYPE_METHOD(ctor, "delete", RoaringBitmap64_delete);
  NODE_SET_PROTOTYPE_METHOD(ctor, "has", RoaringBitmap64_has);
  NODE_SET_PROTOTYPE_METHOD(ctor, "contains", RoaringBitmap64_has);
  NODE_SET_PROTOTYPE_METHOD(ctor, "includes", RoaringBitmap64_has);
  NODE_SET_PROTOTYPE_METHOD(ctor, "clear", RoaringBitmap64_clear);

  // Min/max/clone
  NODE_SET_PROTOTYPE_METHOD(ctor, "minimum", RoaringBitmap64_minimum);
  NODE_SET_PROTOTYPE_METHOD(ctor, "maximum", RoaringBitmap64_maximum);
  NODE_SET_PROTOTYPE_METHOD(ctor, "clone", RoaringBitmap64_clone);
  NODE_SET_PROTOTYPE_METHOD(ctor, "copyFrom", RoaringBitmap64_copyFrom);

  // Comparisons
  NODE_SET_PROTOTYPE_METHOD(ctor, "equals", RoaringBitmap64_equals);
  NODE_SET_PROTOTYPE_METHOD(ctor, "isEqual", RoaringBitmap64_equals);
  NODE_SET_PROTOTYPE_METHOD(ctor, "isSubset", RoaringBitmap64_isSubset);
  NODE_SET_PROTOTYPE_METHOD(ctor, "isStrictSubset", RoaringBitmap64_isStrictSubset);
  NODE_SET_PROTOTYPE_METHOD(ctor, "intersects", RoaringBitmap64_intersects);
  NODE_SET_PROTOTYPE_METHOD(ctor, "isSuperset", RoaringBitmap64_isSuperset);
  NODE_SET_PROTOTYPE_METHOD(ctor, "isStrictSuperset", RoaringBitmap64_isStrictSuperset);

  // Bulk
  NODE_SET_PROTOTYPE_METHOD(ctor, "addMany", RoaringBitmap64_addMany);
  NODE_SET_PROTOTYPE_METHOD(ctor, "removeMany", RoaringBitmap64_removeMany);
  NODE_SET_PROTOTYPE_METHOD(ctor, "toUint64Array", RoaringBitmap64_toUint64Array);
  NODE_SET_PROTOTYPE_METHOD(ctor, "toArray", RoaringBitmap64_toArray);

  // In-place set ops
  NODE_SET_PROTOTYPE_METHOD(ctor, "andInPlace", RoaringBitmap64_andInPlace);
  NODE_SET_PROTOTYPE_METHOD(ctor, "orInPlace", RoaringBitmap64_orInPlace);
  NODE_SET_PROTOTYPE_METHOD(ctor, "xorInPlace", RoaringBitmap64_xorInPlace);
  NODE_SET_PROTOTYPE_METHOD(ctor, "andNotInPlace", RoaringBitmap64_andNotInPlace);

  // Serialization
  NODE_SET_PROTOTYPE_METHOD(ctor, "serialize", RoaringBitmap64_serialize);
  NODE_SET_PROTOTYPE_METHOD(ctor, "getSerializationSizeInBytes", RoaringBitmap64_getSerializationSizeInBytes);
  NODE_SET_PROTOTYPE_METHOD(ctor, "deserialize", RoaringBitmap64_deserializeInstance);
  // serializeFileAsync needs addonData via FunctionCallbackInfo::Data() — wire it
  // through a FunctionTemplate with the external. NODE_SET_PROTOTYPE_METHOD does
  // not pass external Data, which would leave AsyncWorker without addonData.
  {
    v8::Local<v8::FunctionTemplate> sfaTpl =
      v8::FunctionTemplate::New(isolate, RoaringBitmap64_serializeFileAsync, addonData->external.Get(isolate));
    ctor->PrototypeTemplate()->Set(
      NEW_LITERAL_V8_STRING(isolate, "serializeFileAsync", v8::NewStringType::kInternalized),
      sfaTpl);
  }

  // Range ops
  NODE_SET_PROTOTYPE_METHOD(ctor, "addRange", RoaringBitmap64_addRange);
  NODE_SET_PROTOTYPE_METHOD(ctor, "removeRange", RoaringBitmap64_removeRange);
  NODE_SET_PROTOTYPE_METHOD(ctor, "rangeCardinality", RoaringBitmap64_rangeCardinality);
  NODE_SET_PROTOTYPE_METHOD(ctor, "intersectsWithRange", RoaringBitmap64_intersectsWithRange);
  NODE_SET_PROTOTYPE_METHOD(ctor, "hasRange", RoaringBitmap64_hasRange);
  NODE_SET_PROTOTYPE_METHOD(ctor, "containsRange", RoaringBitmap64_hasRange);
  NODE_SET_PROTOTYPE_METHOD(ctor, "flipRange", RoaringBitmap64_flipRange);

  // Optimization / introspection
  NODE_SET_PROTOTYPE_METHOD(ctor, "runOptimize", RoaringBitmap64_runOptimize);
  NODE_SET_PROTOTYPE_METHOD(ctor, "removeRunCompression", RoaringBitmap64_removeRunCompression);
  NODE_SET_PROTOTYPE_METHOD(ctor, "shrinkToFit", RoaringBitmap64_shrinkToFit);
  NODE_SET_PROTOTYPE_METHOD(ctor, "statistics", RoaringBitmap64_statistics);
  NODE_SET_PROTOTYPE_METHOD(ctor, "internalValidate", RoaringBitmap64_internalValidate);

  // Dispose
  NODE_SET_PROTOTYPE_METHOD(ctor, "dispose", RoaringBitmap64_dispose);

  // Symbol.iterator — pass addonData as Data so the callback can resolve it.
  {
    v8::Local<v8::FunctionTemplate> iterTpl =
      v8::FunctionTemplate::New(isolate, RoaringBitmap64_SymbolIterator, addonData->external.Get(isolate));
    ctor->PrototypeTemplate()->Set(v8::Symbol::GetIterator(isolate), iterTpl);
  }

  // reverseIterator — same pattern as Symbol.iterator: needs addonData as Data
  // because NODE_SET_PROTOTYPE_METHOD does not pass external data.
  {
    v8::Local<v8::FunctionTemplate> revTpl =
      v8::FunctionTemplate::New(isolate, RoaringBitmap64_reverseIterator, addonData->external.Get(isolate));
    ctor->PrototypeTemplate()->Set(
      NEW_LITERAL_V8_STRING(isolate, "reverseIterator", v8::NewStringType::kInternalized),
      revTpl);
  }

  auto ctorFunction = ctor->GetFunction(context).ToLocalChecked();
  auto ctorObject = ctorFunction->ToObject(context).ToLocalChecked();

  // Static set ops
  addonData->setMethod(ctorObject, "and", RoaringBitmap64_andStatic);
  addonData->setMethod(ctorObject, "or", RoaringBitmap64_orStatic);
  addonData->setMethod(ctorObject, "xor", RoaringBitmap64_xorStatic);
  addonData->setMethod(ctorObject, "andNot", RoaringBitmap64_andNotStatic);
  addonData->setMethod(ctorObject, "andCardinality", RoaringBitmap64_andCardinalityStatic);
  addonData->setMethod(ctorObject, "orCardinality", RoaringBitmap64_orCardinalityStatic);
  addonData->setMethod(ctorObject, "xorCardinality", RoaringBitmap64_xorCardinalityStatic);
  addonData->setMethod(ctorObject, "andNotCardinality", RoaringBitmap64_andNotCardinalityStatic);
  addonData->setMethod(ctorObject, "jaccardIndex", RoaringBitmap64_jaccardIndexStatic);
  addonData->setMethod(ctorObject, "orMany", RoaringBitmap64_orManyStatic);
  addonData->setMethod(ctorObject, "andMany", RoaringBitmap64_andManyStatic);
  addonData->setMethod(ctorObject, "xorMany", RoaringBitmap64_xorManyStatic);
  addonData->setMethod(ctorObject, "fromRoaring32", RoaringBitmap64_fromRoaring32Static);
  addonData->setMethod(ctorObject, "getInstancesCount", RoaringBitmap64_getInstanceCountStatic);

  // Section F: ergonomic statics. `from` is a literal alias to the
  // constructor (RB32 does the same — see RoaringBitmap32-main.h).
  ignoreMaybeResult(ctorObject->Set(
    context,
    NEW_LITERAL_V8_STRING(isolate, "from", v8::NewStringType::kInternalized),
    ctorFunction));
  addonData->setMethod(ctorObject, "of", RoaringBitmap64_ofStatic);
  addonData->setMethod(ctorObject, "fromRange", RoaringBitmap64_fromRangeStatic);
  addonData->setMethod(ctorObject, "addOffset", RoaringBitmap64_addOffsetStatic);
  addonData->setMethod(ctorObject, "swap", RoaringBitmap64_swapStatic);
  addonData->setMethod(ctorObject, "fromArrayAsync", RoaringBitmap64_fromArrayStaticAsync);

  // Static deserialize
  addonData->setMethod(ctorObject, "deserialize", RoaringBitmap64_deserializeStatic);
  addonData->setMethod(ctorObject, "getDeserializationSize", RoaringBitmap64_getDeserializationSizeStatic);

  // Frozen-view static
  addonData->setMethod(ctorObject, "unsafeFrozenView", RoaringBitmap64_unsafeFrozenViewStatic);

  // Async file I/O
  addonData->setMethod(ctorObject, "deserializeFileAsync", RoaringBitmap64_deserializeFileAsyncStatic);

  ignoreMaybeResult(exports->Set(context, className, ctorFunction));
  addonData->RoaringBitmap64_constructor.Reset(isolate, ctorFunction);
}

#endif  // ROARING_NODE_ROARINGBITMAP64_MAIN_H_
