import FastBitSet from "fastbitset";
import { bench, describe } from "vitest";
import roaringModule from "../index.js";
import { consume } from "./utils";

const { RoaringBitmap32, RoaringBitmap64 } = roaringModule;
const N = 512 * 512;

const left = new Uint32Array(N);
const right = new Uint32Array(N);
for (let i = 0; i < N; i++) {
  left[i] = 3 * i + 5;
  right[i] = 6 * i + 5;
}

const setLeft = new Set(left);
const setRight = new Set(right);
const fastLeft = new FastBitSet(left);
const fastRight = new FastBitSet(right);
const roaringLeft = new RoaringBitmap32(left);
const roaringRight = new RoaringBitmap32(right);

const left64 = new BigUint64Array(N);
const right64 = new BigUint64Array(N);
for (let i = 0; i < N; i++) {
  left64[i] = BigInt(3 * i + 5);
  right64[i] = BigInt(6 * i + 5);
}
const setLeft64 = new Set<bigint>(left64);
const setRight64 = new Set<bigint>(right64);
const roaring64Left = new RoaringBitmap64();
roaring64Left.addMany(left64);
const roaring64Right = new RoaringBitmap64();
roaring64Right.addMany(right64);

describe("intersection size", () => {
  bench("Set", () => {
    let answer = 0;
    if (setRight.size > setLeft.size) {
      for (const value of setLeft) {
        if (setRight.has(value)) {
          answer++;
        }
      }
    } else {
      for (const value of setRight) {
        if (setLeft.has(value)) {
          answer++;
        }
      }
    }
    consume(answer);
  });

  bench("Set<bigint>", () => {
    let answer = 0n;
    const [smaller, larger] = setLeft64.size <= setRight64.size ? [setLeft64, setRight64] : [setRight64, setLeft64];
    for (const value of smaller) {
      if (larger.has(value)) {
        answer++;
      }
    }
    consume(answer);
  });

  bench("FastBitSet", () => {
    consume(fastLeft.intersection_size(fastRight));
  });

  bench("RoaringBitmap32", () => {
    consume(roaringLeft.andCardinality(roaringRight));
  });

  bench("RoaringBitmap64", () => {
    consume(RoaringBitmap64.andCardinality(roaring64Left, roaring64Right));
  });
});
