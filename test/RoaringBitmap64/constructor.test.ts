import { describe, expect, it } from "vitest";
import { RoaringBitmap64 } from "../../";

describe("RoaringBitmap64 constructor (skeleton)", () => {
  it("creates an empty instance", () => {
    const b = new RoaringBitmap64();
    expect(b).toBeInstanceOf(RoaringBitmap64);
  });

  it("starts empty", () => {
    const b = new RoaringBitmap64();
    expect(b.size).toBe(0n);
    expect(b.isEmpty).toBe(true);
  });
});
