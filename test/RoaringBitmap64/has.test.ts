import { describe, expect, it } from "vitest";
import { RoaringBitmap64 } from "../../";

describe("RoaringBitmap64.has", () => {
  it("reports membership across the full uint64 range", () => {
    const b = new RoaringBitmap64();
    b.add(0n);
    b.add(2n ** 40n);
    expect(b.has(0n)).toBe(true);
    expect(b.has(1n)).toBe(false);
    expect(b.has(2n ** 40n)).toBe(true);
    expect(b.has(2n ** 64n - 1n)).toBe(false);
  });

  it("rejects non-BigInt with TypeError", () => {
    const b = new RoaringBitmap64();
    expect(() => (b as any).has(1)).toThrow(TypeError);
  });
});
