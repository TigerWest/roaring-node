import { describe, expect, it } from "vitest";
import { RoaringBitmap64 } from "../../";

describe("RoaringBitmap64 rank/select", () => {
  const rb = new RoaringBitmap64([10n, 20n, 30n, 40n, 50n]);

  it("rank returns 1-based count of values <= argument", () => {
    expect(rb.rank(0n)).toBe(0n);
    expect(rb.rank(10n)).toBe(1n);
    expect(rb.rank(25n)).toBe(2n);
    expect(rb.rank(50n)).toBe(5n);
    expect(rb.rank(100n)).toBe(5n);
  });

  it("select returns the value at the given 0-based rank", () => {
    expect(rb.select(0n)).toBe(10n);
    expect(rb.select(2n)).toBe(30n);
    expect(rb.select(4n)).toBe(50n);
  });

  it("select returns undefined when rank is out of range", () => {
    expect(rb.select(5n)).toBeUndefined();
    expect(rb.select(100n)).toBeUndefined();
  });

  it("rank/select reject negative BigInt", () => {
    expect(() => rb.rank(-1n)).toThrow();
    expect(() => rb.select(-1n)).toThrow();
  });
});
