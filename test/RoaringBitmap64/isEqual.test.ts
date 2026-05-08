import { describe, expect, it } from "vitest";
import { RoaringBitmap32, RoaringBitmap64 } from "../../";

describe("RoaringBitmap64.isEqual (alias for equals)", () => {
  it("returns the same answer as equals", () => {
    const a = new RoaringBitmap64();
    a.add(1n);
    a.add(2n ** 40n);
    const b = new RoaringBitmap64();
    b.add(2n ** 40n);
    b.add(1n);
    expect((a as any).isEqual(b)).toBe(true);
    expect((a as any).isEqual(b)).toBe(a.equals(b));
  });

  it("returns false for different bitmaps", () => {
    const a = new RoaringBitmap64();
    a.add(1n);
    const b = new RoaringBitmap64();
    b.add(2n);
    expect((a as any).isEqual(b)).toBe(false);
    expect((a as any).isEqual(b)).toBe(a.equals(b));
  });

  it("two empty bitmaps are equal", () => {
    const a = new RoaringBitmap64();
    const b = new RoaringBitmap64();
    expect((a as any).isEqual(b)).toBe(true);
  });

  it("rejects RoaringBitmap32 with TypeError", () => {
    const a = new RoaringBitmap64();
    const c = new RoaringBitmap32();
    expect(() => (a as any).isEqual(c)).toThrow(TypeError);
  });
});
