import { describe, expect, it } from "vitest";
import { RoaringBitmap64 } from "../../";

describe("RoaringBitmap64.toUint64Array(out)", () => {
  it("fills a caller-provided BigUint64Array", () => {
    const rb = new RoaringBitmap64([1n, 2n, 3n, 4n]);
    const out = new BigUint64Array(4);
    expect(rb.toUint64Array(out)).toBe(out);
    expect([...out]).toEqual([1n, 2n, 3n, 4n]);
  });

  it("throws if the buffer is too small", () => {
    const rb = new RoaringBitmap64([1n, 2n, 3n]);
    expect(() => rb.toUint64Array(new BigUint64Array(2))).toThrow();
  });

  it("returns a fresh BigUint64Array when called with no argument", () => {
    expect([...new RoaringBitmap64([42n]).toUint64Array()]).toEqual([42n]);
  });
});
