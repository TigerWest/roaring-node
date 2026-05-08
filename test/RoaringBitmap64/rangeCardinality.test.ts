import { describe, expect, it } from "vitest";
import { RoaringBitmap64 } from "../../";

describe("RoaringBitmap64.rangeCardinality", () => {
  it("returns count of elements in [start, end)", () => {
    const b = new RoaringBitmap64();
    b.addRange(0n, 100n);
    expect(b.rangeCardinality(0n, 100n)).toBe(100n);
    expect(b.rangeCardinality(10n, 20n)).toBe(10n);
    expect(b.rangeCardinality(99n, 101n)).toBe(1n);
  });

  it("returns 0n when range is empty (start >= end)", () => {
    const b = new RoaringBitmap64();
    b.addRange(0n, 10n);
    expect(b.rangeCardinality(5n, 5n)).toBe(0n);
    expect(b.rangeCardinality(10n, 5n)).toBe(0n);
  });

  it("returns 0n when bitmap is empty", () => {
    const b = new RoaringBitmap64();
    expect(b.rangeCardinality(0n, 1000n)).toBe(0n);
  });

  it("works across 2^32 boundary", () => {
    const b = new RoaringBitmap64();
    b.addRange((1n << 32n) - 5n, (1n << 32n) + 5n);
    expect(b.rangeCardinality((1n << 32n) - 3n, (1n << 32n) + 3n)).toBe(6n);
  });

  it("rejects non-BigInt with TypeError", () => {
    const b = new RoaringBitmap64();
    expect(() => (b as any).rangeCardinality(0, 10n)).toThrow(TypeError);
  });
});
