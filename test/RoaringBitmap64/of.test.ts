import { describe, expect, it } from "vitest";
import { RoaringBitmap64 } from "../../";

describe("RoaringBitmap64.of", () => {
  it("returns an empty bitmap with no args", () => {
    const r = RoaringBitmap64.of();
    expect(r).toBeInstanceOf(RoaringBitmap64);
    expect(r.size).toBe(0n);
  });

  it("returns a bitmap containing the args", () => {
    const r = RoaringBitmap64.of(1n, 2n, 3n);
    expect(r.size).toBe(3n);
    for (const v of [1n, 2n, 3n]) expect(r.has(v)).toBe(true);
  });

  it("dedupes (set semantics)", () => {
    const r = RoaringBitmap64.of(1n, 1n, 2n, 2n, 2n);
    expect(r.size).toBe(2n);
    expect(r.toArray()).toEqual([1n, 2n]);
  });

  it("works across the 2^32 boundary", () => {
    const r = RoaringBitmap64.of(1n, 1n << 33n, (1n << 60n) + 1n);
    expect(r.size).toBe(3n);
    expect(r.has(1n << 33n)).toBe(true);
    expect(r.has((1n << 60n) + 1n)).toBe(true);
  });

  it("throws TypeError on non-BigInt argument", () => {
    expect(() => (RoaringBitmap64 as any).of(1n, 2)).toThrow(TypeError);
    expect(() => (RoaringBitmap64 as any).of("foo")).toThrow(TypeError);
    expect(() => (RoaringBitmap64 as any).of(null)).toThrow(TypeError);
  });

  it("rejects negative BigInt with RangeError", () => {
    expect(() => RoaringBitmap64.of(-1n)).toThrow(RangeError);
  });

  it("rejects values >= 2^64 with RangeError", () => {
    expect(() => RoaringBitmap64.of(1n << 64n)).toThrow(RangeError);
  });
});
