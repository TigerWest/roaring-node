import { describe, expect, it } from "vitest";
import { RoaringBitmap64 } from "../../";

describe("RoaringBitmap64.reduce", () => {
  it("sums values using bigint accumulator", () => {
    const b = new RoaringBitmap64();
    b.addMany([1n, 2n, 3n, 4n]);
    expect(b.reduce((acc: bigint, v) => acc + v, 0n)).toBe(10n);
  });

  it("passes (acc, value, index, this) to callback", () => {
    const b = new RoaringBitmap64();
    b.addMany([10n, 20n]);
    const seen: [bigint, bigint, number][] = [];
    b.reduce((acc: bigint, v, i, self) => {
      expect(self).toBe(b);
      seen.push([acc, v, i]);
      return acc + v;
    }, 0n);
    expect(seen).toEqual([
      [0n, 10n, 0],
      [10n, 20n, 1],
    ]);
  });

  it("uses default initial value 0 when none provided", () => {
    const b = new RoaringBitmap64();
    b.add(5n);
    expect((b as any).reduce((_acc: any, _v: bigint) => "x")).toBe("x");
  });

  it("throws TypeError when fn is not a function", () => {
    expect(() => (new RoaringBitmap64() as any).reduce(undefined, 0n)).toThrow(TypeError);
  });
});

describe("RoaringBitmap64.reduceRight", () => {
  it("walks from largest to smallest with descending index", () => {
    const b = new RoaringBitmap64();
    b.addMany([1n, 2n, 3n]);
    const seen: [bigint, number][] = [];
    const total = b.reduceRight((acc: bigint, v, i) => {
      seen.push([v, i]);
      return acc + v;
    }, 0n);
    expect(total).toBe(6n);
    expect(seen).toEqual([
      [3n, 2],
      [2n, 1],
      [1n, 0],
    ]);
  });

  it("returns initial value on empty bitmap", () => {
    const b = new RoaringBitmap64();
    expect(b.reduceRight((_a: bigint, _v) => 1n, 42n)).toBe(42n);
  });

  it("throws TypeError when fn is not a function", () => {
    expect(() => (new RoaringBitmap64() as any).reduceRight("x", 0n)).toThrow(TypeError);
  });
});
