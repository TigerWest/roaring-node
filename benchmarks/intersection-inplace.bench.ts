import FastBitSet from "fastbitset";
import { bench, describe } from "vitest";
import roaringModule from "../index.js";

const { RoaringBitmap32, RoaringBitmap64 } = roaringModule;
const N = 256 * 256;

const left = new Uint32Array(N);
const right = new Uint32Array(N);
for (let i = 0; i < N; i++) {
  left[i] = 3 * i + 5;
  right[i] = 6 * i + 5;
}

const setLeft = new Set(left);
const fastLeft = new FastBitSet(left);
const roaringLeft = new RoaringBitmap32(left);

const left64 = new BigUint64Array(N);
const right64 = new BigUint64Array(N);
for (let i = 0; i < N; i++) {
  left64[i] = BigInt(3 * i + 5);
  right64[i] = BigInt(6 * i + 5);
}
const setLeft64 = new Set<bigint>(left64);

describe("intersection (in place)", () => {
  bench("Set", () => {
    const target = new Set(right);
    for (const value of setLeft) {
      target.delete(value);
    }
  });

  bench("Set<bigint>", () => {
    const target = new Set<bigint>(right64);
    for (const value of setLeft64) {
      target.delete(value);
    }
  });

  bench("FastBitSet", () => {
    const target = new FastBitSet(right);
    target.intersection(fastLeft);
  });

  bench("RoaringBitmap32", () => {
    const target = new RoaringBitmap32(right);
    target.andInPlace(roaringLeft);
  });

  bench("RoaringBitmap64", () => {
    const target = new RoaringBitmap64();
    target.addMany(right64);
    const source = new RoaringBitmap64();
    source.addMany(left64);
    target.andInPlace(source);
  });
});
