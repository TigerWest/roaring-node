import { describe, expect, it } from "vitest";
import { RoaringBitmap64 } from "../../";

describe("RoaringBitmap64 instance cardinality methods", () => {
  const a = new RoaringBitmap64([1n, 2n, 3n, 4n]);
  const b = new RoaringBitmap64([3n, 4n, 5n, 6n]);

  it("andCardinality returns intersection size", () => expect(a.andCardinality(b)).toBe(2n));
  it("orCardinality returns union size", () => expect(a.orCardinality(b)).toBe(6n));
  it("xorCardinality returns symmetric difference size", () => expect(a.xorCardinality(b)).toBe(4n));
  it("andNotCardinality returns A \\ B size", () => expect(a.andNotCardinality(b)).toBe(2n));
  it("rejects non-RoaringBitmap64 argument", () => {
    expect(() => (a as any).andCardinality({})).toThrow();
  });
});
