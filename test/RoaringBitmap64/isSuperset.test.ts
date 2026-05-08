import { describe, expect, it } from "vitest";
import { RoaringBitmap32, RoaringBitmap64 } from "../../";

describe("RoaringBitmap64.isSuperset / isStrictSuperset", () => {
  it("recognizes superset", () => {
    const a = new RoaringBitmap64();
    a.add(1n);
    a.add(2n);
    a.add(3n);
    const b = new RoaringBitmap64();
    b.add(1n);
    b.add(2n);
    expect(a.isSuperset(b)).toBe(true);
    expect(a.isStrictSuperset(b)).toBe(true);
    expect(b.isSuperset(a)).toBe(false);
    expect(b.isStrictSuperset(a)).toBe(false);
  });

  it("equal sets: superset yes, strictSuperset no", () => {
    const a = new RoaringBitmap64();
    a.add(1n);
    const b = new RoaringBitmap64();
    b.add(1n);
    expect(a.isSuperset(b)).toBe(true);
    expect(a.isStrictSuperset(b)).toBe(false);
  });

  it("empty bitmap is a (non-strict) superset of itself", () => {
    const a = new RoaringBitmap64();
    const b = new RoaringBitmap64();
    expect(a.isSuperset(b)).toBe(true);
    expect(a.isStrictSuperset(b)).toBe(false);
  });

  it("any non-empty bitmap is a strict superset of an empty bitmap", () => {
    const a = new RoaringBitmap64();
    a.add(1n);
    const b = new RoaringBitmap64();
    expect(a.isSuperset(b)).toBe(true);
    expect(a.isStrictSuperset(b)).toBe(true);
  });

  it("is the inverse of isSubset / isStrictSubset", () => {
    const a = new RoaringBitmap64();
    a.add(2n ** 40n);
    const b = new RoaringBitmap64();
    b.add(2n ** 40n);
    b.add(2n ** 40n + 1n);
    expect(a.isSubset(b)).toBe(b.isSuperset(a));
    expect(a.isStrictSubset(b)).toBe(b.isStrictSuperset(a));
  });

  it("rejects RoaringBitmap32 with TypeError", () => {
    const a = new RoaringBitmap64();
    const c = new RoaringBitmap32();
    expect(() => (a as any).isSuperset(c)).toThrow(TypeError);
    expect(() => (a as any).isStrictSuperset(c)).toThrow(TypeError);
  });

  it("rejects missing argument with TypeError", () => {
    const a = new RoaringBitmap64();
    expect(() => (a as any).isSuperset()).toThrow(TypeError);
    expect(() => (a as any).isStrictSuperset()).toThrow(TypeError);
  });
});
