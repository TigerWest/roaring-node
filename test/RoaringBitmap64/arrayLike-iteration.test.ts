import { describe, expect, it } from "vitest";
import { RoaringBitmap64 } from "../../";

describe("RoaringBitmap64.forEach", () => {
  it("calls fn(value, index, this) for each value in ascending order", () => {
    const b = new RoaringBitmap64();
    b.addMany([2n ** 40n, 1n, 10n]);
    const seen: [bigint, number][] = [];
    const result = b.forEach((v, i, self) => {
      expect(self).toBe(b);
      seen.push([v, i]);
    });
    expect(result).toBe(b);
    expect(seen).toEqual([
      [1n, 0],
      [10n, 1],
      [2n ** 40n, 2],
    ]);
  });

  it("respects thisArg", () => {
    const b = new RoaringBitmap64();
    b.add(1n);
    const ctx = { tag: "ctx" };
    b.forEach(function (this: typeof ctx) {
      expect(this).toBe(ctx);
    }, ctx);
  });

  it("throws TypeError when fn is not a function", () => {
    const b = new RoaringBitmap64();
    expect(() => (b as any).forEach(123)).toThrow(TypeError);
  });

  it("no-op on empty bitmap", () => {
    const b = new RoaringBitmap64();
    let calls = 0;
    b.forEach(() => {
      calls++;
    });
    expect(calls).toBe(0);
  });
});

describe("RoaringBitmap64.map", () => {
  it("returns mapped array of same length", () => {
    const b = new RoaringBitmap64();
    b.addMany([1n, 2n, 3n]);
    const result = b.map((v, i) => `${v}@${i}`);
    expect(result).toEqual(["1@0", "2@1", "3@2"]);
  });

  it("appends to provided output array when given", () => {
    const b = new RoaringBitmap64();
    b.addMany([10n, 20n]);
    const out: bigint[] = [99n];
    const ret = b.map((v) => v * 2n, undefined, out);
    expect(ret).toBe(out);
    expect(out).toEqual([99n, 20n, 40n]);
  });

  it("respects thisArg", () => {
    const b = new RoaringBitmap64();
    b.add(7n);
    const ctx = { factor: 3n };
    const result = b.map(function (this: typeof ctx, v) {
      return v * this.factor;
    }, ctx);
    expect(result).toEqual([21n]);
  });

  it("throws TypeError when fn is not a function", () => {
    const b = new RoaringBitmap64();
    expect(() => (b as any).map(123)).toThrow(TypeError);
  });
});

describe("RoaringBitmap64.filter", () => {
  it("returns a new array with values matching the predicate", () => {
    const b = new RoaringBitmap64();
    b.addMany([1n, 2n, 3n, 4n]);
    expect(b.filter((v) => v % 2n === 0n)).toEqual([2n, 4n]);
  });

  it("appends matches to provided output array", () => {
    const b = new RoaringBitmap64();
    b.addMany([1n, 2n, 3n]);
    const out: bigint[] = [];
    const ret = b.filter((v) => v > 1n, undefined, out);
    expect(ret).toBe(out);
    expect(out).toEqual([2n, 3n]);
  });

  it("throws TypeError when predicate is not a function", () => {
    const b = new RoaringBitmap64();
    expect(() => (b as any).filter("nope")).toThrow(TypeError);
  });
});
