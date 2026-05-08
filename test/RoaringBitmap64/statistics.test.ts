import { describe, expect, it } from "vitest";
import { RoaringBitmap64 } from "../../";

describe("RoaringBitmap64.statistics", () => {
  it("returns expected shape for an empty bitmap", () => {
    const b = new RoaringBitmap64();
    const s = b.statistics();
    expect(s.containers).toBe(0);
    expect(s.size).toBe(0n);
    expect(s.minValue).toBeUndefined();
    expect(s.maxValue).toBeUndefined();
  });

  it("counts cardinality as bigint", () => {
    const b = new RoaringBitmap64();
    b.addRange(0n, 1000n);
    b.add(1n << 40n);
    const s = b.statistics();
    expect(s.size).toBe(1001n);
    expect(s.minValue).toBe(0n);
    expect(s.maxValue).toBe(1n << 40n);
  });

  it("sums of values-in-containers equals size", () => {
    const b = new RoaringBitmap64();
    b.addRange(0n, 50_000n);
    b.add((1n << 32n) + 7n);
    const s = b.statistics();
    const sumValues = s.valuesInArrayContainers + s.valuesInRunContainers + s.valuesInBitsetContainers;
    expect(BigInt(sumValues)).toBe(s.size);
  });

  it("container counts match arrayContainers + runContainers + bitsetContainers", () => {
    const b = new RoaringBitmap64();
    b.addRange(0n, 100_000n);
    b.runOptimize();
    const s = b.statistics();
    expect(s.arrayContainers + s.runContainers + s.bitsetContainers).toBe(s.containers);
  });

  it("throws when disposed", () => {
    const b = new RoaringBitmap64();
    b.dispose();
    expect(() => b.statistics()).toThrow();
  });
});
