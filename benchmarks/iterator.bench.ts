import { bench, describe } from "vitest";
import roaringModule from "../index.js";
import { consume } from "./utils";

const { RoaringBitmap32, RoaringBitmap64 } = roaringModule;
const N = 65536;

const data = new Uint32Array(N);
for (let i = 0; i < N; i++) {
  data[i] = 3 * i + 5;
}

const setData = new Set(data);
const roaringData = new RoaringBitmap32(data);

const data64 = new BigUint64Array(N);
for (let i = 0; i < N; i++) {
  data64[i] = BigInt(3 * i + 5);
}
const setData64 = new Set<bigint>(data64);
const roaring64Data = new RoaringBitmap64();
roaring64Data.addMany(data64);

describe("iterator", () => {
  bench("Set.iterator", () => {
    let total = 0;
    for (const value of setData) {
      total ^= value;
    }
    consume(total);
  });

  bench("Set<bigint>.iterator", () => {
    let total = 0n;
    for (const value of setData64) {
      total ^= value;
    }
    consume(total);
  });

  bench("Set.forEach", () => {
    let total = 0;
    setData.forEach((value) => {
      total ^= value;
    });
    consume(total);
  });

  bench("RoaringBitmap32.iterator", () => {
    let total = 0;
    for (const value of roaringData) {
      total ^= value;
    }
    consume(total);
  });

  bench("RoaringBitmap64.iterator", () => {
    let total = 0n;
    for (const value of roaring64Data) {
      total ^= value;
    }
    consume(total);
  });

  bench("RoaringBitmap32.forEach", () => {
    let total = 0;
    roaringData.forEach((value) => {
      total ^= value;
    });
    consume(total);
  });

  bench("RoaringBitmap64.toUint64Array + loop", () => {
    const arr = roaring64Data.toUint64Array();
    let total = 0n;
    for (let i = 0; i < arr.length; ++i) {
      total ^= arr[i];
    }
    consume(total);
  });

  bench("RoaringBitmap64.toArray + loop", () => {
    const arr = roaring64Data.toArray();
    let total = 0n;
    for (let i = 0; i < arr.length; ++i) {
      total ^= arr[i];
    }
    consume(total);
  });
});
