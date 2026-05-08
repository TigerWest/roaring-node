import { describe, expect, it } from "vitest";
import { RoaringBitmap64 } from "../../";

describe("RoaringBitmap64.serializeAsync", () => {
  it("matches the synchronous serialize output (portable)", async () => {
    const rb = new RoaringBitmap64();
    rb.addRange(0n, 10000n);
    const sync = rb.serialize("portable");
    const asyncBuf = await rb.serializeAsync("portable");
    expect(Buffer.compare(sync, asyncBuf)).toBe(0);
  });

  it("matches the synchronous serialize output (default = portable)", async () => {
    const rb = new RoaringBitmap64([1n, 2n, 3n, 2n ** 50n]);
    const sync = rb.serialize();
    const asyncBuf = await rb.serializeAsync();
    expect(Buffer.compare(sync, asyncBuf)).toBe(0);
  });
});
