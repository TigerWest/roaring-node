import { describe, expect, it } from "vitest";
import { RoaringBitmap64 } from "../../";

describe("RoaringBitmap64.removeRange", () => {
  it("removes [start, end) half-open and returns this", () => {
    const b = new RoaringBitmap64();
    b.addRange(0n, 100n);
    expect(b.removeRange(20n, 30n)).toBe(b);
    expect(b.size).toBe(90n);
    expect(b.has(19n)).toBe(true);
    expect(b.has(20n)).toBe(false);
    expect(b.has(29n)).toBe(false);
    expect(b.has(30n)).toBe(true);
  });

  it("is a no-op when start >= end", () => {
    const b = new RoaringBitmap64();
    b.addRange(0n, 10n);
    b.removeRange(5n, 5n);
    b.removeRange(8n, 3n);
    expect(b.size).toBe(10n);
  });

  it("removing from empty bitmap is a no-op", () => {
    const b = new RoaringBitmap64();
    b.removeRange(0n, 100n);
    expect(b.size).toBe(0n);
  });

  it("rejects non-BigInt with TypeError", () => {
    const b = new RoaringBitmap64();
    expect(() => (b as any).removeRange(0, 10n)).toThrow(TypeError);
  });

  it("rejects negative with RangeError", () => {
    const b = new RoaringBitmap64();
    expect(() => b.removeRange(-1n, 10n)).toThrow(RangeError);
  });
});
