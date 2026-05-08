import { describe, expect, it } from "vitest";
import { RoaringBitmap64 } from "../../";

describe("RoaringBitmap64.addRange", () => {
  it("adds [start, end) half-open and returns this", () => {
    const b = new RoaringBitmap64();
    expect(b.addRange(0n, 10n)).toBe(b);
    expect(b.size).toBe(10n);
    expect(b.has(0n)).toBe(true);
    expect(b.has(9n)).toBe(true);
    expect(b.has(10n)).toBe(false);
  });

  it("is a no-op when start >= end", () => {
    const b = new RoaringBitmap64();
    b.addRange(5n, 5n);
    b.addRange(10n, 5n);
    expect(b.size).toBe(0n);
  });

  it("supports ranges crossing 2^32", () => {
    const b = new RoaringBitmap64();
    const start = (1n << 32n) - 5n;
    const end = (1n << 32n) + 5n;
    b.addRange(start, end);
    expect(b.size).toBe(10n);
    expect(b.has(start)).toBe(true);
    expect(b.has(end - 1n)).toBe(true);
    expect(b.has(end)).toBe(false);
  });

  it("rejects non-BigInt with TypeError", () => {
    const b = new RoaringBitmap64();
    expect(() => (b as any).addRange(0, 10n)).toThrow(TypeError);
    expect(() => (b as any).addRange(0n, 10)).toThrow(TypeError);
  });

  it("rejects negative BigInt with RangeError", () => {
    const b = new RoaringBitmap64();
    expect(() => b.addRange(-1n, 10n)).toThrow(RangeError);
  });

  it("rejects end > 2^64 with RangeError", () => {
    const b = new RoaringBitmap64();
    expect(() => b.addRange(0n, 1n << 64n)).toThrow(RangeError);
  });

  it("cannot include 2^64 - 1 via addRange (half-open end overflow); add() is the workaround", () => {
    // The half-open contract requires `end > start`, and `end` must fit in
    // uint64. So including the maximum value 2^64 - 1 would require
    // `end === 2^64`, which RangeError-rejects.
    const max = (1n << 64n) - 1n;
    const b = new RoaringBitmap64();
    expect(() => b.addRange(max, 1n << 64n)).toThrow(RangeError);

    // Documented workaround: use add() for the single max value, addRange()
    // for everything below it.
    b.add(max);
    expect(b.has(max)).toBe(true);
    expect(b.size).toBe(1n);
  });
});
