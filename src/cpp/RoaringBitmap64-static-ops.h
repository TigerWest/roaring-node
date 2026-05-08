#ifndef ROARING_NODE_ROARINGBITMAP64_STATIC_OPS_H_
#define ROARING_NODE_ROARINGBITMAP64_STATIC_OPS_H_

#include <vector>

#include "RoaringBitmap32.h"
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
  if (raw == nullptr) {
    v8utils::throwError(isolate, "RoaringBitmap64 set operation: allocation failed");
    return;
  }
  auto cons = addonData->RoaringBitmap64_constructor.Get(isolate);
  v8::Local<v8::Object> newObj;
  v8::Local<v8::Value> argv[1] = {v8::Undefined(isolate)};
  if (!cons->NewInstance(isolate->GetCurrentContext(), 1, argv).ToLocal(&newObj)) {
    roaring64_bitmap_free(raw);
    return;
  }
  RoaringBitmap64 * other = ObjectWrap::TryUnwrap<RoaringBitmap64>(newObj, isolate);
  if (other == nullptr) {
    roaring64_bitmap_free(raw);
    v8utils::throwError(isolate, "RoaringBitmap64 set operation: failed to unwrap result");
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

namespace RoaringBitmap64_static_internal {

// Validate the input array of RoaringBitmap64 instances. On success appends
// borrowed pointers to `out` and returns true (out.size() == 0 is a legal
// "empty input" result). On error throws and returns false; out is unspecified.
inline bool unwrapMany(
  v8::Isolate * isolate,
  v8::Local<v8::Value> arg,
  const char * methodName,
  std::vector<const RoaringBitmap64 *> & out) {
  if (!arg->IsArray()) {
    auto msg = std::string(methodName) + " expects an array of RoaringBitmap64";
    isolate->ThrowException(v8::Exception::TypeError(
      v8::String::NewFromUtf8(isolate, msg.c_str(), v8::NewStringType::kNormal).ToLocalChecked()));
    return false;
  }
  v8::Local<v8::Context> context = isolate->GetCurrentContext();
  v8::Local<v8::Array> arr = arg.As<v8::Array>();
  uint32_t len = arr->Length();
  out.clear();
  out.reserve(len);
  for (uint32_t i = 0; i < len; ++i) {
    v8::Local<v8::Value> el;
    if (!arr->Get(context, i).ToLocal(&el)) return false;
    const RoaringBitmap64 * b = ObjectWrap::TryUnwrap<const RoaringBitmap64>(el, isolate);
    if (b == nullptr) {
      auto msg = std::string(methodName) + ": element at index " + std::to_string(i) +
                 " is not a RoaringBitmap64";
      isolate->ThrowException(v8::Exception::TypeError(
        v8::String::NewFromUtf8(isolate, msg.c_str(), v8::NewStringType::kNormal).ToLocalChecked()));
      return false;
    }
    if (b->disposed) {
      auto msg = std::string(methodName) + ": element at index " + std::to_string(i) +
                 " is a disposed RoaringBitmap64";
      v8utils::throwError(isolate, msg.c_str());
      return false;
    }
    out.push_back(b);
  }
  return true;
}

}  // namespace RoaringBitmap64_static_internal

inline void RoaringBitmap64_orManyStatic(const v8::FunctionCallbackInfo<v8::Value> & info) {
  v8::Isolate * isolate = info.GetIsolate();
  AddonData * addonData = AddonData::get(info);
  if (addonData == nullptr) return v8utils::throwError(isolate, ERROR_INVALID_OBJECT);
  if (info.Length() < 1) {
    return v8utils::throwTypeError(isolate, "RoaringBitmap64.orMany expects an array argument");
  }
  std::vector<const RoaringBitmap64 *> items;
  if (!RoaringBitmap64_static_internal::unwrapMany(isolate, info[0], "RoaringBitmap64.orMany", items)) return;

  roaring64_bitmap_t * result = nullptr;
  if (items.empty()) {
    result = roaring64_bitmap_create();
  } else {
    result = roaring64_bitmap_copy(items[0]->bitmap);
    if (result != nullptr) {
      for (size_t i = 1; i < items.size(); ++i) {
        roaring64_bitmap_or_inplace(result, items[i]->bitmap);
      }
    }
  }
  RoaringBitmap64_static_internal::returnNewBitmap(isolate, addonData, info, result);
}

inline void RoaringBitmap64_andManyStatic(const v8::FunctionCallbackInfo<v8::Value> & info) {
  v8::Isolate * isolate = info.GetIsolate();
  AddonData * addonData = AddonData::get(info);
  if (addonData == nullptr) return v8utils::throwError(isolate, ERROR_INVALID_OBJECT);
  if (info.Length() < 1) {
    return v8utils::throwTypeError(isolate, "RoaringBitmap64.andMany expects an array argument");
  }
  std::vector<const RoaringBitmap64 *> items;
  if (!RoaringBitmap64_static_internal::unwrapMany(isolate, info[0], "RoaringBitmap64.andMany", items)) return;

  roaring64_bitmap_t * result = nullptr;
  if (items.empty()) {
    result = roaring64_bitmap_create();
  } else {
    result = roaring64_bitmap_copy(items[0]->bitmap);
    if (result != nullptr) {
      for (size_t i = 1; i < items.size(); ++i) {
        roaring64_bitmap_and_inplace(result, items[i]->bitmap);
      }
    }
  }
  RoaringBitmap64_static_internal::returnNewBitmap(isolate, addonData, info, result);
}

inline void RoaringBitmap64_fromRoaring32Static(const v8::FunctionCallbackInfo<v8::Value> & info) {
  v8::Isolate * isolate = info.GetIsolate();
  AddonData * addonData = AddonData::get(info);
  if (addonData == nullptr) return v8utils::throwError(isolate, ERROR_INVALID_OBJECT);
  if (info.Length() < 1) {
    return v8utils::throwTypeError(
      isolate, "RoaringBitmap64.fromRoaring32 expects a RoaringBitmap32 argument");
  }
  const RoaringBitmap32 * src = ObjectWrap::TryUnwrap<const RoaringBitmap32>(info[0], isolate);
  if (src == nullptr || src->roaring == nullptr) {
    return v8utils::throwTypeError(
      isolate, "RoaringBitmap64.fromRoaring32 argument must be a non-disposed RoaringBitmap32");
  }

  roaring64_bitmap_t * result = roaring64_bitmap_create();
  if (result == nullptr) {
    return v8utils::throwError(isolate, "RoaringBitmap64.fromRoaring32: allocation failed");
  }

  uint64_t card = roaring_bitmap_get_cardinality(src->roaring);
  if (card > 0) {
    // Chunked conversion: keeps peak memory bounded for very large RB32s.
    constexpr uint32_t CHUNK = 4096;
    uint32_t buf32[CHUNK];
    uint64_t buf64[CHUNK];
    roaring_uint32_iterator_t it;
    roaring_iterator_init(src->roaring, &it);
    while (it.has_value) {
      uint32_t produced = roaring_uint32_iterator_read(&it, buf32, CHUNK);
      if (produced == 0) break;
      for (uint32_t i = 0; i < produced; ++i) buf64[i] = static_cast<uint64_t>(buf32[i]);
      roaring64_bitmap_add_many(result, produced, buf64);
    }
  }

  RoaringBitmap64_static_internal::returnNewBitmap(isolate, addonData, info, result);
}

#endif  // ROARING_NODE_ROARINGBITMAP64_STATIC_OPS_H_
