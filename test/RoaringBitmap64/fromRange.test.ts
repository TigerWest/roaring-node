import { describe, expect, it } from "vitest";
import { RoaringBitmap64 } from "../../";

describe("RoaringBitmap64.fromRange", () => {
  it("returns an empty bitmap when start == end", () => {
    expect(RoaringBitmap64.fromRange(0n, 0n).size).toBe(0n);
    expect(RoaringBitmap64.fromRange(5n, 5n).size).toBe(0n);
  });

  it("returns an empty bitmap when start > end", () => {
    expect(RoaringBitmap64.fromRange(10n, 0n).size).toBe(0n);
  });

  it("populates [start, end) with default step 1", () => {
    const r = RoaringBitmap64.fromRange(0n, 10n);
    expect(r.size).toBe(10n);
    expect(r.minimum()).toBe(0n);
    expect(r.maximum()).toBe(9n);
  });

  it("respects an explicit step", () => {
    const r = RoaringBitmap64.fromRange(0n, 10n, 3n);
    expect(r.toArray()).toEqual([0n, 3n, 6n, 9n]);
  });

  it("treats step 0 as 1 (clamp)", () => {
    const r = RoaringBitmap64.fromRange(0n, 5n, 0n);
    expect(r.size).toBe(5n);
    expect(r.toArray()).toEqual([0n, 1n, 2n, 3n, 4n]);
  });

  it("crosses the 2^32 boundary", () => {
    const start = 1n << 33n;
    const r = RoaringBitmap64.fromRange(start, start + 5n);
    expect(r.size).toBe(5n);
    expect(r.has(start)).toBe(true);
    expect(r.has(start + 4n)).toBe(true);
    expect(r.has(start + 5n)).toBe(false);
  });

  it("throws TypeError for non-BigInt args", () => {
    expect(() => (RoaringBitmap64 as any).fromRange(0, 10)).toThrow(TypeError);
    expect(() => (RoaringBitmap64 as any).fromRange(0n, 10)).toThrow(TypeError);
    expect(() => (RoaringBitmap64 as any).fromRange()).toThrow(TypeError);
  });

  it("throws RangeError for negative bounds", () => {
    expect(() => RoaringBitmap64.fromRange(-1n, 10n)).toThrow(RangeError);
    expect(() => RoaringBitmap64.fromRange(0n, -1n)).toThrow(RangeError);
  });

  it("throws RangeError for negative step", () => {
    expect(() => RoaringBitmap64.fromRange(0n, 10n, -1n)).toThrow(RangeError);
  });
});
