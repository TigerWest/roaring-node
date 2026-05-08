import { describe, expect, it } from "vitest";
import { RoaringBitmap64 } from "../../";

describe("RoaringBitmap64.clear", () => {
  it("empties the bitmap", () => {
    const b = new RoaringBitmap64();
    b.add(1n);
    b.add(2n);
    b.clear();
    expect(b.size).toBe(0n);
    expect(b.isEmpty).toBe(true);
  });
});
