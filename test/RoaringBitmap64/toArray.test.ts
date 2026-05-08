import { describe, expect, it } from "vitest";
import { RoaringBitmap64 } from "../../";

describe("RoaringBitmap64.toArray", () => {
  it("returns empty array for empty bitmap", () => {
    const b = new RoaringBitmap64();
    expect(b.toArray()).toEqual([]);
  });

  it("returns sorted bigint[]", () => {
    const b = new RoaringBitmap64();
    b.addMany([5n, 1n, 2n ** 60n, 0n]);
    expect(b.toArray()).toEqual([0n, 1n, 5n, 2n ** 60n]);
  });
});
