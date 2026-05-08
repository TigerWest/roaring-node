import { describe, expect, it } from "vitest";
import { RoaringBitmap64 } from "../../";

describe("RoaringBitmap64.toString / contentToString", () => {
  it("toString returns the literal class name", () => {
    const b = new RoaringBitmap64();
    b.add(1n);
    expect(b.toString()).toBe("RoaringBitmap64");
  });

  it("contentToString returns [v1,v2,...]", () => {
    const b = new RoaringBitmap64();
    b.addMany([1n, 2n, 10n]);
    expect(b.contentToString()).toBe("[1,2,10]");
  });

  it("contentToString on empty bitmap returns []", () => {
    expect(new RoaringBitmap64().contentToString()).toBe("[]");
  });

  it("contentToString respects maxLength with ellipsis truncation", () => {
    const b = new RoaringBitmap64();
    for (let i = 0n; i < 50n; ++i) {
      b.add(i);
    }
    const s = b.contentToString(20);
    expect(s.length).toBeLessThanOrEqual(20 + 3);
    expect(s.startsWith("[")).toBe(true);
    expect(s.endsWith("]") || s.endsWith("...]")).toBe(true);
  });
});

describe("RoaringBitmap64.join", () => {
  it("joins values with default ',' separator", () => {
    const b = new RoaringBitmap64();
    b.addMany([1n, 2n, 10n]);
    expect(b.join()).toBe("1,2,10");
  });

  it("joins values with a custom separator", () => {
    const b = new RoaringBitmap64();
    b.addMany([1n, 2n, 3n]);
    expect(b.join(" | ")).toBe("1 | 2 | 3");
  });

  it("returns empty string on empty bitmap", () => {
    expect(new RoaringBitmap64().join()).toBe("");
  });
});
