import { describe, expect, it } from "vitest";
import { RoaringBitmap32, RoaringBitmap64 } from "../../";

describe("RoaringBitmap64.copyFrom", () => {
  it("replaces this bitmap's contents with a copy of other", () => {
    const a = new RoaringBitmap64();
    a.add(1n);
    a.add(2n);
    a.add(3n);
    const b = new RoaringBitmap64();
    b.add(100n);
    b.add(2n ** 40n);
    expect(a.copyFrom(b)).toBe(a);
    expect(a.toArray()).toEqual([100n, 2n ** 40n]);
    expect(a.size).toBe(2n);
  });

  it("makes operands independent — mutating this does not affect other", () => {
    const a = new RoaringBitmap64();
    const b = new RoaringBitmap64();
    b.add(1n);
    b.add(2n);
    a.copyFrom(b);
    a.add(99n);
    expect(b.has(99n)).toBe(false);
    expect(b.toArray()).toEqual([1n, 2n]);
    expect(a.toArray()).toEqual([1n, 2n, 99n]);
  });

  it("copying an empty bitmap clears this", () => {
    const a = new RoaringBitmap64();
    a.add(1n);
    a.add(2n);
    const empty = new RoaringBitmap64();
    a.copyFrom(empty);
    expect(a.toArray()).toEqual([]);
    expect(a.isEmpty).toBe(true);
  });

  it("copyFrom self is a no-op for values", () => {
    const a = new RoaringBitmap64();
    a.add(1n);
    a.add(2n);
    expect(a.copyFrom(a)).toBe(a);
    expect(a.toArray()).toEqual([1n, 2n]);
  });

  it("rejects non-RoaringBitmap64 with TypeError", () => {
    const a = new RoaringBitmap64();
    const c = new RoaringBitmap32();
    expect(() => (a as any).copyFrom(c)).toThrow(TypeError);
    expect(() => (a as any).copyFrom(null)).toThrow(TypeError);
    expect(() => (a as any).copyFrom({})).toThrow(TypeError);
    expect(() => (a as any).copyFrom()).toThrow(TypeError);
  });
});
