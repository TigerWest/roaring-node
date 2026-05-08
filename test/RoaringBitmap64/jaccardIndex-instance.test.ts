import { describe, expect, it } from "vitest";
import { RoaringBitmap64 } from "../../";

describe("RoaringBitmap64 instance jaccardIndex", () => {
  it("matches the static result", () => {
    const a = new RoaringBitmap64([1n, 2n, 3n]);
    const b = new RoaringBitmap64([2n, 3n, 4n]);
    expect(a.jaccardIndex(b)).toBeCloseTo(RoaringBitmap64.jaccardIndex(a, b));
    expect(a.jaccardIndex(b)).toBeCloseTo(2 / 4);
  });
});
