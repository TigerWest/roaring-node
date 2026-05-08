import { describe, expect, it } from "vitest";
import { RoaringBitmap64 } from "../../";

const make = (vals: bigint[]) => {
  const b = new RoaringBitmap64();
  b.addMany(vals);
  return b;
};

describe("RoaringBitmap64 in-place set operations", () => {
  it("andInPlace keeps the intersection", () => {
    const a = make([1n, 2n, 3n]);
    const b = make([2n, 3n, 4n]);
    expect(a.andInPlace(b)).toBe(a);
    expect(a.toArray()).toEqual([2n, 3n]);
  });

  it("orInPlace adds missing elements", () => {
    const a = make([1n, 2n]);
    const b = make([2n, 2n ** 40n]);
    a.orInPlace(b);
    expect(a.toArray()).toEqual([1n, 2n, 2n ** 40n]);
  });

  it("xorInPlace toggles symmetric difference", () => {
    const a = make([1n, 2n, 3n]);
    const b = make([2n, 4n]);
    a.xorInPlace(b);
    expect(a.toArray()).toEqual([1n, 3n, 4n]);
  });

  it("andNotInPlace removes elements present in other", () => {
    const a = make([1n, 2n, 3n]);
    const b = make([2n]);
    a.andNotInPlace(b);
    expect(a.toArray()).toEqual([1n, 3n]);
  });

  it("rejects non-RoaringBitmap64 argument", () => {
    const a = make([1n]);
    expect(() => (a as any).andInPlace({})).toThrow(TypeError);
  });
});
