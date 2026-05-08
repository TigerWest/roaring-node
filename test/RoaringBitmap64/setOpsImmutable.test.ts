import { describe, expect, it } from "vitest";
import { RoaringBitmap64 } from "../../";

describe("RoaringBitmap64.union", () => {
  it("returns a new RoaringBitmap64 when other is a RoaringBitmap64", () => {
    const a = new RoaringBitmap64();
    a.add(1n);
    a.add(2n);
    const b = new RoaringBitmap64();
    b.add(2n);
    b.add(3n);

    const result = a.union(b);
    expect(result).toBeInstanceOf(RoaringBitmap64);
    expect(result.toArray()).toEqual([1n, 2n, 3n]);
    expect(a.toArray()).toEqual([1n, 2n]);
    expect(b.toArray()).toEqual([2n, 3n]);
  });

  it("returns a new instance distinct from both operands", () => {
    const a = new RoaringBitmap64();
    a.add(1n);
    const b = new RoaringBitmap64();
    b.add(2n);

    const result = a.union(b);
    expect(result).not.toBe(a);
    expect(result).not.toBe(b);

    result.add(99n);
    expect(a.has(99n)).toBe(false);
    expect(b.has(99n)).toBe(false);
  });

  it("works with an empty other RoaringBitmap64", () => {
    const a = new RoaringBitmap64();
    a.add(1n);
    a.add(2n);
    const empty = new RoaringBitmap64();

    expect(a.union(empty).toArray()).toEqual([1n, 2n]);
    expect(empty.union(a).toArray()).toEqual([1n, 2n]);
  });

  it("works across the 2^32 boundary", () => {
    const a = new RoaringBitmap64();
    a.add((1n << 32n) - 1n);
    const b = new RoaringBitmap64();
    b.add(1n << 32n);

    const result = a.union(b);
    expect(result.has((1n << 32n) - 1n)).toBe(true);
    expect(result.has(1n << 32n)).toBe(true);
    expect(result.size).toBe(2n);
  });

  it("returns a Set<bigint> when other is a Set<bigint>", () => {
    const a = new RoaringBitmap64();
    a.add(1n);
    a.add(2n);

    const result = a.union(new Set<bigint>([2n, 3n]));
    expect(result).toBeInstanceOf(Set);
    expect([...result].sort()).toEqual([1n, 2n, 3n]);
  });
});

describe("RoaringBitmap64.intersection", () => {
  it("returns a new RoaringBitmap64 when other is a RoaringBitmap64", () => {
    const a = new RoaringBitmap64();
    a.add(1n);
    a.add(2n);
    a.add(3n);
    const b = new RoaringBitmap64();
    b.add(2n);
    b.add(3n);
    b.add(4n);

    const result = a.intersection(b);
    expect(result).toBeInstanceOf(RoaringBitmap64);
    expect(result.toArray()).toEqual([2n, 3n]);
    expect(a.toArray()).toEqual([1n, 2n, 3n]);
    expect(b.toArray()).toEqual([2n, 3n, 4n]);
  });

  it("returns an empty RoaringBitmap64 when operands are disjoint", () => {
    const a = new RoaringBitmap64();
    a.add(1n);
    const b = new RoaringBitmap64();
    b.add(2n);
    expect(a.intersection(b).isEmpty).toBe(true);
  });

  it("returns an empty RoaringBitmap64 when other is empty", () => {
    const a = new RoaringBitmap64();
    a.add(1n);
    const empty = new RoaringBitmap64();
    expect(a.intersection(empty).isEmpty).toBe(true);
    expect(empty.intersection(a).isEmpty).toBe(true);
  });

  it("works across the 2^32 boundary", () => {
    const a = new RoaringBitmap64();
    a.add((1n << 32n) - 1n);
    a.add(1n << 32n);
    const b = new RoaringBitmap64();
    b.add(1n << 32n);
    b.add((1n << 32n) + 1n);

    expect(a.intersection(b).toArray()).toEqual([1n << 32n]);
  });

  it("returns a Set<bigint> when other is a Set<bigint>", () => {
    const a = new RoaringBitmap64();
    a.add(1n);
    a.add(2n);
    a.add(3n);
    const result = a.intersection(new Set<bigint>([2n, 3n, 4n]));
    expect(result).toBeInstanceOf(Set);
    expect([...result].sort()).toEqual([2n, 3n]);
  });
});

