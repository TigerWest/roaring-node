import { describe, expect, it } from "vitest";
import { RoaringBitmap64 } from "../../";

const make = (vals: bigint[]) => {
  const b = new RoaringBitmap64();
  b.addMany(vals);
  return b;
};

describe("RoaringBitmap64 static set operations", () => {
  const a = make([1n, 2n, 3n]);
  const b = make([2n, 3n, 4n]);

  it("and returns intersection (new instance, operands unchanged)", () => {
    const r = RoaringBitmap64.and(a, b);
    expect(r).toBeInstanceOf(RoaringBitmap64);
    expect(r.toArray()).toEqual([2n, 3n]);
    expect(a.size).toBe(3n);
    expect(b.size).toBe(3n);
  });

  it("or returns union", () => {
    expect(RoaringBitmap64.or(a, b).toArray()).toEqual([1n, 2n, 3n, 4n]);
  });

  it("xor returns symmetric difference", () => {
    expect(RoaringBitmap64.xor(a, b).toArray()).toEqual([1n, 4n]);
  });

  it("andNot returns difference", () => {
    expect(RoaringBitmap64.andNot(a, b).toArray()).toEqual([1n]);
  });

  it("*Cardinality returns counts as bigint", () => {
    expect(RoaringBitmap64.andCardinality(a, b)).toBe(2n);
    expect(RoaringBitmap64.orCardinality(a, b)).toBe(4n);
    expect(RoaringBitmap64.xorCardinality(a, b)).toBe(2n);
    expect(RoaringBitmap64.andNotCardinality(a, b)).toBe(1n);
  });

  it("rejects non-RoaringBitmap64", () => {
    expect(() => (RoaringBitmap64 as any).and(a, {})).toThrow(TypeError);
  });
});
