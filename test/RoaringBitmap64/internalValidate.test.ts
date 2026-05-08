import { describe, expect, it } from "vitest";
import { RoaringBitmap64 } from "../../";

describe("RoaringBitmap64.internalValidate", () => {
  it("returns void (does not throw) on a clean bitmap", () => {
    const b = new RoaringBitmap64();
    b.addRange(0n, 100n);
    expect(b.internalValidate()).toBeUndefined();
  });

  it("does not throw on an empty bitmap", () => {
    const b = new RoaringBitmap64();
    expect(() => b.internalValidate()).not.toThrow();
  });

  it("does not throw after deserializing a bitmap that was just serialized", () => {
    const b = new RoaringBitmap64();
    b.addRange(0n, 1000n);
    b.add(1n << 40n);
    const buf = b.serialize();
    const r = RoaringBitmap64.deserialize(buf);
    expect(() => r.internalValidate()).not.toThrow();
  });

  it("throws when bitmap is disposed", () => {
    const b = new RoaringBitmap64();
    b.dispose();
    expect(() => b.internalValidate()).toThrow();
  });
});
