import { describe, expect, it } from "vitest";
import { RoaringBitmap64 } from "../../";

describe("RoaringBitmap64 keys/values/entries/toStringTag", () => {
  it("keys() yields ascending bigint values", () => {
    const b = new RoaringBitmap64();
    b.addMany([2n ** 40n, 1n, 10n]);
    expect([...b.keys()]).toEqual([1n, 10n, 2n ** 40n]);
  });

  it("values() yields ascending bigint values (alias of keys)", () => {
    const b = new RoaringBitmap64();
    b.addMany([5n, 3n, 7n]);
    expect([...b.values()]).toEqual([3n, 5n, 7n]);
  });

  it("entries() yields [v, v] bigint pairs", () => {
    const b = new RoaringBitmap64();
    b.addMany([1n, 2n]);
    expect([...b.entries()]).toEqual([
      [1n, 1n],
      [2n, 2n],
    ]);
  });

  it("Object.prototype.toString.call returns [object Set]", () => {
    const b = new RoaringBitmap64();
    expect(Object.prototype.toString.call(b)).toBe("[object Set]");
  });

  it("empty bitmap yields nothing from keys/values/entries", () => {
    const b = new RoaringBitmap64();
    expect([...b.keys()]).toEqual([]);
    expect([...b.values()]).toEqual([]);
    expect([...b.entries()]).toEqual([]);
  });
});
