import { describe, expect, it } from "vitest";
import { RoaringBitmap64 } from "../..";

describe("RoaringBitmap64 frozen guards", () => {
  it("a fresh bitmap reports isFrozen=false", () => {
    const rb = new RoaringBitmap64();
    expect(rb.isFrozen).toBe(false);
  });
});
