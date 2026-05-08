import { describe, expect, it } from "vitest";
import { RoaringBitmap64 } from "../../";

describe("RoaringBitmap64.addMany (BigUint64Array)", () => {
  it("adds all values from a BigUint64Array", () => {
    const data = new BigUint64Array([0n, 1n, 2n ** 32n, 2n ** 50n, 2n ** 64n - 1n]);
    const b = new RoaringBitmap64();
    expect(b.addMany(data)).toBe(b);
    expect(b.size).toBe(5n);
    expect(b.has(2n ** 50n)).toBe(true);
    expect(b.has(2n ** 64n - 1n)).toBe(true);
  });

  it("rejects non-BigUint64Array, non-iterable input", () => {
    const b = new RoaringBitmap64();
    expect(() => (b as any).addMany(42)).toThrow(TypeError);
  });
});

describe("RoaringBitmap64.addMany (iterable)", () => {
  it("accepts a bigint[]", () => {
    const b = new RoaringBitmap64();
    b.addMany([1n, 2n, 2n ** 50n]);
    expect(b.size).toBe(3n);
  });

  it("accepts a generator", () => {
    function* gen() {
      yield 1n;
      yield 2n ** 40n;
    }
    const b = new RoaringBitmap64();
    b.addMany(gen());
    expect(b.size).toBe(2n);
  });

  it("rejects iterable element that is not a BigInt", () => {
    const b = new RoaringBitmap64();
    expect(() => b.addMany([1n, 2 as any, 3n])).toThrow(TypeError);
  });

  it("rejects iterable element that is out of range", () => {
    const b = new RoaringBitmap64();
    expect(() => b.addMany([1n, 2n ** 64n])).toThrow(RangeError);
  });
});

describe("RoaringBitmap64.addMany (RoaringBitmap64 source)", () => {
  it("accepts a RoaringBitmap64 source via roaring64_bitmap_or_inplace", () => {
    const a = new RoaringBitmap64([1n, 2n, 3n]);
    const b = new RoaringBitmap64([3n, 4n, 5n]);
    a.addMany(b);
    expect([...a].sort((x, y) => Number(x - y))).toEqual([1n, 2n, 3n, 4n, 5n]);
  });

  it("addMany(self) is a no-op (idempotent)", () => {
    const a = new RoaringBitmap64([1n, 2n, 3n]);
    const before = a.size;
    a.addMany(a);
    expect(a.size).toBe(before);
    expect([...a]).toEqual([1n, 2n, 3n]);
  });
});

describe("RoaringBitmap64.removeMany", () => {
  it("removes via BigUint64Array", () => {
    const b = new RoaringBitmap64();
    b.addMany([1n, 2n, 3n, 4n]);
    b.removeMany(new BigUint64Array([2n, 4n]));
    expect(b.size).toBe(2n);
    expect(b.has(1n)).toBe(true);
    expect(b.has(2n)).toBe(false);
  });

  it("removes via iterable", () => {
    const b = new RoaringBitmap64();
    b.addMany([1n, 2n, 3n]);
    b.removeMany([1n, 3n]);
    expect(b.size).toBe(1n);
    expect(b.has(2n)).toBe(true);
  });
});
