import { describe, expect, it } from "vitest";
import { RoaringBitmap64 } from "../../";

describe("RoaringBitmap64.find", () => {
  it("returns the first matching value", () => {
    const b = new RoaringBitmap64();
    b.addMany([1n, 2n, 3n, 4n]);
    expect(b.find((v) => v > 2n)).toBe(3n);
  });

  it("returns undefined if no value matches", () => {
    const b = new RoaringBitmap64();
    b.addMany([1n, 2n]);
    expect(b.find((v) => v > 99n)).toBeUndefined();
  });

  it("returns undefined on empty bitmap", () => {
    expect(new RoaringBitmap64().find(() => true)).toBeUndefined();
  });

  it("throws TypeError when predicate is not a function", () => {
    expect(() => (new RoaringBitmap64() as any).find(1)).toThrow(TypeError);
  });
});

describe("RoaringBitmap64.findIndex", () => {
  it("returns the zero-based index of the first match", () => {
    const b = new RoaringBitmap64();
    b.addMany([10n, 20n, 30n, 40n]);
    expect(b.findIndex((v) => v >= 30n)).toBe(2);
  });

  it("returns -1 when nothing matches", () => {
    const b = new RoaringBitmap64();
    b.addMany([1n, 2n]);
    expect(b.findIndex((v) => v > 99n)).toBe(-1);
  });

  it("throws TypeError when predicate is not a function", () => {
    expect(() => (new RoaringBitmap64() as any).findIndex(undefined)).toThrow(TypeError);
  });
});

describe("RoaringBitmap64.indexOf / lastIndexOf", () => {
  it("indexOf returns rank of value when present, -1 otherwise", () => {
    const b = new RoaringBitmap64();
    b.addMany([5n, 10n, 2n ** 40n]);
    expect(b.indexOf(5n)).toBe(0);
    expect(b.indexOf(10n)).toBe(1);
    expect(b.indexOf(2n ** 40n)).toBe(2);
    expect(b.indexOf(99n)).toBe(-1);
  });

  it("indexOf rejects non-bigint value with -1 (matches has() semantics)", () => {
    const b = new RoaringBitmap64();
    b.add(5n);
    expect(b.indexOf("5" as any)).toBe(-1);
  });

  it("lastIndexOf behaves like indexOf for unique-value sets", () => {
    const b = new RoaringBitmap64();
    b.addMany([5n, 10n, 2n ** 40n]);
    expect(b.lastIndexOf(10n)).toBe(1);
    expect(b.lastIndexOf(2n ** 40n)).toBe(2);
    expect(b.lastIndexOf(99n)).toBe(-1);
  });
});

describe("RoaringBitmap64.at", () => {
  it("returns the value at the given zero-based index", () => {
    const b = new RoaringBitmap64();
    b.addMany([5n, 10n, 2n ** 40n]);
    expect(b.at(0)).toBe(5n);
    expect(b.at(1)).toBe(10n);
    expect(b.at(2)).toBe(2n ** 40n);
  });

  it("supports negative indices counting from the end", () => {
    const b = new RoaringBitmap64();
    b.addMany([5n, 10n, 2n ** 40n]);
    expect(b.at(-1)).toBe(2n ** 40n);
    expect(b.at(-3)).toBe(5n);
  });

  it("returns undefined for out-of-bounds indices", () => {
    const b = new RoaringBitmap64();
    b.addMany([5n, 10n]);
    expect(b.at(2)).toBeUndefined();
    expect(b.at(-3)).toBeUndefined();
    expect(b.at(0.5)).toBeUndefined();
  });

  it("returns undefined on empty bitmap", () => {
    expect(new RoaringBitmap64().at(0)).toBeUndefined();
  });
});
