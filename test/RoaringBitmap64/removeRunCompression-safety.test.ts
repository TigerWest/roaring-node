import { describe, expect, it } from "vitest";
import { RoaringBitmap64 } from "../../";

describe("RoaringBitmap64.removeRunCompression safety", () => {
  it("preserves all values when run-compressed bitmap is decompressed", () => {
    const rb = new RoaringBitmap64();
    rb.addRange(1_000_000n, 1_010_000n);
    rb.runOptimize();
    const before = [...rb];
    const changed = rb.removeRunCompression();
    expect(changed).toBe(true);
    expect([...rb]).toEqual(before);
    expect(rb.size).toBe(10_000n);
  });

  it("returns false and is a no-op on bitmap without runs", () => {
    const rb = new RoaringBitmap64([1n, 2n, 3n]);
    const changed = rb.removeRunCompression();
    expect(changed).toBe(false);
    expect([...rb]).toEqual([1n, 2n, 3n]);
  });
});
