import { describe, expect, it } from "vitest";
import { RoaringBitmap64 } from "../../";

describe("RoaringBitmap64.size", () => {
  it("returns 0n for an empty bitmap", () => {
    const b = new RoaringBitmap64();
    expect(b.size).toBe(0n);
    expect(b.isEmpty).toBe(true);
  });

  it("counts inserted unique values", () => {
    const b = new RoaringBitmap64();
    b.add(1n);
    b.add(1n);
    b.add(2n ** 50n);
    expect(b.size).toBe(2n);
    expect(b.isEmpty).toBe(false);
  });
});
