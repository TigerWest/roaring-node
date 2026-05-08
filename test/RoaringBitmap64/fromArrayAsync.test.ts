import { describe, expect, it } from "vitest";
import { RoaringBitmap32, RoaringBitmap64 } from "../../";

function* gen(...vals: bigint[]): Generator<bigint> {
  for (const v of vals) yield v;
}

describe("RoaringBitmap64.fromArrayAsync", () => {
  it("returns Promise<RoaringBitmap64> when called with no args", async () => {
    const r = await RoaringBitmap64.fromArrayAsync();
    expect(r).toBeInstanceOf(RoaringBitmap64);
    expect(r.size).toBe(0n);
  });

  it("treats undefined / null as empty input", async () => {
    const ru = await RoaringBitmap64.fromArrayAsync(undefined as any);
    const rn = await RoaringBitmap64.fromArrayAsync(null as any);
    expect(ru.size).toBe(0n);
    expect(rn.size).toBe(0n);
  });

  it("accepts an empty BigUint64Array", async () => {
    const r = await RoaringBitmap64.fromArrayAsync(new BigUint64Array(0));
    expect(r.size).toBe(0n);
  });

  it("populates from a BigUint64Array (fast path)", async () => {
    const vals = new BigUint64Array([1n, 2n, 3n, 1n << 40n]);
    const r = await RoaringBitmap64.fromArrayAsync(vals);
    expect(r.size).toBe(4n);
    for (const v of [1n, 2n, 3n, 1n << 40n]) expect(r.has(v)).toBe(true);
    expect(Array.from(vals)).toEqual([1n, 2n, 3n, 1n << 40n]);
  });

  it("populates from an array of bigints", async () => {
    const r = await RoaringBitmap64.fromArrayAsync([1n, 100n, 1n << 40n]);
    expect(r.size).toBe(3n);
    expect(r.has(1n << 40n)).toBe(true);
  });

  it("populates from a generator", async () => {
    const r = await RoaringBitmap64.fromArrayAsync(gen(7n, 8n));
    expect(r.toArray()).toEqual([7n, 8n]);
  });

  it("populates from a Set (set semantics)", async () => {
    const r = await RoaringBitmap64.fromArrayAsync(new Set([1n, 2n, 3n, 1n]));
    expect(r.size).toBe(3n);
  });

  it("crosses the 2^32 boundary", async () => {
    const r = await RoaringBitmap64.fromArrayAsync([1n, 1n << 33n, (1n << 60n) + 1n]);
    expect(r.has(1n << 33n)).toBe(true);
    expect(r.has((1n << 60n) + 1n)).toBe(true);
  });

  it("invokes a node-style callback when provided (success)", async () => {
    await new Promise<void>((resolve, reject) => {
      const ret: any = (RoaringBitmap64 as any).fromArrayAsync([1n, 2n], (err: Error | null, b: any) => {
        try {
          expect(err).toBeNull();
          expect(b).toBeInstanceOf(RoaringBitmap64);
          expect(b.size).toBe(2n);
          resolve();
        } catch (e) {
          reject(e);
        }
      });
      expect(ret).toBeUndefined();
    });
  });

  it("invokes a callback with an empty bitmap when only a callback is passed", async () => {
    await new Promise<void>((resolve, reject) => {
      (RoaringBitmap64 as any).fromArrayAsync((err: Error | null, b: any) => {
        try {
          expect(err).toBeNull();
          expect(b).toBeInstanceOf(RoaringBitmap64);
          expect(b.size).toBe(0n);
          resolve();
        } catch (e) {
          reject(e);
        }
      });
    });
  });

  it("rejects when called with another RoaringBitmap64 instance (sync throw, mentions clone)", () => {
    const a = new RoaringBitmap64();
    expect(() => (RoaringBitmap64 as any).fromArrayAsync(a)).toThrow(/RoaringBitmap64/);
    expect(() => (RoaringBitmap64 as any).fromArrayAsync(a)).toThrow(/clone/);
  });

  it("rejects the promise on a non-BigInt element", async () => {
    await expect(RoaringBitmap64.fromArrayAsync([1n, 2 as any])).rejects.toThrow(TypeError);
  });

  it("rejects the promise on a negative bigint", async () => {
    await expect(RoaringBitmap64.fromArrayAsync([-1n])).rejects.toThrow(RangeError);
  });

  it("handles many concurrent invocations without contamination", async () => {
    const callsBefore = RoaringBitmap64.getInstancesCount();
    const tasks: Promise<RoaringBitmap64>[] = [];
    for (let i = 0; i < 32; i++) {
      const base = BigInt(i) * 100n;
      tasks.push(RoaringBitmap64.fromArrayAsync([base, base + 1n, base + 2n]));
    }
    const results = await Promise.all(tasks);
    expect(results).toHaveLength(32);
    for (let i = 0; i < 32; i++) {
      const base = BigInt(i) * 100n;
      expect(results[i]).toBeInstanceOf(RoaringBitmap64);
      expect(results[i].size).toBe(3n);
      expect(results[i].has(base)).toBe(true);
      expect(results[i].has(base + 2n)).toBe(true);
    }
    // Cleanup so the assertion below isn't fooled by retained instances.
    for (const r of results) r.dispose();
    if (typeof global.gc === "function") global.gc();
    // Instance count should not grow unboundedly across invocations.
    const callsAfter = RoaringBitmap64.getInstancesCount();
    expect(callsAfter - callsBefore).toBeLessThanOrEqual(32);
  });

  it("runs independently of RoaringBitmap32.fromArrayAsync (no cross-contamination)", async () => {
    const [r64, r32] = await Promise.all([
      RoaringBitmap64.fromArrayAsync([1n, 2n, 1n << 40n]),
      RoaringBitmap32.fromArrayAsync([10, 20, 30]),
    ]);
    expect(r64).toBeInstanceOf(RoaringBitmap64);
    expect(r32).toBeInstanceOf(RoaringBitmap32);
    expect(r64.size).toBe(3n);
    expect(r32.size).toBe(3);
    expect(r64.has(1n << 40n)).toBe(true);
    expect(r32.has(20)).toBe(true);
  });
});
