import { describe, expect, it } from "vitest";
import { bufferAlignedAllocUnsafe, RoaringBitmap64 } from "../../";

function bm(...vals: bigint[]): RoaringBitmap64 {
  const b = new RoaringBitmap64();
  for (const v of vals) b.add(v);
  return b;
}

function freezeRB64(rb: RoaringBitmap64) {
  const size = Number(rb.getSerializationSizeInBytes("unsafe_frozen_croaring"));
  const out = bufferAlignedAllocUnsafe(size, 64);
  const written = rb.serialize("unsafe_frozen_croaring");
  out.set(new Uint8Array(written.buffer, written.byteOffset, written.byteLength));
  return out;
}

describe("RoaringBitmap64.swap", () => {
  it("exchanges contents in place", () => {
    const a = bm(1n, 2n, 3n);
    const b = bm(100n, 200n);
    RoaringBitmap64.swap(a, b);
    expect(a.toArray()).toEqual([100n, 200n]);
    expect(b.toArray()).toEqual([1n, 2n, 3n]);
  });

  it("preserves JS object identity", () => {
    const a = bm(1n);
    const b = bm(2n);
    const aRef = a;
    const bRef = b;
    RoaringBitmap64.swap(a, b);
    expect(a).toBe(aRef);
    expect(b).toBe(bRef);
  });

  it("invalidates iterators (version bump)", () => {
    const a = bm(1n, 2n, 3n);
    const b = bm(10n, 20n, 30n);
    const it = a[Symbol.iterator]();
    expect(it.next().value).toBe(1n);
    RoaringBitmap64.swap(a, b);
    let nextResult: IteratorResult<bigint>;
    try {
      nextResult = it.next();
    } catch {
      return;
    }
    if (!nextResult.done) {
      expect([10n, 20n, 30n]).not.toContain(nextResult.value);
    }
  });

  it("returns undefined", () => {
    const a = bm(1n);
    const b = bm(2n);
    expect(RoaringBitmap64.swap(a, b)).toBeUndefined();
  });

  it("is a no-op for swap(a, a)", () => {
    const a = bm(1n, 2n);
    RoaringBitmap64.swap(a, a);
    expect(a.toArray()).toEqual([1n, 2n]);
  });

  it("throws TypeError on non-RoaringBitmap64 args", () => {
    const a = bm(1n);
    expect(() => (RoaringBitmap64 as any).swap(a, {})).toThrow(TypeError);
    expect(() => (RoaringBitmap64 as any).swap({}, a)).toThrow(TypeError);
    expect(() => (RoaringBitmap64 as any).swap(a)).toThrow(TypeError);
  });

  it("throws on a frozen-view operand", () => {
    const a = bm(1n, 2n, 3n);
    const frozen = RoaringBitmap64.unsafeFrozenView("unsafe_frozen_croaring", freezeRB64(a));
    expect(() => RoaringBitmap64.swap(a, frozen)).toThrow();
    expect(() => RoaringBitmap64.swap(frozen, a)).toThrow();
  });
});
