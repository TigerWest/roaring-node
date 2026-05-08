import { describe, expect, it } from "vitest";
import { RoaringBitmap64 } from "../../";

const valueMethods = ["add", "tryAdd", "remove", "delete", "has", "contains", "includes"] as const;

describe("RoaringBitmap64 BigInt validation", () => {
  it.each(valueMethods)("%s rejects number", (m) => {
    const b = new RoaringBitmap64();
    expect(() => (b as any)[m](1)).toThrow(TypeError);
  });

  it.each(valueMethods)("%s rejects negative BigInt", (m) => {
    const b = new RoaringBitmap64();
    expect(() => (b as any)[m](-1n)).toThrow(RangeError);
  });

  it.each(valueMethods)("%s rejects BigInt >= 2^64", (m) => {
    const b = new RoaringBitmap64();
    expect(() => (b as any)[m](2n ** 64n)).toThrow(RangeError);
  });

  it.each(valueMethods)("%s accepts boundary 2^64 - 1n", (m) => {
    const b = new RoaringBitmap64();
    expect(() => (b as any)[m](2n ** 64n - 1n)).not.toThrow();
  });

  it.each(valueMethods)("%s accepts 0n", (m) => {
    const b = new RoaringBitmap64();
    expect(() => (b as any)[m](0n)).not.toThrow();
  });
});
