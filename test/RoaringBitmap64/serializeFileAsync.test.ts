import { promises as fs } from "node:fs";
import * as os from "node:os";
import * as path from "node:path";
import { describe, expect, it } from "vitest";
import { RoaringBitmap64 } from "../..";

describe("RoaringBitmap64.serializeFileAsync + deserializeFileAsync", () => {
  it("round-trips an empty bitmap", async () => {
    const dir = await fs.mkdtemp(path.join(os.tmpdir(), "rb64-"));
    const file = path.join(dir, "empty.bin");
    const a = new RoaringBitmap64();
    await a.serializeFileAsync(file, "portable");
    const b = await RoaringBitmap64.deserializeFileAsync(file, "portable");
    expect(b.size).toBe(0n);
    await fs.rm(dir, { recursive: true });
  });

  it("round-trips a small bitmap", async () => {
    const dir = await fs.mkdtemp(path.join(os.tmpdir(), "rb64-"));
    const file = path.join(dir, "small.bin");
    const a = new RoaringBitmap64([1n, 2n, 3n, 1n << 40n]);
    await a.serializeFileAsync(file, "portable");
    const b = await RoaringBitmap64.deserializeFileAsync(file, "portable");
    expect(a.equals(b)).toBe(true);
    await fs.rm(dir, { recursive: true });
  });

  it("rejects non-string filePath", async () => {
    const a = new RoaringBitmap64();
    await expect(a.serializeFileAsync(42 as any, "portable")).rejects.toThrow(/filePath/);
  });

  it("rejects unknown format", async () => {
    const dir = await fs.mkdtemp(path.join(os.tmpdir(), "rb64-"));
    const file = path.join(dir, "bad.bin");
    const a = new RoaringBitmap64([1n]);
    await expect(a.serializeFileAsync(file, "bogus" as any)).rejects.toThrow(/format/);
    await fs.rm(dir, { recursive: true });
  });

  it("does not block the event loop", async () => {
    const dir = await fs.mkdtemp(path.join(os.tmpdir(), "rb64-"));
    const file = path.join(dir, "big.bin");
    // Build a sparse bitmap so the portable buffer is large (uses bitset
    // containers which always serialize to ~8KB / container) — guarantees
    // the off-thread fwrite takes long enough for at least one tick.
    const big = new RoaringBitmap64();
    const values = new BigUint64Array(200_000);
    for (let i = 0; i < values.length; i++) {
      // Spread values across many high containers so they all stay sparse.
      values[i] = BigInt(i) * 65537n;
    }
    big.addMany(values);
    let ticks = 0;
    const t = setInterval(() => {
      ticks++;
    }, 1);
    // Run two passes so cumulative off-thread time clears the 1ms tick.
    await big.serializeFileAsync(file, "portable");
    await big.serializeFileAsync(file, "portable");
    clearInterval(t);
    expect(ticks).toBeGreaterThan(0);
    await fs.rm(dir, { recursive: true });
  });
});
