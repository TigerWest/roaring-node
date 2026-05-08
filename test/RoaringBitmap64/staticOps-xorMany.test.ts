import { describe, expect, it } from "vitest";
import { RoaringBitmap64 } from "../../";

function bm(...vals: bigint[]): RoaringBitmap64 {
  const b = new RoaringBitmap64();
  for (const v of vals) b.add(v);
  return b;
}

describe("RoaringBitmap64.xorMany", () => {
  it("returns an empty bitmap for []", () => {
    const r = RoaringBitmap64.xorMany([]);
    expect(r).toBeInstanceOf(RoaringBitmap64);
    expect(r.size).toBe(0n);
  });

  it("returns a clone (not alias) for [a]", () => {
    const a = bm(1n, 2n, 3n);
    const r = RoaringBitmap64.xorMany([a]);
    expect(r).not.toBe(a);
    expect(r.size).toBe(3n);
    a.add(4n);
    expect(r.size).toBe(3n);
    r.add(99n);
    expect(a.has(99n)).toBe(false);
  });

  it("xorMany([a, b]) equals xor(a, b)", () => {
    const a = bm(1n, 2n, 3n);
    const b = bm(2n, 3n, 4n);
    const viaMany = RoaringBitmap64.xorMany([a, b]);
    const viaPair = RoaringBitmap64.xor(a, b);
    expect(viaMany.equals(viaPair)).toBe(true);
    expect(viaMany.toArray()).toEqual([1n, 4n]);
  });

  it("xorMany([a, b, c]) takes the parity of occurrences across all inputs", () => {
    const a = bm(1n, 2n, 3n);
    const b = bm(2n, 3n, 4n);
    const c = bm(3n, 4n, 5n);
    const r = RoaringBitmap64.xorMany([a, b, c]);
    expect(r.toArray()).toEqual([1n, 3n, 5n]);
  });

  it("works across the 2^32 boundary", () => {
    const a = bm(1n, 1n << 40n);
    const b = bm(1n, (1n << 40n) + 1n);
    const c = bm((1n << 40n) + 1n, (1n << 40n) + 2n);
    const r = RoaringBitmap64.xorMany([a, b, c]);
    expect(r.toArray()).toEqual([1n << 40n, (1n << 40n) + 2n]);
  });

  it("does not mutate input bitmaps", () => {
    const a = bm(1n, 2n);
    const b = bm(2n, 3n);
    const aBefore = a.toArray();
    const bBefore = b.toArray();
    RoaringBitmap64.xorMany([a, b]);
    expect(a.toArray()).toEqual(aBefore);
    expect(b.toArray()).toEqual(bBefore);
  });

  it("rejects non-array argument with TypeError", () => {
    expect(() => (RoaringBitmap64 as any).xorMany(null)).toThrow(TypeError);
    expect(() => (RoaringBitmap64 as any).xorMany(42)).toThrow(TypeError);
    expect(() => (RoaringBitmap64 as any).xorMany()).toThrow(TypeError);
  });

  it("rejects non-RoaringBitmap64 element with TypeError", () => {
    const a = bm(1n);
    expect(() => (RoaringBitmap64 as any).xorMany([a, {}])).toThrow(TypeError);
    expect(() => (RoaringBitmap64 as any).xorMany([a, null])).toThrow(TypeError);
  });

  it("rejects disposed element", () => {
    const a = bm(1n);
    const b = bm(2n);
    b.dispose();
    expect(() => RoaringBitmap64.xorMany([a, b])).toThrow();
  });
});
