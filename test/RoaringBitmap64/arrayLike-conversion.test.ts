import { describe, expect, it } from "vitest";
import { RoaringBitmap64 } from "../../";

describe("RoaringBitmap64.toSet", () => {
  it("returns a new Set<bigint> with all values", () => {
    const b = new RoaringBitmap64();
    b.addMany([1n, 10n, 2n ** 50n]);
    const s = b.toSet();
    expect(s).toBeInstanceOf(Set);
    expect([...s].sort((a, c) => (a < c ? -1 : 1))).toEqual([1n, 10n, 2n ** 50n]);
  });

  it("respects maxLength when first arg is a number", () => {
    const b = new RoaringBitmap64();
    b.addMany([1n, 2n, 3n, 4n]);
    expect(b.toSet(2).size).toBe(2);
  });

  it("appends to provided Set", () => {
    const b = new RoaringBitmap64();
    b.addMany([1n, 2n]);
    const target = new Set<bigint>([99n]);
    const ret = b.toSet(target);
    expect(ret).toBe(target);
    expect(target.size).toBe(3);
    expect(target.has(99n)).toBe(true);
    expect(target.has(1n)).toBe(true);
    expect(target.has(2n)).toBe(true);
  });

  it("respects maxLength when second arg is given to (output, maxLength)", () => {
    const b = new RoaringBitmap64();
    b.addMany([1n, 2n, 3n]);
    const target = new Set<bigint>();
    b.toSet(target, 2);
    expect(target.size).toBe(2);
  });
});

describe("RoaringBitmap64.toSorted / toJSON", () => {
  it("toSorted() with no comparator equals toArray()", () => {
    const b = new RoaringBitmap64();
    b.addMany([2n ** 40n, 1n, 10n]);
    expect(b.toSorted()).toEqual(b.toArray());
  });

  it("toSorted(cmp) sorts using the comparator", () => {
    const b = new RoaringBitmap64();
    b.addMany([1n, 2n, 3n]);
    expect(b.toSorted((a, c) => (a < c ? 1 : a > c ? -1 : 0))).toEqual([3n, 2n, 1n]);
  });

  it("toSorted throws TypeError when cmp is not a function", () => {
    expect(() => new RoaringBitmap64().toSorted(123 as any)).toThrow(TypeError);
  });

  it("toJSON returns the same bigints as toArray (JSON.stringify will throw on bigint)", () => {
    const b = new RoaringBitmap64();
    b.addMany([1n, 2n]);
    expect(b.toJSON()).toEqual([1n, 2n]);
  });
});

describe("RoaringBitmap64.toReversed", () => {
  it("returns values in descending order", () => {
    const b = new RoaringBitmap64();
    b.addMany([2n ** 40n, 1n, 10n]);
    expect(b.toReversed()).toEqual([2n ** 40n, 10n, 1n]);
  });

  it("returns [] for empty bitmap", () => {
    expect(new RoaringBitmap64().toReversed()).toEqual([]);
  });
});
