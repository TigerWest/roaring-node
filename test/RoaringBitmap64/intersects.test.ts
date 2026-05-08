import { describe, expect, it } from "vitest";
import { RoaringBitmap32, RoaringBitmap64 } from "../../";

describe("RoaringBitmap64.intersects", () => {
  it("returns true when bitmaps share at least one value", () => {
    const a = new RoaringBitmap64();
    a.add(1n);
    a.add(2n ** 40n);
    const b = new RoaringBitmap64();
    b.add(2n ** 40n);
    expect(a.intersects(b)).toBe(true);
  });

  it("returns false when bitmaps are disjoint", () => {
    const a = new RoaringBitmap64();
    a.add(1n);
    const b = new RoaringBitmap64();
    b.add(2n);
    expect(a.intersects(b)).toBe(false);
  });

  it("returns false when either side is empty", () => {
    const a = new RoaringBitmap64();
    const b = new RoaringBitmap64();
    b.add(1n);
    expect(a.intersects(b)).toBe(false);
    expect(b.intersects(a)).toBe(false);
  });

  it("rejects RoaringBitmap32 with TypeError", () => {
    const a = new RoaringBitmap64();
    const c = new RoaringBitmap32();
    expect(() => (a as any).intersects(c)).toThrow(TypeError);
  });

  it("rejects non-bitmap argument with TypeError", () => {
    const a = new RoaringBitmap64();
    expect(() => (a as any).intersects({})).toThrow(TypeError);
    expect(() => (a as any).intersects(null)).toThrow(TypeError);
    expect(() => (a as any).intersects()).toThrow(TypeError);
  });
});

describe("RoaringBitmap64.intersectsWithRange", () => {
  it("returns true when the bitmap has values in [start, end)", () => {
    const b = new RoaringBitmap64();
    b.addRange(100n, 200n);
    expect(b.intersectsWithRange(50n, 150n)).toBe(true);
    expect(b.intersectsWithRange(199n, 200n)).toBe(true);
    expect(b.intersectsWithRange(100n, 101n)).toBe(true);
  });

  it("returns false when no value falls in [start, end)", () => {
    const b = new RoaringBitmap64();
    b.addRange(100n, 200n);
    expect(b.intersectsWithRange(0n, 100n)).toBe(false);
    expect(b.intersectsWithRange(200n, 1000n)).toBe(false);
  });

  it("returns false when start >= end (empty range)", () => {
    const b = new RoaringBitmap64();
    b.add(50n);
    expect(b.intersectsWithRange(50n, 50n)).toBe(false);
    expect(b.intersectsWithRange(100n, 50n)).toBe(false);
  });

  it("returns false on an empty bitmap", () => {
    const b = new RoaringBitmap64();
    expect(b.intersectsWithRange(0n, 1000n)).toBe(false);
  });

  it("works across the 2^32 boundary", () => {
    const b = new RoaringBitmap64();
    b.addRange((1n << 32n) - 5n, (1n << 32n) + 5n);
    expect(b.intersectsWithRange((1n << 32n) - 3n, (1n << 32n) + 3n)).toBe(true);
    expect(b.intersectsWithRange(0n, (1n << 32n) - 100n)).toBe(false);
  });

  it("rejects non-BigInt with TypeError", () => {
    const b = new RoaringBitmap64();
    expect(() => (b as any).intersectsWithRange(0, 10n)).toThrow(TypeError);
    expect(() => (b as any).intersectsWithRange(0n, 10)).toThrow(TypeError);
  });
});
