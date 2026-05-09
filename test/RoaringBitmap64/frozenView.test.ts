import { describe, expect, it } from "vitest";
import { bufferAlignedAllocUnsafe, RoaringBitmap64 } from "../..";

function freeze(rb: RoaringBitmap64): Buffer {
  const size = Number(rb.getSerializationSizeInBytes("unsafe_frozen_croaring"));
  const out: Buffer = bufferAlignedAllocUnsafe(size, 64);
  const written = rb.serialize("unsafe_frozen_croaring");
  out.set(new Uint8Array(written.buffer, written.byteOffset, written.byteLength));
  return out;
}

describe("RoaringBitmap64.unsafeFrozenView", () => {
  it("round-trips contents", () => {
    const src = new RoaringBitmap64([1n, 2n, 3n, 0xffffffffn, 1n << 40n]);
    const buf = freeze(src);
    const view = RoaringBitmap64.unsafeFrozenView("unsafe_frozen_croaring", buf);
    expect(view.size).toBe(src.size);
    expect([...view]).toEqual([...src]);
  });

  it("isFrozen=true on the view", () => {
    const src = new RoaringBitmap64([1n]);
    const view = RoaringBitmap64.unsafeFrozenView("unsafe_frozen_croaring", freeze(src));
    expect(view.isFrozen).toBe(true);
  });

  it("mutating the view throws", () => {
    const src = new RoaringBitmap64([1n]);
    const view = RoaringBitmap64.unsafeFrozenView("unsafe_frozen_croaring", freeze(src));
    expect(() => view.add(2n)).toThrow(/frozen/);
    expect(() => view.clear()).toThrow(/frozen/);
    expect(() => view.addRange(0n, 10n)).toThrow(/frozen/);
  });

  it("rejects unsafe_frozen_portable (no 64-bit C API)", () => {
    const src = new RoaringBitmap64([1n]);
    expect(() => RoaringBitmap64.unsafeFrozenView("unsafe_frozen_portable" as any, freeze(src))).toThrow(/portable/);
  });

  it("rejects an unaligned buffer", () => {
    const src = new RoaringBitmap64([1n]);
    const aligned = freeze(src);
    const misaligned = Buffer.from(aligned.buffer, aligned.byteOffset + 1, aligned.byteLength - 1);
    expect(() => RoaringBitmap64.unsafeFrozenView("unsafe_frozen_croaring", misaligned)).toThrow(/align/);
  });

  it("survives GC of the original buffer reference", () => {
    let buf: Buffer | undefined = (() => {
      const src = new RoaringBitmap64([10n, 20n, 30n, 1n << 40n]);
      return freeze(src);
    })();
    const view = RoaringBitmap64.unsafeFrozenView("unsafe_frozen_croaring", buf!);
    buf = undefined;
    if (typeof (globalThis as any).gc === "function") (globalThis as any).gc();
    expect([...view]).toEqual([10n, 20n, 30n, 1n << 40n]);
  });
});

describe("RoaringBitmap64.unsafeFrozenView argument order parity with RB32", () => {
  it("accepts (storage, format) like RB32", () => {
    const src = new RoaringBitmap64([1n, 2n, 3n]);
    const buf = freeze(src);
    const view = RoaringBitmap64.unsafeFrozenView(buf, "unsafe_frozen_croaring");
    expect(view.size).toBe(3n);
    expect(view.has(2n)).toBe(true);
  });

  it("still accepts (format, storage) (no regression)", () => {
    const src = new RoaringBitmap64([7n, 8n]);
    const buf = freeze(src);
    const view = RoaringBitmap64.unsafeFrozenView("unsafe_frozen_croaring", buf);
    expect(view.size).toBe(2n);
    expect(view.has(7n)).toBe(true);
  });
});
