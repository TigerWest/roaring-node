import { describe, expect, it } from "vitest";
import { RoaringBitmap64 } from "../../";

describe("RoaringBitmap64 ES Set protocol", () => {
  describe("isSubsetOf", () => {
    it("works against another RoaringBitmap64", () => {
      const a = new RoaringBitmap64();
      a.add(1n);
      a.add(2n);
      const b = new RoaringBitmap64();
      b.add(1n);
      b.add(2n);
      b.add(3n);
      expect(a.isSubsetOf(b)).toBe(true);
      expect(b.isSubsetOf(a)).toBe(false);
    });

    it("works against a Set<bigint>", () => {
      const a = new RoaringBitmap64();
      a.add(1n);
      a.add(2n);
      expect(a.isSubsetOf(new Set([1n, 2n, 3n]))).toBe(true);
      expect(a.isSubsetOf(new Set([1n]))).toBe(false);
    });
  });

  describe("isSupersetOf", () => {
    it("works against another RoaringBitmap64", () => {
      const a = new RoaringBitmap64();
      a.add(1n);
      a.add(2n);
      a.add(3n);
      const b = new RoaringBitmap64();
      b.add(1n);
      b.add(2n);
      expect(a.isSupersetOf(b)).toBe(true);
      expect(b.isSupersetOf(a)).toBe(false);
    });

    it("works against a Set<bigint>", () => {
      const a = new RoaringBitmap64();
      a.add(1n);
      a.add(2n);
      a.add(3n);
      expect(a.isSupersetOf(new Set([1n, 2n]))).toBe(true);
      expect(a.isSupersetOf(new Set([1n, 4n]))).toBe(false);
    });
  });

  describe("isDisjointFrom", () => {
    it("works against another RoaringBitmap64", () => {
      const a = new RoaringBitmap64();
      a.add(1n);
      const b = new RoaringBitmap64();
      b.add(2n);
      expect(a.isDisjointFrom(b)).toBe(true);
      b.add(1n);
      expect(a.isDisjointFrom(b)).toBe(false);
    });

    it("works against a Set<bigint>", () => {
      const a = new RoaringBitmap64();
      a.add(1n);
      a.add(2n);
      expect(a.isDisjointFrom(new Set([3n, 4n]))).toBe(true);
      expect(a.isDisjointFrom(new Set([2n, 4n]))).toBe(false);
    });

    it("two empty sides are disjoint", () => {
      const a = new RoaringBitmap64();
      const b = new RoaringBitmap64();
      expect(a.isDisjointFrom(b)).toBe(true);
      expect(a.isDisjointFrom(new Set<bigint>())).toBe(true);
    });

    it("walks the smaller side when the other is a Set", () => {
      const big = new RoaringBitmap64();
      for (let i = 0n; i < 1000n; i++) big.add(i);
      const small = new Set<bigint>([5000n, 6000n]);
      expect(big.isDisjointFrom(small)).toBe(true);
      const overlapping = new Set<bigint>([5n, 6000n]);
      expect(big.isDisjointFrom(overlapping)).toBe(false);
    });
  });
});
