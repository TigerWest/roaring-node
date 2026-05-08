import { describe, expect, it } from "vitest";
import { RoaringBitmap64 } from "../../";

describe("RoaringBitmap64 Symbol.iterator", () => {
  it("for-of yields ascending bigint values", () => {
    const b = new RoaringBitmap64();
    b.addMany([10n, 1n, 2n ** 40n]);
    const out: bigint[] = [];
    for (const v of b) out.push(v);
    expect(out).toEqual([1n, 10n, 2n ** 40n]);
  });

  it("empty bitmap yields nothing", () => {
    const b = new RoaringBitmap64();
    expect(Array.from(b)).toEqual([]);
  });
});
