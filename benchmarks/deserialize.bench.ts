import { bench, describe } from "vitest";
import roaringModule from "../index.js";
import { consume } from "./utils";

const { RoaringBitmap32, RoaringBitmap64 } = roaringModule;

const sizes: Array<[string, number]> = [
  ["small (1k)", 1_000],
  ["medium (100k)", 100_000],
  ["large (1M)", 1_000_000],
];

const buffers32: Record<string, Buffer> = {};
const buffers64: Record<string, Buffer> = {};
for (const [name, n] of sizes) {
  const data32 = new Uint32Array(n);
  for (let i = 0; i < n; ++i) data32[i] = 7 * i;
  const b32 = new RoaringBitmap32(data32);
  buffers32[name] = b32.serialize("portable") as unknown as Buffer;

  const data64 = new BigUint64Array(n);
  for (let i = 0; i < n; ++i) data64[i] = BigInt(i) * 7n;
  const b64 = new RoaringBitmap64();
  b64.addMany(data64);
  buffers64[name] = b64.serialize();
}

for (const [name] of sizes) {
  describe(`deserialize ${name}`, () => {
    const buf32 = buffers32[name];
    const buf64 = buffers64[name];
    bench("RoaringBitmap32", () => {
      consume(RoaringBitmap32.deserialize(buf32, "portable"));
    });
    bench("RoaringBitmap64", () => {
      consume(RoaringBitmap64.deserialize(buf64));
    });
  });
}
