import { describe, expect, it } from "vitest";
import { RoaringBitmap64 } from "../../";

describe("RoaringBitmap64.freeze", () => {
  it("rejects mutations after freeze()", () => {
    const rb = new RoaringBitmap64([1n, 2n]);
    rb.freeze();
    expect(rb.isFrozen).toBe(true);
    expect(() => rb.add(3n)).toThrow();
    expect(() => rb.remove(1n)).toThrow();
    expect(() => rb.clear()).toThrow();
  });

  it("read methods continue to work", () => {
    const rb = new RoaringBitmap64([1n, 2n, 3n]);
    rb.freeze();
    expect(rb.has(2n)).toBe(true);
    expect(rb.size).toBe(3n);
    expect([...rb]).toEqual([1n, 2n, 3n]);
  });

  it("freeze is idempotent", () => {
    const rb = new RoaringBitmap64();
    rb.freeze();
    rb.freeze();
    expect(rb.isFrozen).toBe(true);
  });
});
