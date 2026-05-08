import { describe, expect, it } from "vitest";
import { RoaringBitmap64 } from "../../";

describe("RoaringBitmap64.dispose", () => {
  it("isDisposed flips after dispose", () => {
    const b = new RoaringBitmap64();
    expect(b.isDisposed).toBe(false);
    b.dispose();
    expect(b.isDisposed).toBe(true);
  });

  it("is idempotent", () => {
    const b = new RoaringBitmap64();
    b.dispose();
    expect(() => b.dispose()).not.toThrow();
    expect(b.isDisposed).toBe(true);
  });

  it("post-dispose methods throw", () => {
    const b = new RoaringBitmap64();
    b.add(1n);
    b.dispose();
    expect(() => b.add(2n)).toThrow();
    expect(() => b.has(1n)).toThrow();
    expect(() => b.toArray()).toThrow();
    expect(() => b.serialize()).toThrow();
    expect(() => b.clone()).toThrow();
  });

  it("iterator throws when parent disposed mid-iteration", () => {
    const b = new RoaringBitmap64();
    b.addMany([1n, 2n, 3n]);
    const it = b[Symbol.iterator]();
    expect(it.next().value).toBe(1n);
    b.dispose();
    expect(() => it.next()).toThrow();
  });
});
