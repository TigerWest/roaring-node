import { describe, expect, it } from "vitest";
import { RoaringBitmap64 } from "../../";

describe("RoaringBitmap64.runOptimize", () => {
  it("returns true when at least one container becomes a run", () => {
    const b = new RoaringBitmap64();
    b.addRange(0n, 100_000n); // dense run-friendly data
    expect(b.runOptimize()).toBe(true);
  });

  it("returns false when no run conversion is possible", () => {
    const b = new RoaringBitmap64();
    b.add(1n);
    b.add(1_000_000n);
    b.add(1n << 40n);
    expect(b.runOptimize()).toBe(false);
  });

  it("shrinks portable serialization size for run-friendly data", () => {
    // Build up dense data via individual add() calls so containers start as
    // arrays/bitsets rather than runs (addRange already creates run-form
    // containers in CRoaring 64-bit).
    const b = new RoaringBitmap64();
    for (let i = 0n; i < 100_000n; i++) b.add(i);
    const sizeBefore = b.getSerializationSizeInBytes();
    b.runOptimize();
    const sizeAfter = b.getSerializationSizeInBytes();
    expect(sizeAfter).toBeLessThan(sizeBefore);
  });

  it("throws when bitmap is disposed", () => {
    const b = new RoaringBitmap64();
    b.dispose();
    expect(() => b.runOptimize()).toThrow();
  });

  it("does NOT invalidate active iterators when no container changed (returns false)", () => {
    const b = new RoaringBitmap64();
    b.add(1n);
    b.add(1_000_000n);
    b.add(1n << 40n);
    const it = b[Symbol.iterator]();
    expect(it.next().value).toBe(1n);
    expect(b.runOptimize()).toBe(false);
    // Iterator must still work because container layout did not change.
    expect(it.next().value).toBe(1_000_000n);
    expect(it.next().value).toBe(1n << 40n);
    expect(it.next().done).toBe(true);
  });

  it("invalidates active iterators when at least one container changed (returns true)", () => {
    const b = new RoaringBitmap64();
    for (let i = 0n; i < 1000n; i++) b.add(i);
    const it = b[Symbol.iterator]();
    expect(it.next().value).toBe(0n);
    expect(b.runOptimize()).toBe(true);
    expect(() => it.next()).toThrow(/mutated/);
  });
});
