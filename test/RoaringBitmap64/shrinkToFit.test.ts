import { describe, expect, it } from "vitest";
import { RoaringBitmap64 } from "../../";

describe("RoaringBitmap64.shrinkToFit", () => {
  it("returns 0n on an empty bitmap", () => {
    const b = new RoaringBitmap64();
    expect(b.shrinkToFit()).toBe(0n);
  });

  it("returns a non-negative bigint for a bitmap with values", () => {
    const b = new RoaringBitmap64();
    for (let i = 0n; i < 1000n; i++) b.add(i);
    const saved = b.shrinkToFit();
    expect(typeof saved).toBe("bigint");
    expect(saved >= 0n).toBe(true);
  });

  it("preserves bitmap values exactly", () => {
    const b = new RoaringBitmap64();
    b.add(1n);
    b.add(1_000_000n);
    b.add(1n << 40n);
    const before = b.toArray();
    b.shrinkToFit();
    expect(b.toArray()).toEqual(before);
    expect(b.size).toBe(3n);
  });

  it("throws when bitmap is disposed", () => {
    const b = new RoaringBitmap64();
    b.dispose();
    expect(() => b.shrinkToFit()).toThrow();
  });
});
