import { describe, expect, it } from "vitest";
import { RoaringBitmap64 } from "../../";

describe("RoaringBitmap64.clone", () => {
  it("produces an independent copy", () => {
    const a = new RoaringBitmap64();
    a.add(1n);
    a.add(2n ** 33n);
    const b = a.clone();
    expect(b.size).toBe(2n);
    expect(b.has(2n ** 33n)).toBe(true);
    b.add(99n);
    expect(a.has(99n)).toBe(false);
    expect(a.size).toBe(2n);
  });
});
