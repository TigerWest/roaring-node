import { describe, expect, it } from "vitest";
import { RoaringBitmap64, RoaringBitmap64ReverseIterator } from "../..";

describe("RoaringBitmap64ReverseIterator", () => {
  it("yields values in strictly decreasing order", () => {
    const rb = new RoaringBitmap64([1n, 2n, 3n, 0xffffffffn, 1n << 40n]);
    const got: bigint[] = [];
    for (const v of new RoaringBitmap64ReverseIterator(rb)) {
      got.push(v);
    }
    expect(got).toEqual([1n << 40n, 0xffffffffn, 3n, 2n, 1n]);
  });

  it("works on empty bitmap", () => {
    const rb = new RoaringBitmap64();
    const it = new RoaringBitmap64ReverseIterator(rb);
    expect(it.next()).toEqual({ value: undefined, done: true });
  });

  it("works on single-element bitmap", () => {
    const rb = new RoaringBitmap64([42n]);
    const it = new RoaringBitmap64ReverseIterator(rb);
    expect(it.next()).toEqual({ value: 42n, done: false });
    expect(it.next()).toEqual({ value: undefined, done: true });
  });

  it("throws if parent is mutated mid-iteration", () => {
    const rb = new RoaringBitmap64([1n, 2n, 3n]);
    const it = new RoaringBitmap64ReverseIterator(rb);
    it.next();
    rb.add(99n);
    expect(() => it.next()).toThrow(/mutated/);
  });

  it("throws if parent is disposed mid-iteration", () => {
    const rb = new RoaringBitmap64([1n, 2n, 3n]);
    const it = new RoaringBitmap64ReverseIterator(rb);
    rb.dispose();
    expect(() => it.next()).toThrow(/disposed/);
  });

  it("rb.reverseIterator() returns a working iterator", () => {
    const rb = new RoaringBitmap64([10n, 20n, 30n]);
    expect([...rb.reverseIterator()]).toEqual([30n, 20n, 10n]);
  });

  it("forward + reverse round-trip yields the same multiset", () => {
    const values = [0n, 1n, 0xffffffffn, (1n << 40n) - 1n, 1n << 63n, (1n << 64n) - 1n];
    const rb = new RoaringBitmap64(values);
    const fwd = [...rb];
    const rev = [...rb.reverseIterator()];
    expect(rev).toEqual([...fwd].reverse());
  });

  it("default-import alias works", async () => {
    const Mod = (await import("../../RoaringBitmap64ReverseIterator.js")).default;
    const rb = new RoaringBitmap64([5n, 4n, 3n]);
    expect([...new Mod(rb)]).toEqual([5n, 4n, 3n]);
  });
});
