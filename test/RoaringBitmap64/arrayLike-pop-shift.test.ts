import { describe, expect, it } from "vitest";
import { RoaringBitmap64 } from "../../";

describe("RoaringBitmap64.pop", () => {
  it("removes and returns the maximum value", () => {
    const b = new RoaringBitmap64();
    b.addMany([1n, 5n, 2n ** 50n]);
    expect(b.pop()).toBe(2n ** 50n);
    expect(b.has(2n ** 50n)).toBe(false);
    expect(b.size).toBe(2n);
  });

  it("returns undefined on empty bitmap", () => {
    expect(new RoaringBitmap64().pop()).toBeUndefined();
  });

  it("draining a bitmap converges to undefined", () => {
    const b = new RoaringBitmap64();
    b.addMany([1n, 2n, 3n]);
    expect(b.pop()).toBe(3n);
    expect(b.pop()).toBe(2n);
    expect(b.pop()).toBe(1n);
    expect(b.pop()).toBeUndefined();
  });
});

describe("RoaringBitmap64.shift", () => {
  it("removes and returns the minimum value", () => {
    const b = new RoaringBitmap64();
    b.addMany([1n, 5n, 2n ** 50n]);
    expect(b.shift()).toBe(1n);
    expect(b.has(1n)).toBe(false);
    expect(b.size).toBe(2n);
  });

  it("returns undefined on empty bitmap", () => {
    expect(new RoaringBitmap64().shift()).toBeUndefined();
  });

  it("draining a bitmap converges to undefined", () => {
    const b = new RoaringBitmap64();
    b.addMany([1n, 2n, 3n]);
    expect(b.shift()).toBe(1n);
    expect(b.shift()).toBe(2n);
    expect(b.shift()).toBe(3n);
    expect(b.shift()).toBeUndefined();
  });
});
