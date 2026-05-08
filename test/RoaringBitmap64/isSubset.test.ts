import { describe, expect, it } from "vitest";
import { RoaringBitmap64 } from "../../";

describe("RoaringBitmap64.isSubset / isStrictSubset", () => {
  it("recognizes subset", () => {
    const a = new RoaringBitmap64();
    a.add(1n);
    a.add(2n);
    const b = new RoaringBitmap64();
    b.add(1n);
    b.add(2n);
    b.add(3n);
    expect(a.isSubset(b)).toBe(true);
    expect(a.isStrictSubset(b)).toBe(true);
    expect(b.isSubset(a)).toBe(false);
  });

  it("equal sets: subset yes, strictSubset no", () => {
    const a = new RoaringBitmap64();
    a.add(1n);
    const b = new RoaringBitmap64();
    b.add(1n);
    expect(a.isSubset(b)).toBe(true);
    expect(a.isStrictSubset(b)).toBe(false);
  });
});
