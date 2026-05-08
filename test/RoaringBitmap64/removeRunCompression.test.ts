import { describe, expect, it } from "vitest";
import { RoaringBitmap64 } from "../../";

describe("RoaringBitmap64.removeRunCompression", () => {
  it("returns false on an empty bitmap", () => {
    const b = new RoaringBitmap64();
    expect(b.removeRunCompression()).toBe(false);
    expect(b.statistics().runContainers).toBe(0);
  });

  it("returns false when no run containers exist", () => {
    const b = new RoaringBitmap64();
    b.add(1n);
    b.add(1_000_000n);
    b.add(1n << 40n);
    // Sparse data — runOptimize would not create runs either, but be
    // defensive: the bitmap should not have run containers.
    expect(b.statistics().runContainers).toBe(0);
    expect(b.removeRunCompression()).toBe(false);
  });

  it("returns true when run containers exist and removes them", () => {
    const b = new RoaringBitmap64();
    for (let i = 0n; i < 100_000n; i++) b.add(i);
    expect(b.runOptimize()).toBe(true);
    expect(b.statistics().runContainers).toBeGreaterThan(0);
    const valuesBefore = b.toArray();
    expect(b.removeRunCompression()).toBe(true);
    expect(b.statistics().runContainers).toBe(0);
    // Values are preserved exactly.
    expect(b.toArray()).toEqual(valuesBefore);
  });

  it("invalidates active iterators when it returns true", () => {
    const b = new RoaringBitmap64();
    for (let i = 0n; i < 100_000n; i++) b.add(i);
    b.runOptimize();
    const it = b[Symbol.iterator]();
    expect(it.next().value).toBe(0n);
    expect(b.removeRunCompression()).toBe(true);
    expect(() => it.next()).toThrow(/mutated/);
  });

  it("throws when bitmap is disposed", () => {
    const b = new RoaringBitmap64();
    b.dispose();
    expect(() => b.removeRunCompression()).toThrow();
  });
});
