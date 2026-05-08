#ifndef ROARING_NODE_ROARINGBITMAP64_ITERATOR_H_
#define ROARING_NODE_ROARINGBITMAP64_ITERATOR_H_

#include "RoaringBitmap64.h"
#include "addon-data.h"

class RoaringBitmap64Iterator final : public ObjectWrap {
 public:
  static const constexpr uint64_t OBJECT_TOKEN = 0x21524F4152360010ULL;

  roaring64_iterator_t * iter;
  v8::Global<v8::Object> bitmapPersistent;
  v8::Global<v8::Object> persistent;
  bool exhausted;

  explicit RoaringBitmap64Iterator(AddonData * addonData) :
    ObjectWrap(addonData), iter(nullptr), exhausted(true) {}

  ~RoaringBitmap64Iterator() {
    if (this->iter) {
      roaring64_iterator_free(this->iter);
      this->iter = nullptr;
    }
    if (!this->persistent.IsEmpty()) this->persistent.ClearWeak();
  }
};

inline void RoaringBitmap64Iterator_WeakCallback(v8::WeakCallbackInfo<RoaringBitmap64Iterator> const & info) {
  auto * p = info.GetParameter();
  if (p) {
    p->~RoaringBitmap64Iterator();
    bare_aligned_free(p);
  }
}

inline void RoaringBitmap64Iterator_New(const v8::FunctionCallbackInfo<v8::Value> & info) {
  v8::Isolate * isolate = info.GetIsolate();
  AddonData * addonData = AddonData::get(info);
  if (addonData == nullptr) return v8utils::throwError(isolate, ERROR_INVALID_OBJECT);

  if (!info.IsConstructCall()) {
    auto cons = addonData->RoaringBitmap64Iterator_constructor.Get(isolate);
    v8::Local<v8::Value> argv[1] = {info.Length() > 0 ? info[0] : v8::Undefined(isolate).As<v8::Value>()};
    v8::MaybeLocal<v8::Object> v = cons->NewInstance(isolate->GetCurrentContext(), 1, argv);
    v8::Local<v8::Object> vlocal;
    if (v.ToLocal(&vlocal)) info.GetReturnValue().Set(vlocal);
    return;
  }

  RoaringBitmap64 * parent = info.Length() > 0
    ? ObjectWrap::TryUnwrap<RoaringBitmap64>(info[0], isolate)
    : nullptr;
  if (parent == nullptr || parent->disposed) {
    return v8utils::throwError(isolate, "RoaringBitmap64Iterator requires a non-disposed RoaringBitmap64");
  }

  auto holder = info.This();
  auto * mem = bare_aligned_malloc(32, sizeof(RoaringBitmap64Iterator));
  if (mem == nullptr) {
    return v8utils::throwError(isolate, "RoaringBitmap64Iterator: allocation failed");
  }
  auto * instance = new (mem) RoaringBitmap64Iterator(addonData);
  instance->iter = roaring64_iterator_create(parent->bitmap);
  instance->exhausted = !roaring64_iterator_has_value(instance->iter);

  int indices[2] = {0, 1};
  void * values[2] = {instance, (void *)(RoaringBitmap64Iterator::OBJECT_TOKEN)};
  holder->SetAlignedPointerInInternalFields(2, indices, values);

  instance->persistent.Reset(isolate, holder);
  instance->persistent.SetWeak(instance, RoaringBitmap64Iterator_WeakCallback, v8::WeakCallbackType::kParameter);
  instance->bitmapPersistent.Reset(isolate, info[0].As<v8::Object>());

  info.GetReturnValue().Set(holder);
}

inline void RoaringBitmap64Iterator_next(const v8::FunctionCallbackInfo<v8::Value> & info) {
  v8::Isolate * isolate = info.GetIsolate();
  auto context = isolate->GetCurrentContext();
  RoaringBitmap64Iterator * self = ObjectWrap::TryUnwrap<RoaringBitmap64Iterator>(info.This(), isolate);
  if (self == nullptr) return v8utils::throwError(isolate, "RoaringBitmap64Iterator: invalid this");

  if (!self->bitmapPersistent.IsEmpty()) {
    v8::Local<v8::Object> parentObj = self->bitmapPersistent.Get(isolate);
    RoaringBitmap64 * parent = ObjectWrap::TryUnwrap<RoaringBitmap64>(parentObj, isolate);
    if (parent == nullptr || parent->disposed) {
      return v8utils::throwError(isolate, "RoaringBitmap64Iterator: parent is disposed");
    }
  }

  v8::Local<v8::Object> result = v8::Object::New(isolate);
  auto valueKey = v8::String::NewFromUtf8Literal(isolate, "value", v8::NewStringType::kInternalized);
  auto doneKey = v8::String::NewFromUtf8Literal(isolate, "done", v8::NewStringType::kInternalized);

  if (self->exhausted || self->iter == nullptr || !roaring64_iterator_has_value(self->iter)) {
    ignoreMaybeResult(result->Set(context, valueKey, v8::Undefined(isolate)));
    ignoreMaybeResult(result->Set(context, doneKey, v8::Boolean::New(isolate, true)));
    self->exhausted = true;
    info.GetReturnValue().Set(result);
    return;
  }

  uint64_t v = roaring64_iterator_value(self->iter);
  ignoreMaybeResult(result->Set(context, valueKey, v8::BigInt::NewFromUnsigned(isolate, v)));
  ignoreMaybeResult(result->Set(context, doneKey, v8::Boolean::New(isolate, false)));
  if (!roaring64_iterator_advance(self->iter)) {
    self->exhausted = true;
  }
  info.GetReturnValue().Set(result);
}

inline void RoaringBitmap64Iterator_self(const v8::FunctionCallbackInfo<v8::Value> & info) {
  info.GetReturnValue().Set(info.This());
}

inline void RoaringBitmap64Iterator_Init(v8::Local<v8::Object> exports, AddonData * addonData) {
  v8::Isolate * isolate = addonData->isolate;
  v8::Local<v8::Context> context = isolate->GetCurrentContext();
  v8::Local<v8::String> className = addonData->strings.RoaringBitmap64Iterator.Get(isolate);

  v8::Local<v8::FunctionTemplate> ctor =
    v8::FunctionTemplate::New(isolate, RoaringBitmap64Iterator_New, addonData->external.Get(isolate));
  if (ctor.IsEmpty()) return;
  addonData->RoaringBitmap64Iterator_constructorTemplate.Reset(isolate, ctor);

  ctor->SetClassName(className);
  ctor->InstanceTemplate()->SetInternalFieldCount(2);
  ctor->PrototypeTemplate()->Set(v8::Symbol::GetToStringTag(isolate), className);

  NODE_SET_PROTOTYPE_METHOD(ctor, "next", RoaringBitmap64Iterator_next);

  ctor->PrototypeTemplate()->Set(
    v8::Symbol::GetIterator(isolate),
    v8::FunctionTemplate::New(isolate, RoaringBitmap64Iterator_self));

  auto ctorFunction = ctor->GetFunction(context).ToLocalChecked();
  ignoreMaybeResult(exports->Set(context, className, ctorFunction));
  addonData->RoaringBitmap64Iterator_constructor.Reset(isolate, ctorFunction);
}

#endif  // ROARING_NODE_ROARINGBITMAP64_ITERATOR_H_
