import { describe, expect, it } from "vitest";
import { RoaringBitmap64, RoaringBitmap64Iterator } from "../../";

describe("RoaringBitmap64Iterator", () => {
  it("next() returns IteratorResult shape", () => {
    const b = new RoaringBitmap64();
    b.add(7n);
    const it = b[Symbol.iterator]();
    expect(it).toBeInstanceOf(RoaringBitmap64Iterator);
    expect(it.next()).toEqual({ value: 7n, done: false });
    expect(it.next()).toEqual({ value: undefined, done: true });
  });

  it("is itself iterable (Symbol.iterator returns this)", () => {
    const b = new RoaringBitmap64();
    b.add(1n);
    const it = b[Symbol.iterator]();
    expect((it as any)[Symbol.iterator]()).toBe(it);
  });
});
