import { describe, expect, it } from "vitest";
import { RoaringBitmap64 } from "../../";

function bm(...vals: bigint[]): RoaringBitmap64 {
  const b = new RoaringBitmap64();
  for (const v of vals) b.add(v);
  return b;
}

describe("RoaringBitmap64.from", () => {
  it("is literally the constructor function", () => {
    expect(RoaringBitmap64.from).toBe(RoaringBitmap64);
  });

  it("returns an empty bitmap when called with no arg", () => {
    const r = RoaringBitmap64.from();
    expect(r).toBeInstanceOf(RoaringBitmap64);
    expect(r.size).toBe(0n);
  });

  it("accepts a BigUint64Array", () => {
    const r = RoaringBitmap64.from(new BigUint64Array([1n, 2n, 3n]));
    expect(r.size).toBe(3n);
    for (const v of [1n, 2n, 3n]) expect(r.has(v)).toBe(true);
  });

  it("accepts an iterable of bigints", () => {
    const r = RoaringBitmap64.from([1n, 100n, 1n << 40n]);
    expect(r.size).toBe(3n);
    expect(r.has(1n << 40n)).toBe(true);
  });

  it("accepts a generator", () => {
    function* gen() {
      yield 7n;
      yield 8n;
    }
    const r = RoaringBitmap64.from(gen());
    expect(r.toArray()).toEqual([7n, 8n]);
  });

  it("rejects another RoaringBitmap64 (use clone() instead)", () => {
    expect(() => RoaringBitmap64.from(bm(1n) as any)).toThrow(TypeError);
  });
});
