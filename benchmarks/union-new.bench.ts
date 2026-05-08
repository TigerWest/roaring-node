import FastBitSet from "fastbitset";
import { bench, describe } from "vitest";
import roaringModule from "../index.js";
import { consume } from "./utils";

const { RoaringBitmap32, RoaringBitmap64 } = roaringModule;
const N = 1024 * 1024;

const setA = new Set<number>();
const setB = new Set<number>();
const fastA = new FastBitSet();
const fastB = new FastBitSet();
const roaringA = new RoaringBitmap32();
const roaringB = new RoaringBitmap32();

const setA64 = new Set<bigint>();
const setB64 = new Set<bigint>();
const roaring64A = new RoaringBitmap64();
const roaring64B = new RoaringBitmap64();

for (let i = 0; i < N; i++) {
  const valueA = 3 * i + 5;
  const valueB = 6 * i + 5;
  setA.add(valueA);
  setB.add(valueB);
  fastA.add(valueA);
  fastB.add(valueB);
  roaringA.add(valueA);
  roaringB.add(valueB);

  const valueA64 = BigInt(valueA);
  const valueB64 = BigInt(valueB);
  setA64.add(valueA64);
  setB64.add(valueB64);
  roaring64A.add(valueA64);
  roaring64B.add(valueB64);
}

describe("union (new)", () => {
  bench("Set", () => {
    consume(genericSetUnion(setA, setB));
  });

  bench("Set<bigint>", () => {
    const answer = new Set<bigint>(setA64);
    for (const value of setB64) {
      answer.add(value);
    }
    consume(answer);
  });

  bench("FastBitSet", () => {
    consume(fastA.new_union(fastB));
  });

  bench("RoaringBitmap32", () => {
    consume(RoaringBitmap32.or(roaringA, roaringB));
  });

  bench("RoaringBitmap64", () => {
    consume(RoaringBitmap64.or(roaring64A, roaring64B));
  });
});

function genericSetUnion(set1: Set<number>, set2: Set<number>) {
  const answer = new Set(set1);
  for (const value of set2) {
    answer.add(value);
  }
  return answer;
}
