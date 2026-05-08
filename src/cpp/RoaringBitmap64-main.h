#ifndef ROARING_NODE_ROARINGBITMAP64_MAIN_H_
#define ROARING_NODE_ROARINGBITMAP64_MAIN_H_

#include "RoaringBitmap64.h"
#include "RoaringBitmap64-bulk.h"
#include "RoaringBitmap64-ops.h"
#include "RoaringBitmap64-static-ops.h"
#include "RoaringBitmap64-serialization.h"
#include "RoaringBitmap64Iterator.h"
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
    // If the argument is itself a RoaringBitmap64 instance, skip addMany —
    // the caller (typically clone) will copy the bitmap manually after
    // construction. addMany only handles BigUint64Array / Iterable<bigint>.
    RoaringBitmap64 * sourceRB64 = ObjectWrap::TryUnwrap<RoaringBitmap64>(info[0], isolate);
    if (sourceRB64 == nullptr) {
      RoaringBitmap64_addMany(info);
    }
  }

  info.GetReturnValue().Set(holder);
}

// ---- Element ops ----

inline void RoaringBitmap64_add(const v8::FunctionCallbackInfo<v8::Value> & info) {
  v8::Isolate * isolate = info.GetIsolate();
  RoaringBitmap64 * self = ObjectWrap::TryUnwrap<RoaringBitmap64>(info.This(), isolate);
  if (self == nullptr || self->disposed) {
    return v8utils::throwError(isolate, "RoaringBitmap64 is disposed");
  }
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
  RoaringBitmap64 * self = ObjectWrap::TryUnwrap<RoaringBitmap64>(info.This(), isolate);
  if (self == nullptr || self->disposed) {
    return v8utils::throwError(isolate, "RoaringBitmap64 is disposed");
  }
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
  RoaringBitmap64 * self = ObjectWrap::TryUnwrap<RoaringBitmap64>(info.This(), isolate);
  if (self == nullptr || self->disposed) {
    return v8utils::throwError(isolate, "RoaringBitmap64 is disposed");
  }
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
  RoaringBitmap64 * self = ObjectWrap::TryUnwrap<RoaringBitmap64>(info.This(), isolate);
  if (self == nullptr || self->disposed) {
    return v8utils::throwError(isolate, "RoaringBitmap64 is disposed");
  }
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
  RoaringBitmap64 * self = ObjectWrap::TryUnwrap<RoaringBitmap64>(info.This(), isolate);
  if (self == nullptr || self->disposed) {
    return v8utils::throwError(isolate, "RoaringBitmap64 is disposed");
  }
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
  v8::Local<v8::Value> argv[1] = {info.This()};
  v8::Local<v8::Object> newObj;
  if (!cons->NewInstance(isolate->GetCurrentContext(), 1, argv).ToLocal(&newObj)) {
    return v8utils::throwError(isolate, "RoaringBitmap64.clone failed to create instance");
  }
  RoaringBitmap64 * other = ObjectWrap::TryUnwrap<RoaringBitmap64>(newObj, isolate);
  if (other == nullptr) {
    return v8utils::throwError(isolate, "RoaringBitmap64.clone failed to create instance");
  }

  if (other->bitmap) roaring64_bitmap_free(other->bitmap);
  other->bitmap = roaring64_bitmap_copy(self->bitmap);
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

  // Comparisons
  NODE_SET_PROTOTYPE_METHOD(ctor, "equals", RoaringBitmap64_equals);
  NODE_SET_PROTOTYPE_METHOD(ctor, "isSubset", RoaringBitmap64_isSubset);
  NODE_SET_PROTOTYPE_METHOD(ctor, "isStrictSubset", RoaringBitmap64_isStrictSubset);

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

  // Dispose
  NODE_SET_PROTOTYPE_METHOD(ctor, "dispose", RoaringBitmap64_dispose);

  // Symbol.iterator — pass addonData as Data so the callback can resolve it.
  {
    v8::Local<v8::FunctionTemplate> iterTpl =
      v8::FunctionTemplate::New(isolate, RoaringBitmap64_SymbolIterator, addonData->external.Get(isolate));
    ctor->PrototypeTemplate()->Set(v8::Symbol::GetIterator(isolate), iterTpl);
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

  // Static deserialize
  addonData->setMethod(ctorObject, "deserialize", RoaringBitmap64_deserializeStatic);
  addonData->setMethod(ctorObject, "getDeserializationSize", RoaringBitmap64_getDeserializationSizeStatic);

  ignoreMaybeResult(exports->Set(context, className, ctorFunction));
  addonData->RoaringBitmap64_constructor.Reset(isolate, ctorFunction);
}

#endif  // ROARING_NODE_ROARINGBITMAP64_MAIN_H_
