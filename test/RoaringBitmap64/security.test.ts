import { describe, expect, it } from "vitest";
import { RoaringBitmap64 } from "../../";

describe("RoaringBitmap64 security & rigor", () => {
  describe("constructor argument validation", () => {
    it("rejects another RoaringBitmap64 instance with TypeError (use clone instead)", () => {
      const a = new RoaringBitmap64();
      a.addMany([1n, 2n, 3n]);
      expect(() => new RoaringBitmap64(a as any)).toThrow(TypeError);
    });

    it("rejects non-iterable, non-BigUint64Array with TypeError", () => {
      expect(() => new RoaringBitmap64(42 as any)).toThrow(TypeError);
      expect(() => new RoaringBitmap64("hello" as any)).toThrow(TypeError);
    });

    it("accepts undefined (empty bitmap)", () => {
      const b = new RoaringBitmap64(undefined);
      expect(b.size).toBe(0n);
    });

    it("accepts null (empty bitmap)", () => {
      const b = new RoaringBitmap64(null as any);
      expect(b.size).toBe(0n);
    });

    it("clone() still works after constructor RB64 rejection", () => {
      const a = new RoaringBitmap64();
      a.addMany([1n, 2n, 3n]);
      const c = a.clone();
      expect(c).toBeInstanceOf(RoaringBitmap64);
      expect(c.toArray()).toEqual([1n, 2n, 3n]);
      a.add(99n);
      expect(c.has(99n)).toBe(false);
    });
  });

  describe("static set ops argument validation", () => {
    it("throws TypeError on null argument (no crash)", () => {
      const a = new RoaringBitmap64();
      expect(() => RoaringBitmap64.and(a, null as any)).toThrow(TypeError);
      expect(() => RoaringBitmap64.or(null as any, a)).toThrow(TypeError);
    });

    it("throws TypeError on undefined argument (no crash)", () => {
      const a = new RoaringBitmap64();
      expect(() => RoaringBitmap64.and(a, undefined as any)).toThrow(TypeError);
      expect(() => RoaringBitmap64.xor(undefined as any, a)).toThrow(TypeError);
    });

    it("throws TypeError on disposed argument", () => {
      const a = new RoaringBitmap64();
      const b = new RoaringBitmap64();
      a.add(1n);
      b.dispose();
      expect(() => RoaringBitmap64.and(a, b)).toThrow(TypeError);
      expect(() => RoaringBitmap64.or(b, a)).toThrow(TypeError);
      expect(() => RoaringBitmap64.jaccardIndex(a, b)).toThrow(TypeError);
    });

    it("throws TypeError on plain object (not RB64)", () => {
      const a = new RoaringBitmap64();
      expect(() => RoaringBitmap64.and(a, {} as any)).toThrow(TypeError);
      expect(() => RoaringBitmap64.andCardinality({} as any, a)).toThrow(TypeError);
    });
  });

  describe("instance.deserialize lifecycle (no leak)", () => {
    it("repeated deserialize on same instance does not leak", () => {
      const original = new RoaringBitmap64();
      original.addMany([1n, 2n, 3n, 1000n, 2n ** 40n]);
      const buf = original.serialize();

      const target = new RoaringBitmap64();
      target.add(999n);
      // Repeated calls should replace, not append, and should free the old bitmap
      for (let i = 0; i < 50; ++i) {
        target.deserialize(buf);
      }
      expect(target.size).toBe(5n);
      expect(target.equals(original)).toBe(true);
      expect(target.has(999n)).toBe(false);
    });
  });

  describe("post-dispose static op safety", () => {
    it("static.deserialize with truncated buffer throws (no crash)", () => {
      const a = new RoaringBitmap64();
      a.addMany([1n, 2n, 3n]);
      const buf = a.serialize();
      const truncated = buf.slice(0, Math.max(1, Math.floor(buf.length / 2)));
      expect(() => RoaringBitmap64.deserialize(truncated)).toThrow();
    });

    it("static.deserialize with random garbage throws (no crash)", () => {
      const garbage = Buffer.from([0xff, 0xee, 0xdd, 0xcc, 0xbb, 0xaa, 0x99]);
      expect(() => RoaringBitmap64.deserialize(garbage)).toThrow();
    });

    it("static.deserialize with empty buffer throws", () => {
      expect(() => RoaringBitmap64.deserialize(Buffer.alloc(0))).toThrow();
    });
  });

  describe("iterator invalidation safety", () => {
    it("iterator throws if parent is mutated during iteration (add)", () => {
      const b = new RoaringBitmap64();
      b.addMany([1n, 2n, 3n]);
      const it = b[Symbol.iterator]();
      expect(it.next().value).toBe(1n);
      b.add(99n); // mutate
      expect(() => it.next()).toThrow(/mutated/);
    });

    it("iterator throws if parent is mutated during iteration (clear)", () => {
      const b = new RoaringBitmap64();
      b.addMany([1n, 2n, 3n]);
      const it = b[Symbol.iterator]();
      expect(it.next().value).toBe(1n);
      b.clear();
      expect(() => it.next()).toThrow(/mutated/);
    });

    it("iterator throws if parent is mutated by inplace op", () => {
      const a = new RoaringBitmap64();
      a.addMany([1n, 2n, 3n]);
      const b = new RoaringBitmap64();
      b.addMany([2n, 3n, 4n]);
      const it = a[Symbol.iterator]();
      expect(it.next().value).toBe(1n);
      a.orInPlace(b);
      expect(() => it.next()).toThrow(/mutated/);
    });
  });

  describe("hostile iterable safety", () => {
    it("iterable returning non-object from next() throws TypeError (not crash)", () => {
      const hostile = {
        [Symbol.iterator]() {
          return {
            next() {
              return 42 as any; // not an object
            },
          };
        },
      };
      const b = new RoaringBitmap64();
      expect(() => b.addMany(hostile as any)).toThrow(TypeError);
    });

    it("iterable factory returning non-object throws TypeError", () => {
      const hostile = {
        [Symbol.iterator]() {
          return 42 as any;
        },
      };
      const b = new RoaringBitmap64();
      expect(() => b.addMany(hostile as any)).toThrow(TypeError);
    });

    it("iterable without next() method throws TypeError", () => {
      const hostile = {
        [Symbol.iterator]() {
          return {};
        },
      };
      const b = new RoaringBitmap64();
      expect(() => b.addMany(hostile as any)).toThrow(TypeError);
    });
  });
});
