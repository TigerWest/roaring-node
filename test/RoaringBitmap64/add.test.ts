import { describe, expect, it } from "vitest";
import { RoaringBitmap64 } from "../../";

describe("RoaringBitmap64.add", () => {
  it("inserts BigInt values and returns this", () => {
    const b = new RoaringBitmap64();
    expect(b.add(0n)).toBe(b);
    b.add(1n);
    b.add(2n ** 32n);
    b.add(2n ** 63n);
    b.add(2n ** 64n - 1n);
    expect(b.size).toBe(5n);
  });

  it("rejects number with TypeError", () => {
    const b = new RoaringBitmap64();
    expect(() => (b as any).add(1)).toThrow(TypeError);
  });

  it("rejects negative BigInt with RangeError", () => {
    const b = new RoaringBitmap64();
    expect(() => b.add(-1n)).toThrow(RangeError);
  });

  it("rejects BigInt >= 2^64 with RangeError", () => {
    const b = new RoaringBitmap64();
    expect(() => b.add(2n ** 64n)).toThrow(RangeError);
  });
});

describe("RoaringBitmap64.tryAdd", () => {
  it("returns true on first insert, false on duplicate", () => {
    const b = new RoaringBitmap64();
    expect(b.tryAdd(7n)).toBe(true);
    expect(b.tryAdd(7n)).toBe(false);
    expect(b.size).toBe(1n);
  });
});
