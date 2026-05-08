import { bench, describe } from "vitest";
import roaringModule from "../index.js";

const { RoaringBitmap32, RoaringBitmap64 } = roaringModule;
const N = 65535;

const data = new Uint32Array(N);
for (let i = 0; i < N; i++) {
  data[i] = 3 * i + 5;
}
const dataArray = Array.from(data);

const data64 = new BigUint64Array(N);
for (let i = 0; i < N; i++) {
  data64[i] = BigInt(3 * i + 5);
}
const dataArray64 = Array.from(data64);

describe("add", () => {
  bench("Set.add", () => {
    const x = new Set<number>();
    for (let i = 0; i < N; ++i) {
      x.add(data[i]);
    }
  });

  bench("Set<bigint>.add", () => {
    const x = new Set<bigint>();
    for (let i = 0; i < N; ++i) {
      x.add(data64[i]);
    }
  });

  bench("RoaringBitmap32.tryAdd", () => {
    const x = new RoaringBitmap32();
    for (let i = 0; i < N; ++i) {
      x.tryAdd(data[i]);
    }
  });

  bench("RoaringBitmap64.tryAdd", () => {
    const x = new RoaringBitmap64();
    for (let i = 0; i < N; ++i) {
      x.tryAdd(data64[i]);
    }
  });

  bench("RoaringBitmap32.add", () => {
    const x = new RoaringBitmap32();
    for (let i = 0; i < N; ++i) {
      x.add(data[i]);
    }
  });

  bench("RoaringBitmap64.add", () => {
    const x = new RoaringBitmap64();
    for (let i = 0; i < N; ++i) {
      x.add(data64[i]);
    }
  });

  bench("RoaringBitmap32.addMany Array", () => {
    const x = new RoaringBitmap32();
    x.addMany(dataArray);
  });

  bench("RoaringBitmap64.addMany bigint[]", () => {
    const x = new RoaringBitmap64();
    x.addMany(dataArray64);
  });

  bench("RoaringBitmap32.addMany Uint32Array", () => {
    const x = new RoaringBitmap32();
    x.addMany(data);
  });

  bench("RoaringBitmap64.addMany BigUint64Array", () => {
    const x = new RoaringBitmap64();
    x.addMany(data64);
  });
});
