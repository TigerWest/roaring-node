import { describe, expect, it } from "vitest";
import { RoaringBitmap64 } from "../../";

describe("RoaringBitmap64.removeMany", () => {
  it("removes values from BigUint64Array", () => {
    const b = new RoaringBitmap64();
    b.addMany([1n, 2n, 3n, 4n, 5n]);
    b.removeMany(new BigUint64Array([2n, 4n]));
    expect(b.toArray()).toEqual([1n, 3n, 5n]);
  });

  it("removes values from Iterable<bigint>", () => {
    const b = new RoaringBitmap64();
    b.addMany([1n, 2n, 3n, 4n, 5n]);
    b.removeMany([2n, 4n]);
    expect(b.toArray()).toEqual([1n, 3n, 5n]);
  });

  it("ignores values not in the bitmap", () => {
    const b = new RoaringBitmap64();
    b.addMany([1n, 2n]);
    b.removeMany([5n, 6n]);
    expect(b.toArray()).toEqual([1n, 2n]);
  });

  it("rejects non-BigInt elements with TypeError", () => {
    const b = new RoaringBitmap64();
    b.addMany([1n, 2n]);
    expect(() => b.removeMany([1, 2] as any)).toThrow(TypeError);
  });

  it("rejects negative BigInt with RangeError", () => {
    const b = new RoaringBitmap64();
    expect(() => b.removeMany([-1n] as any)).toThrow(RangeError);
  });

  it("returns this for chaining", () => {
    const b = new RoaringBitmap64();
    b.add(1n);
    expect(b.removeMany([1n])).toBe(b);
  });

  it("throws when bitmap is disposed", () => {
    const b = new RoaringBitmap64();
    b.dispose();
    expect(() => b.removeMany([1n])).toThrow();
  });
});
