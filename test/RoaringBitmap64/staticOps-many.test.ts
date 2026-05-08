import { describe, expect, it } from "vitest";
import { RoaringBitmap64 } from "../../";

function bm(...vals: bigint[]): RoaringBitmap64 {
  const b = new RoaringBitmap64();
  for (const v of vals) b.add(v);
  return b;
}

describe("RoaringBitmap64.orMany", () => {
  it("returns an empty bitmap for []", () => {
    const r = RoaringBitmap64.orMany([]);
    expect(r.size).toBe(0n);
  });

  it("returns a clone (not alias) for [a]", () => {
    const a = bm(1n, 2n, 3n);
    const r = RoaringBitmap64.orMany([a]);
    expect(r).not.toBe(a);
    expect(r.size).toBe(3n);
    a.add(4n);
    expect(r.size).toBe(3n); // independent
  });

  it("unions multiple bitmaps", () => {
    const r = RoaringBitmap64.orMany([bm(1n, 2n), bm(2n, 3n), bm(3n, 4n)]);
    expect(r.size).toBe(4n);
    for (const v of [1n, 2n, 3n, 4n]) expect(r.has(v)).toBe(true);
  });

  it("rejects non-array with TypeError", () => {
    expect(() => (RoaringBitmap64 as any).orMany(null)).toThrow(TypeError);
    expect(() => (RoaringBitmap64 as any).orMany(42)).toThrow(TypeError);
  });

  it("rejects non-RoaringBitmap64 element with TypeError", () => {
    expect(() => (RoaringBitmap64 as any).orMany([bm(1n), {}])).toThrow(TypeError);
  });

  it("rejects disposed element", () => {
    const a = bm(1n);
    const b = bm(2n);
    b.dispose();
    expect(() => RoaringBitmap64.orMany([a, b])).toThrow();
  });
});

describe("RoaringBitmap64.andMany", () => {
  it("returns an empty bitmap for []", () => {
    const r = RoaringBitmap64.andMany([]);
    expect(r.size).toBe(0n);
  });

  it("intersects multiple bitmaps", () => {
    const r = RoaringBitmap64.andMany([bm(1n, 2n, 3n), bm(2n, 3n, 4n), bm(3n, 4n, 5n)]);
    expect(r.size).toBe(1n);
    expect(r.has(3n)).toBe(true);
  });

  it("returns a clone of the only element for [a]", () => {
    const a = bm(1n, 2n);
    const r = RoaringBitmap64.andMany([a]);
    expect(r).not.toBe(a);
    expect(r.size).toBe(2n);
  });
});
