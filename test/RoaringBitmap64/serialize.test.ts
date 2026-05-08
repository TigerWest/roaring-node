import { describe, expect, it } from "vitest";
import { RoaringBitmap64 } from "../../";

describe("RoaringBitmap64.serialize", () => {
  it("produces a Buffer of the announced size", () => {
    const b = new RoaringBitmap64();
    b.addMany([1n, 2n, 2n ** 50n]);
    const announced = b.getSerializationSizeInBytes();
    const buf = b.serialize();
    expect(Buffer.isBuffer(buf)).toBe(true);
    expect(BigInt(buf.length)).toBe(announced);
  });

  it("serializes an empty bitmap to a non-empty header buffer", () => {
    const b = new RoaringBitmap64();
    const buf = b.serialize();
    expect(buf.length).toBeGreaterThan(0);
  });
});

describe("RoaringBitmap64 serialization round-trip", () => {
  const cases: Array<[string, bigint[]]> = [
    ["empty", []],
    ["singleton", [42n]],
    ["spans 2^32 boundary", [0n, 2n ** 32n - 1n, 2n ** 32n, 2n ** 33n]],
    ["very sparse", [0n, 2n ** 63n, 2n ** 64n - 1n]],
  ];

  it.each(cases)("%s round-trips", (_, vals) => {
    const a = new RoaringBitmap64();
    a.addMany(vals);
    const buf = a.serialize();
    const b = RoaringBitmap64.deserialize(buf);
    expect(b.equals(a)).toBe(true);
    const sorted = vals.slice().sort((x, y) => (x < y ? -1 : x > y ? 1 : 0));
    expect(b.toArray()).toEqual(sorted);
  });

  it("dense range 0..1M round-trips", () => {
    const N = 1_000_000;
    const data = new BigUint64Array(N);
    for (let i = 0; i < N; ++i) data[i] = BigInt(i);
    const a = new RoaringBitmap64();
    a.addMany(data);
    const buf = a.serialize();
    const b = RoaringBitmap64.deserialize(buf);
    expect(b.size).toBe(BigInt(N));
    expect(b.equals(a)).toBe(true);
  });
});
