import { describe, expect, it } from "vitest";
import { RoaringBitmap64 } from "../../";

const buildBytes = (): Buffer => {
  const b = new RoaringBitmap64();
  b.addMany([0n, 1n, 2n ** 33n, 2n ** 50n, 2n ** 64n - 1n]);
  return b.serialize();
};

describe("RoaringBitmap64.deserialize buffer shapes", () => {
  const reference = buildBytes();
  const expected = [0n, 1n, 2n ** 33n, 2n ** 50n, 2n ** 64n - 1n];

  it("accepts Buffer", () => {
    expect(RoaringBitmap64.deserialize(reference).toArray()).toEqual(expected);
  });

  it("accepts Uint8Array", () => {
    const u8 = new Uint8Array(reference);
    expect(RoaringBitmap64.deserialize(u8).toArray()).toEqual(expected);
  });

  it("accepts ArrayBuffer", () => {
    const ab = reference.buffer.slice(reference.byteOffset, reference.byteOffset + reference.byteLength);
    expect(RoaringBitmap64.deserialize(ab).toArray()).toEqual(expected);
  });

  it("accepts DataView", () => {
    const ab = reference.buffer.slice(reference.byteOffset, reference.byteOffset + reference.byteLength);
    const dv = new DataView(ab);
    expect(RoaringBitmap64.deserialize(dv).toArray()).toEqual(expected);
  });

  it("rejects unsupported types with TypeError", () => {
    expect(() => (RoaringBitmap64 as any).deserialize("not a buffer")).toThrow(TypeError);
  });

  it("throws on truncated buffer without crashing", () => {
    const truncated = reference.subarray(0, Math.max(1, reference.length - 4));
    expect(() => RoaringBitmap64.deserialize(truncated)).toThrow();
    expect(new RoaringBitmap64().size).toBe(0n);
  });

  it("getDeserializationSize returns the consumed length, 0 on invalid", () => {
    expect(RoaringBitmap64.getDeserializationSize(reference)).toBe(BigInt(reference.length));
    expect(RoaringBitmap64.getDeserializationSize(Buffer.from([1, 2, 3]))).toBe(0n);
  });

  it("instance deserialize reuses the same instance", () => {
    const b = new RoaringBitmap64();
    b.add(99n);
    const result = b.deserialize(reference);
    expect(result).toBe(b);
    expect(b.toArray()).toEqual(expected);
  });
});
