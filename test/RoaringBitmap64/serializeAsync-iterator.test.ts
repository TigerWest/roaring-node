import { describe, expect, it } from "vitest";
import { RoaringBitmap64, RoaringBitmap64Iterator } from "../..";

describe("async (frozen) serialize must invalidate live iterators", () => {
  it("frozen serializeAsync invalidates a live iterator (shrink_to_fit re-layout)", async () => {
    const b = new RoaringBitmap64();
    b.addRange(0n, 200000n);
    const it = new RoaringBitmap64Iterator(b);
    expect(it.next().done).toBe(false);

    // Frozen serialize triggers shrink_to_fit on the main thread (synchronous,
    // before the worker even starts). The fix bumps _version so the iterator's
    // captured version snapshot becomes stale.
    await b.serializeAsync("unsafe_frozen_croaring");
    expect(() => it.next()).toThrow(/mutated|invalid|version|disposed/i);
  });

  // Soft smoke: portable serializeAsync does not currently call shrink_to_fit,
  // so it must not crash when an iterator is alive. We don't assert whether
  // it.next() throws — that's an implementation detail subject to change.
  it("portable serializeAsync does not crash with a live iterator", async () => {
    const b = new RoaringBitmap64();
    b.addRange(0n, 1000n);
    const it = new RoaringBitmap64Iterator(b);
    it.next();
    await b.serializeAsync("portable");
    try {
      it.next();
    } catch {
      // Either outcome is acceptable; we only require the await above to settle.
    }
  });
});
