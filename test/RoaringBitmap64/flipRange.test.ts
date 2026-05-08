import { describe, expect, it } from "vitest";
import { RoaringBitmap64 } from "../../";

describe("RoaringBitmap64.flipRange", () => {
  it("toggles every value in [start, end) in place", () => {
    const b = new RoaringBitmap64();
    b.add(1n);
    b.add(2n);
    b.add(5n);
    expect(b.flipRange(1n, 5n)).toBe(b);
    // 1 and 2 were present -> removed; 3 and 4 were absent -> added; 5 untouched
    expect(b.toArray()).toEqual([3n, 4n, 5n]);
  });

  it("is a no-op when start >= end", () => {
    const b = new RoaringBitmap64();
    b.add(1n);
    b.add(2n);
    expect(b.flipRange(5n, 5n)).toBe(b);
    expect(b.toArray()).toEqual([1n, 2n]);
    expect(b.flipRange(10n, 5n)).toBe(b);
    expect(b.toArray()).toEqual([1n, 2n]);
  });

  it("works across the 2^32 boundary", () => {
    const b = new RoaringBitmap64();
    b.add((1n << 32n) - 1n);
    b.add(1n << 32n);
    b.flipRange((1n << 32n) - 1n, (1n << 32n) + 2n);
    // (2^32 - 1) was present -> removed
    // (2^32) was present -> removed
    // (2^32 + 1) was absent -> added
    expect(b.toArray()).toEqual([(1n << 32n) + 1n]);
  });

  it("rejects non-BigInt range with TypeError", () => {
    const b = new RoaringBitmap64();
    expect(() => (b as any).flipRange(0, 10n)).toThrow(TypeError);
    expect(() => (b as any).flipRange(0n, 10)).toThrow(TypeError);
  });

  it("requires both arguments", () => {
    const b = new RoaringBitmap64();
    expect(() => (b as any).flipRange(0n)).toThrow();
    expect(() => (b as any).flipRange()).toThrow();
  });

  it("throws when bitmap is disposed", () => {
    const b = new RoaringBitmap64();
    b.dispose();
    expect(() => b.flipRange(0n, 10n)).toThrow();
  });
});