describe("RoaringBitmap64.difference", () => {
  it("returns a new RoaringBitmap64 when other is a RoaringBitmap64", () => {
    const a = new RoaringBitmap64();
    a.add(1n);
    a.add(2n);
    a.add(3n);
    const b = new RoaringBitmap64();
    b.add(2n);

    const result = a.difference(b);
    expect(result).toBeInstanceOf(RoaringBitmap64);
    expect(result.toArray()).toEqual([1n, 3n]);
    expect(a.toArray()).toEqual([1n, 2n, 3n]);
    expect(b.toArray()).toEqual([2n]);
  });

  it("returns this-as-clone when other is empty", () => {
    const a = new RoaringBitmap64();
    a.add(1n);
    a.add(2n);
    const empty = new RoaringBitmap64();

    const result = a.difference(empty);
    expect(result.toArray()).toEqual([1n, 2n]);
    result.remove(1n);
    expect(a.toArray()).toEqual([1n, 2n]);
  });

  it("returns an empty bitmap when other is a superset", () => {
    const a = new RoaringBitmap64();
    a.add(1n);
    const b = new RoaringBitmap64();
    b.add(1n);
    b.add(2n);
    expect(a.difference(b).isEmpty).toBe(true);
  });

  it("works across the 2^32 boundary", () => {
    const a = new RoaringBitmap64();
    a.add((1n << 32n) - 1n);
    a.add(1n << 32n);
    a.add((1n << 32n) + 1n);
    const b = new RoaringBitmap64();
    b.add(1n << 32n);

    expect(a.difference(b).toArray()).toEqual([(1n << 32n) - 1n, (1n << 32n) + 1n]);
  });

  it("returns a Set<bigint> when other is a Set<bigint>", () => {
    const a = new RoaringBitmap64();
    a.add(1n);
    a.add(2n);
    a.add(3n);
    const result = a.difference(new Set<bigint>([2n]));
    expect(result).toBeInstanceOf(Set);
    expect([...result].sort()).toEqual([1n, 3n]);
  });

  it("Set fallback works when this is larger than other", () => {
    const a = new RoaringBitmap64();
    for (let i = 0n; i < 100n; i++) {
      a.add(i);
    }
    const result = a.difference(new Set<bigint>([1n, 2n, 3n]));
    expect(result).toBeInstanceOf(Set);
    expect(result.size).toBe(97);
    expect(result.has(0n)).toBe(true);
    expect(result.has(1n)).toBe(false);
    expect(result.has(99n)).toBe(true);
  });

  it("Set fallback works when other is larger than this", () => {
    const a = new RoaringBitmap64();
    a.add(1n);
    a.add(2n);
    const big = new Set<bigint>();
    for (let i = 0n; i < 100n; i++) {
      big.add(i + 50n);
    }
    const result = a.difference(big);
    expect(result).toBeInstanceOf(Set);
    expect([...result].sort()).toEqual([1n, 2n]);
  });
});

describe("RoaringBitmap64.symmetricDifference", () => {
  it("returns a new RoaringBitmap64 when other is a RoaringBitmap64", () => {
    const a = new RoaringBitmap64();
    a.add(1n);
    a.add(2n);
    a.add(3n);
    const b = new RoaringBitmap64();
    b.add(2n);
    b.add(3n);
    b.add(4n);

    const result = a.symmetricDifference(b);
    expect(result).toBeInstanceOf(RoaringBitmap64);
    expect(result.toArray()).toEqual([1n, 4n]);
    expect(a.toArray()).toEqual([1n, 2n, 3n]);
    expect(b.toArray()).toEqual([2n, 3n, 4n]);
  });

  it("equals union when operands are disjoint", () => {
    const a = new RoaringBitmap64();
    a.add(1n);
    const b = new RoaringBitmap64();
    b.add(2n);
    expect(a.symmetricDifference(b).toArray()).toEqual([1n, 2n]);
  });

  it("returns empty bitmap when operands are equal", () => {
    const a = new RoaringBitmap64();
    a.add(1n);
    a.add(2n);
    const b = new RoaringBitmap64();
    b.add(1n);
    b.add(2n);
    expect(a.symmetricDifference(b).isEmpty).toBe(true);
  });

  it("works across the 2^32 boundary", () => {
    const a = new RoaringBitmap64();
    a.add((1n << 32n) - 1n);
    a.add(1n << 32n);
    const b = new RoaringBitmap64();
    b.add(1n << 32n);
    b.add((1n << 32n) + 1n);

    expect(a.symmetricDifference(b).toArray()).toEqual([(1n << 32n) - 1n, (1n << 32n) + 1n]);
  });

  it("returns a Set<bigint> when other is a Set<bigint>", () => {
    const a = new RoaringBitmap64();
    a.add(1n);
    a.add(2n);
    a.add(3n);
    const result = a.symmetricDifference(new Set<bigint>([2n, 3n, 4n]));
    expect(result).toBeInstanceOf(Set);
    expect([...result].sort()).toEqual([1n, 4n]);
  });
});
