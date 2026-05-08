import { describe, expect, it } from "vitest";
import { RoaringBitmap64 } from "../../";

describe("RoaringBitmap64.clear return value", () => {
  it("returns true when bitmap had content", () => {
    const rb = new RoaringBitmap64([1n, 2n]);
    expect(rb.clear()).toBe(true);
    expect(rb.size).toBe(0n);
  });
  it("returns false when bitmap was already empty", () => {
    expect(new RoaringBitmap64().clear()).toBe(false);
  });
});
