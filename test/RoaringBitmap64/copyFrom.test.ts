import { describe, expect, it } from "vitest";
import { RoaringBitmap32, RoaringBitmap64 } from "../../";

describe("RoaringBitmap64.copyFrom", () => {
  it("replaces this bitmap's contents with a copy of other", () => {
    const a = new RoaringBitmap64();
    a.add(1n);
    a.add(2n);
    a.add(3n);
    const b = new RoaringBitmap64();
    b.add(100n);
    b.add(2n ** 40n);
    expect(a.copyFrom(b)).toBe(a);
    expect(a.toArray()).toEqual([100n, 2n ** 40n]);
    expect(a.size).toBe(2n);
  });

  it("makes operands independent — mutating this does not affect other", () => {
    const a = new RoaringBitmap64();
    const b = new RoaringBitmap64();
    b.add(1n);
    b.add(2n);
    a.copyFrom(b);
    a.add(99n);
    expect(b.has(99n)).toBe(false);
    expect(b.toArray()).toEqual([1n, 2n]);
    expect(a.toArray()).toEqual([1n, 2n, 99n]);
  });

  it("copying an empty bitmap clears this", () => {
    const a = new RoaringBitmap64();
    a.add(1n);
    a.add(2n);
    const empty = new RoaringBitmap64();
    a.copyFrom(empty);
    expect(a.toArray()).toEqual([]);
    expect(a.isEmpty).toBe(true);
  });

  it("copyFrom self is a no-op for values", () => {
    const a = new RoaringBitmap64();
    a.add(1n);
    a.add(2n);
    expect(a.copyFrom(a)).toBe(a);
    expect(a.toArray()).toEqual([1n, 2n]);
  });

  it("rejects non-empty RoaringBitmap32 (cross-class) — number elements are not BigInt", () => {
    // RB64.copyFrom now treats non-RB64 inputs as iterables (RB32 parity).
    // RB32 yields `number` values; readUint64BigInt rejects non-BigInt with
    // TypeError, so a populated RB32 source is rejected element-by-element.
    // (An *empty* RB32 yields nothing and would be accepted as "clear" —
    // that's deliberate: see the iterable path below.)
    const a = new RoaringBitmap64();
    const c = new RoaringBitmap32();
    c.add(1);
    expect(() => (a as any).copyFrom(c)).toThrow(TypeError);
  });
});

describe("RoaringBitmap64.copyFrom RB32-parity overloads", () => {
  it("accepts an iterable of bigints", () => {
    const b = new RoaringBitmap64([99n]);
    b.copyFrom([1n, 2n, 3n]);
    expect(b.size).toBe(3n);
    expect(b.has(99n)).toBe(false);
    expect(b.has(2n)).toBe(true);
  });

  it("accepts BigUint64Array", () => {
    const b = new RoaringBitmap64();
    b.copyFrom(new BigUint64Array([10n, 20n, 30n]));
    expect(b.toArray()).toEqual([10n, 20n, 30n]);
  });

  it("accepts null and clears", () => {
    const b = new RoaringBitmap64([1n, 2n]);
    b.copyFrom(null);
    expect(b.size).toBe(0n);
  });

  it("accepts undefined and clears", () => {
    const b = new RoaringBitmap64([1n, 2n]);
    b.copyFrom(undefined);
    expect(b.size).toBe(0n);
  });

  it("accepts no argument and clears", () => {
    const b = new RoaringBitmap64([1n, 2n]);
    (b as any).copyFrom();
    expect(b.size).toBe(0n);
  });

  it("RoaringBitmap64 source remains a deep copy (no regression)", () => {
    const a = new RoaringBitmap64([7n, 8n]);
    const b = new RoaringBitmap64();
    b.copyFrom(a);
    a.add(9n);
    expect(b.toArray()).toEqual([7n, 8n]);
  });
});
