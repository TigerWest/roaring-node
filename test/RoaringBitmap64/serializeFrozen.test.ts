import { describe, expect, it } from "vitest";
import { RoaringBitmap64 } from "../..";

describe("RoaringBitmap64 frozen serialize", () => {
  it("getSerializationSizeInBytes('unsafe_frozen_croaring') matches serialize length", () => {
    const rb = new RoaringBitmap64([1n, 2n, 3n, 1n << 40n]);
    const size = rb.getSerializationSizeInBytes("unsafe_frozen_croaring");
    const buf = rb.serialize("unsafe_frozen_croaring");
    expect(BigInt(buf.length)).toBe(size);
  });

  it("portable format is the default and matches the no-arg call", () => {
    const rb = new RoaringBitmap64([1n, 2n, 3n]);
    const a = rb.serialize();
    const b = rb.serialize("portable");
    expect(Buffer.compare(a, b)).toBe(0);
  });

  it("rejects an invalid format string", () => {
    const rb = new RoaringBitmap64();
    expect(() => rb.serialize("bogus" as any)).toThrow(/format/);
  });

  it("rejects an invalid format string in getSerializationSizeInBytes", () => {
    const rb = new RoaringBitmap64();
    expect(() => rb.getSerializationSizeInBytes("bogus" as any)).toThrow(/format/);
  });
});
