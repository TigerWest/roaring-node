import { describe, expect, it } from "vitest";
import { RoaringBitmap64, RoaringBitmap32 } from "../../";

describe("RoaringBitmap64.equals", () => {
  it("compares equal bitmaps", () => {
    const a = new RoaringBitmap64();
    a.add(1n);
    a.add(2n ** 40n);
    const b = new RoaringBitmap64();
    b.add(2n ** 40n);
    b.add(1n);
    expect(a.equals(b)).toBe(true);
  });

  it("returns false for different bitmaps", () => {
    const a = new RoaringBitmap64();
    a.add(1n);
    const b = new RoaringBitmap64();
    b.add(2n);
    expect(a.equals(b)).toBe(false);
  });

  it("rejects RoaringBitmap32 with TypeError", () => {
    const a = new RoaringBitmap64();
    const c = new RoaringBitmap32();
    expect(() => (a as any).equals(c)).toThrow(TypeError);
  });
});
