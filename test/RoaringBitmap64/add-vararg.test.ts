import { describe, expect, it } from "vitest";
import { RoaringBitmap64 } from "../..";

describe("RoaringBitmap64 vararg add/remove parity with RB32", () => {
  it("add accepts multiple bigints (no silent drop)", () => {
    const b = new RoaringBitmap64();
    b.add(1n, 2n, 3n);
    expect(b.size).toBe(3n);
    expect(b.toArray()).toEqual([1n, 2n, 3n]);
  });

  it("tryAdd returns true when at least one value is new", () => {
    const b = new RoaringBitmap64([1n]);
    expect(b.tryAdd(1n, 2n)).toBe(true);
    expect(b.tryAdd(1n, 2n)).toBe(false);
    expect(b.toArray()).toEqual([1n, 2n]);
  });

  it("remove accepts multiple bigints", () => {
    const b = new RoaringBitmap64([1n, 2n, 3n, 4n]);
    expect(b.remove(2n, 4n)).toBe(b);
    expect(b.toArray()).toEqual([1n, 3n]);
  });

  it("delete returns true when at least one value was actually removed", () => {
    const b = new RoaringBitmap64([1n, 2n]);
    expect(b.delete(2n, 99n)).toBe(true);
    expect(b.delete(99n, 100n)).toBe(false);
    expect(b.toArray()).toEqual([1n]);
  });

  it("zero arguments is a no-op for all four", () => {
    const b = new RoaringBitmap64([1n]);
    expect(b.add()).toBe(b);
    expect(b.tryAdd()).toBe(false);
    expect(b.remove()).toBe(b);
    expect(b.delete()).toBe(false);
    expect(b.toArray()).toEqual([1n]);
  });

  it("add/remove are atomic-on-error: invalid arg in middle leaves prior writes visible", () => {
    // Documented semantic: we apply args left-to-right and stop on the first
    // bad arg via early-return after the throw. Pre-existing writes stand —
    // matches what readUint64BigInt's throw does naturally.
    const b = new RoaringBitmap64();
    expect(() => (b as any).add(10n, "nope", 30n)).toThrow();
    expect(b.has(10n)).toBe(true);
    expect(b.has(30n)).toBe(false);
  });
});
