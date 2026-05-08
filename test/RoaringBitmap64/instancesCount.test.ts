import { describe, expect, it } from "vitest";
import { RoaringBitmap32, RoaringBitmap64 } from "../../";

describe("RoaringBitmap64.getInstancesCount", () => {
  it("returns a non-negative integer number (not bigint)", () => {
    const n = RoaringBitmap64.getInstancesCount();
    expect(typeof n).toBe("number");
    expect(Number.isInteger(n)).toBe(true);
    expect(n).toBeGreaterThanOrEqual(0);
  });

  it("increases by 1 when a new RoaringBitmap64 is constructed", () => {
    const before = RoaringBitmap64.getInstancesCount();
    const b = new RoaringBitmap64();
    const after = RoaringBitmap64.getInstancesCount();
    expect(after - before).toBe(1);
    expect(b).toBeInstanceOf(RoaringBitmap64);
    b.dispose();
  });

  it("never drops below baseline after dispose() (GC-eventual decrement)", () => {
    // RB64 mirrors RB32: the counter decrements only when the dtor runs,
    // which happens after V8 GC, not synchronously on dispose(). The
    // observable contract is therefore an upper bound, not equality.
    const baseline = RoaringBitmap64.getInstancesCount();
    const b = new RoaringBitmap64();
    expect(RoaringBitmap64.getInstancesCount()).toBe(baseline + 1);
    b.dispose();
    const after = RoaringBitmap64.getInstancesCount();
    expect(after).toBeGreaterThanOrEqual(baseline);
    expect(after).toBeLessThanOrEqual(baseline + 1);
  });

  it("clone() bumps the count", () => {
    const baseline = RoaringBitmap64.getInstancesCount();
    const a = new RoaringBitmap64();
    a.add(1n);
    expect(RoaringBitmap64.getInstancesCount()).toBe(baseline + 1);
    const b = a.clone();
    expect(RoaringBitmap64.getInstancesCount()).toBe(baseline + 2);
    a.dispose();
    b.dispose();
    // Post-dispose count is GC-bounded; only assert the upper bound.
    expect(RoaringBitmap64.getInstancesCount()).toBeLessThanOrEqual(baseline + 2);
  });

  it("is independent of RoaringBitmap32.getInstancesCount", () => {
    const before64 = RoaringBitmap64.getInstancesCount();
    const before32 = RoaringBitmap32.getInstancesCount();
    const a32 = new RoaringBitmap32();
    expect(RoaringBitmap64.getInstancesCount()).toBe(before64);
    expect(RoaringBitmap32.getInstancesCount()).toBe(before32 + 1);
    const a64 = new RoaringBitmap64();
    expect(RoaringBitmap64.getInstancesCount()).toBe(before64 + 1);
    expect(RoaringBitmap32.getInstancesCount()).toBe(before32 + 1);
    // Disposing RB32 must not move the RB64 counter, and vice versa.
    a32.dispose?.();
    expect(RoaringBitmap64.getInstancesCount()).toBe(before64 + 1);
    a64.dispose();
    expect(RoaringBitmap32.getInstancesCount()).toBeLessThanOrEqual(before32 + 1);
  });
});
