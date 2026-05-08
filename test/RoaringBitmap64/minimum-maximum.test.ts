import { describe, expect, it } from "vitest";
import { RoaringBitmap64 } from "../../";

describe("RoaringBitmap64.minimum / maximum", () => {
  it("returns undefined when empty", () => {
    const b = new RoaringBitmap64();
    expect(b.minimum()).toBeUndefined();
    expect(b.maximum()).toBeUndefined();
  });

  it("returns extremes across the full range", () => {
    const b = new RoaringBitmap64();
    b.add(7n);
    b.add(2n ** 50n);
    b.add(2n ** 64n - 1n);
    expect(b.minimum()).toBe(7n);
    expect(b.maximum()).toBe(2n ** 64n - 1n);
  });
});
