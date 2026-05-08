import { describe, expect, it } from "vitest";
import { RoaringBitmap64 } from "../../";

describe("RoaringBitmap64.every", () => {
  it("returns true when predicate matches every value", () => {
    const b = new RoaringBitmap64();
    b.addMany([2n, 4n, 6n]);
    expect(b.every((v) => v % 2n === 0n)).toBe(true);
  });

  it("returns false on first mismatch", () => {
    const b = new RoaringBitmap64();
    b.addMany([2n, 3n, 4n]);
    let calls = 0;
    expect(
      b.every((v) => {
        calls++;
        return v % 2n === 0n;
      }),
    ).toBe(false);
    expect(calls).toBe(2);
  });

  it("returns true on empty bitmap", () => {
    expect(new RoaringBitmap64().every(() => false)).toBe(true);
  });

  it("throws TypeError when predicate is not a function", () => {
    expect(() => (new RoaringBitmap64() as any).every(0)).toThrow(TypeError);
  });
});

describe("RoaringBitmap64.some", () => {
  it("returns true on first match", () => {
    const b = new RoaringBitmap64();
    b.addMany([1n, 2n, 3n]);
    let calls = 0;
    expect(
      b.some((v) => {
        calls++;
        return v === 2n;
      }),
    ).toBe(true);
    expect(calls).toBe(2);
  });

  it("returns false when nothing matches", () => {
    const b = new RoaringBitmap64();
    b.addMany([1n, 2n]);
    expect(b.some((v) => v === 99n)).toBe(false);
  });

  it("returns false on empty bitmap", () => {
    expect(new RoaringBitmap64().some(() => true)).toBe(false);
  });

  it("throws TypeError when predicate is not a function", () => {
    expect(() => (new RoaringBitmap64() as any).some(null)).toThrow(TypeError);
  });
});
