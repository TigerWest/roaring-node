import { describe, expect, it } from "vitest";
import { RoaringBitmap64 } from "../../";

function bm(...vals: bigint[]): RoaringBitmap64 {
  const b = new RoaringBitmap64();
  for (const v of vals) b.add(v);
  return b;
}

describe("RoaringBitmap64.addOffset", () => {
  it("returns an empty bitmap when input is empty", () => {
    const r = RoaringBitmap64.addOffset(new RoaringBitmap64(), 100n);
    expect(r).toBeInstanceOf(RoaringBitmap64);
    expect(r.size).toBe(0n);
  });

  it("shifts every value by a positive offset", () => {
    const r = RoaringBitmap64.addOffset(bm(1n, 2n, 3n), 10n);
    expect(r.toArray()).toEqual([11n, 12n, 13n]);
  });

  it("drops underflow with a negative offset", () => {
    const r = RoaringBitmap64.addOffset(bm(0n, 1n, 5n), -2n);
    expect(r.toArray()).toEqual([3n]);
  });

  it("is a no-op for offset 0", () => {
    const a = bm(1n, 2n, 3n);
    const r = RoaringBitmap64.addOffset(a, 0n);
    expect(r.toArray()).toEqual([1n, 2n, 3n]);
    expect(r).not.toBe(a);
  });

  it("drops positive overflow at the 2^64 boundary", () => {
    const max = (1n << 64n) - 1n;
    const r = RoaringBitmap64.addOffset(bm(max - 4n, max), 4n);
    expect(r.toArray()).toEqual([max]);
  });

  it("returns a fresh independent bitmap", () => {
    const a = bm(1n, 2n);
    const r = RoaringBitmap64.addOffset(a, 5n);
    expect(r).not.toBe(a);
    a.add(3n);
    expect(r.size).toBe(2n);
    r.add(99n);
    expect(a.has(99n)).toBe(false);
  });

  it("does not mutate the input", () => {
    const a = bm(1n, 2n, 3n);
    const before = a.toArray();
    RoaringBitmap64.addOffset(a, 10n);
    expect(a.toArray()).toEqual(before);
  });

  it("works across the 2^32 boundary in either direction", () => {
    const r = RoaringBitmap64.addOffset(bm((1n << 32n) + 5n), -10n);
    expect(r.toArray()).toEqual([(1n << 32n) - 5n]);
    const r2 = RoaringBitmap64.addOffset(bm((1n << 32n) - 5n), 10n);
    expect(r2.toArray()).toEqual([(1n << 32n) + 5n]);
  });

  it("throws TypeError when input is not a RoaringBitmap64", () => {
    expect(() => (RoaringBitmap64 as any).addOffset({}, 1n)).toThrow(TypeError);
    expect(() => (RoaringBitmap64 as any).addOffset(null, 1n)).toThrow(TypeError);
  });

  it("throws TypeError when offset is not a BigInt", () => {
    expect(() => (RoaringBitmap64 as any).addOffset(bm(1n), 5)).toThrow(TypeError);
    expect(() => (RoaringBitmap64 as any).addOffset(bm(1n), "5")).toThrow(TypeError);
  });

  it("throws RangeError on offset out of int64 range", () => {
    // 2^63 is just past INT64_MAX
    expect(() => RoaringBitmap64.addOffset(bm(1n), 1n << 63n)).toThrow(RangeError);
    // -(2^63) - 1 is just past INT64_MIN
    expect(() => RoaringBitmap64.addOffset(bm(1n), -(1n << 63n) - 1n)).toThrow(RangeError);
  });

  it("handles INT64_MIN offset (drops every value via underflow)", () => {
    const r = RoaringBitmap64.addOffset(bm(0n, 1n, (1n << 62n) - 1n), -(1n << 63n));
    expect(r.size).toBe(0n);
  });
});
