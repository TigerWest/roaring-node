import { describe, expect, it } from "vitest";
import { RoaringBitmap64 } from "../../";

describe("RoaringBitmap64.remove / delete", () => {
  it("remove returns this and is silent for missing values", () => {
    const b = new RoaringBitmap64();
    b.add(5n);
    expect(b.remove(5n)).toBe(b);
    expect(b.remove(99n)).toBe(b);
    expect(b.size).toBe(0n);
  });

  it("delete returns true when present, false otherwise", () => {
    const b = new RoaringBitmap64();
    b.add(2n ** 40n);
    expect(b.delete(2n ** 40n)).toBe(true);
    expect(b.delete(2n ** 40n)).toBe(false);
  });

  it("rejects non-BigInt", () => {
    const b = new RoaringBitmap64();
    expect(() => (b as any).remove(1)).toThrow(TypeError);
  });
});
