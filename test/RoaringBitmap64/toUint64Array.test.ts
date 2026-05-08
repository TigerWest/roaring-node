import { describe, expect, it } from "vitest";
import { RoaringBitmap64 } from "../../";

describe("RoaringBitmap64.toUint64Array", () => {
  it("returns an empty BigUint64Array for empty bitmap", () => {
    const b = new RoaringBitmap64();
    const out = b.toUint64Array();
    expect(out).toBeInstanceOf(BigUint64Array);
    expect(out.length).toBe(0);
  });

  it("returns sorted ascending values", () => {
    const data = new BigUint64Array([2n ** 50n, 1n, 0n, 2n ** 32n]);
    const b = new RoaringBitmap64();
    b.addMany(data);
    const out = b.toUint64Array();
    expect(Array.from(out)).toEqual([0n, 1n, 2n ** 32n, 2n ** 50n]);
  });

  it("survives a 1M element round-trip", () => {
    const N = 1_000_000;
    const data = new BigUint64Array(N);
    for (let i = 0; i < N; ++i) data[i] = BigInt(i) * 7n;
    const b = new RoaringBitmap64();
    b.addMany(data);
    const out = b.toUint64Array();
    expect(out.length).toBe(N);
    expect(out[0]).toBe(0n);
    expect(out[N - 1]).toBe(BigInt(N - 1) * 7n);
  });
});
