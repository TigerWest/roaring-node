import { describe, expect, it } from "vitest";
import { RoaringBitmap64 } from "../../";

describe("RoaringBitmap64.deserializeAsync (Buffer)", () => {
  it("round-trips a portable buffer asynchronously", async () => {
    const orig = new RoaringBitmap64([1n, 2n, 2n ** 33n, 2n ** 50n, 2n ** 64n - 1n]);
    const buf = orig.serialize();
    const decoded = await RoaringBitmap64.deserializeAsync(buf);
    expect(decoded.toArray()).toEqual([1n, 2n, 2n ** 33n, 2n ** 50n, 2n ** 64n - 1n]);
  });
});
