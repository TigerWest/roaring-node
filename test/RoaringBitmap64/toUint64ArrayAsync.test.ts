import { describe, expect, it } from "vitest";
import { RoaringBitmap64 } from "../../";

describe("RoaringBitmap64.toUint64ArrayAsync", () => {
  it("matches the synchronous toUint64Array output", async () => {
    const rb = new RoaringBitmap64();
    rb.addRange(0n, 1024n);
    rb.add(2n ** 50n);
    const sync = rb.toUint64Array();
    const asyncArr = await rb.toUint64ArrayAsync();
    expect(asyncArr).toBeInstanceOf(BigUint64Array);
    expect(asyncArr.length).toBe(sync.length);
    expect([...asyncArr]).toEqual([...sync]);
  });

  it("returns an empty BigUint64Array for an empty bitmap", async () => {
    const rb = new RoaringBitmap64();
    const arr = await rb.toUint64ArrayAsync();
    expect(arr).toBeInstanceOf(BigUint64Array);
    expect(arr.length).toBe(0);
  });
});
