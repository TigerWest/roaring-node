import { describe, expect, it } from "vitest";
import { RoaringBitmap32, RoaringBitmap64 } from "../../";

describe("RoaringBitmap64.fromRoaring32", () => {
  it("returns an empty RB64 from an empty RB32", () => {
    const r = RoaringBitmap64.fromRoaring32(new RoaringBitmap32());
    expect(r.size).toBe(0n);
  });

  it("preserves a small RB32", () => {
    const a = new RoaringBitmap32([1, 2, 3, 100, 1_000_000]);
    const r = RoaringBitmap64.fromRoaring32(a);
    expect(r.size).toBe(5n);
    for (const v of [1n, 2n, 3n, 100n, 1_000_000n]) expect(r.has(v)).toBe(true);
  });

  it("preserves a large RB32 (>= 100k values)", () => {
    const a = new RoaringBitmap32();
    for (let i = 0; i < 100_000; i++) a.add(i);
    a.add(0xffff_fffe);
    const r = RoaringBitmap64.fromRoaring32(a);
    expect(r.size).toBe(100_001n);
    expect(r.has(0n)).toBe(true);
    expect(r.has(99_999n)).toBe(true);
    expect(r.has(0xffff_fffen)).toBe(true);
  });

  it("rejects non-RoaringBitmap32 with TypeError", () => {
    expect(() => (RoaringBitmap64 as any).fromRoaring32({})).toThrow(TypeError);
    expect(() => (RoaringBitmap64 as any).fromRoaring32(null)).toThrow(TypeError);
  });

  it("rejects RoaringBitmap64 input with TypeError", () => {
    const b = new RoaringBitmap64();
    expect(() => (RoaringBitmap64 as any).fromRoaring32(b)).toThrow(TypeError);
  });
});
