import { describe, expect, it } from "vitest";
import { RoaringBitmap64 } from "../../";

describe("RoaringBitmap64.hasRange", () => {
  it("returns true when every value in [start, end) is present", () => {
    const b = new RoaringBitmap64();
    b.addRange(100n, 200n);
    expect(b.hasRange(100n, 200n)).toBe(true);
    expect(b.hasRange(150n, 160n)).toBe(true);
    expect(b.hasRange(199n, 200n)).toBe(true);
  });

  it("returns false when any value in the range is missing", () => {
    const b = new RoaringBitmap64();
    b.addRange(100n, 200n);
    b.remove(150n);
    expect(b.hasRange(100n, 200n)).toBe(false);
    expect(b.hasRange(149n, 152n)).toBe(false);
  });

  it("returns false when the range extends past the bitmap", () => {
    const b = new RoaringBitmap64();
    b.addRange(100n, 200n);
    expect(b.hasRange(99n, 200n)).toBe(false);
    expect(b.hasRange(100n, 201n)).toBe(false);
  });

  it("returns false for an empty range (start >= end)", () => {
    const b = new RoaringBitmap64();
    b.addRange(0n, 100n);
    expect(b.hasRange(50n, 50n)).toBe(false);
    expect(b.hasRange(100n, 50n)).toBe(false);
  });

  it("returns false on an empty bitmap", () => {
    const b = new RoaringBitmap64();
    expect(b.hasRange(0n, 1000n)).toBe(false);
  });

  it("works across the 2^32 boundary", () => {
    const b = new RoaringBitmap64();
    b.addRange((1n << 32n) - 5n, (1n << 32n) + 5n);
    expect(b.hasRange((1n << 32n) - 3n, (1n << 32n) + 3n)).toBe(true);
    expect(b.hasRange((1n << 32n) - 6n, (1n << 32n) + 3n)).toBe(false);
  });

  it("rejects non-BigInt with TypeError", () => {
    const b = new RoaringBitmap64();
    expect(() => (b as any).hasRange(0, 10n)).toThrow(TypeError);
    expect(() => (b as any).hasRange(0n, 10)).toThrow(TypeError);
  });

  it("containsRange is an alias for hasRange", () => {
    const b = new RoaringBitmap64();
    b.addRange(0n, 50n);
    expect((b as any).containsRange(0n, 50n)).toBe(true);
    expect((b as any).containsRange(0n, 51n)).toBe(false);
  });
});
