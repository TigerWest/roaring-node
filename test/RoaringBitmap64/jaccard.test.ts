import { describe, expect, it } from "vitest";
import { RoaringBitmap64 } from "../../";

describe("RoaringBitmap64.jaccardIndex", () => {
  it("computes |A∩B| / |A∪B|", () => {
    const a = new RoaringBitmap64();
    a.addMany([1n, 2n, 3n]);
    const b = new RoaringBitmap64();
    b.addMany([2n, 3n, 4n]);
    expect(RoaringBitmap64.jaccardIndex(a, b)).toBeCloseTo(0.5, 12);
  });

  it("returns 1 for equal non-empty sets", () => {
    const a = new RoaringBitmap64();
    a.add(1n);
    const b = new RoaringBitmap64();
    b.add(1n);
    expect(RoaringBitmap64.jaccardIndex(a, b)).toBe(1);
  });
});
