import { existsSync, rmSync } from "node:fs";
import { tmpdir } from "node:os";
import { join } from "node:path";
import { afterEach, describe, expect, it } from "vitest";
import { RoaringBitmap64 } from "../../";

const cleanupPaths: string[] = [];
afterEach(() => {
  for (const p of cleanupPaths.splice(0)) {
    try {
      rmSync(p, { force: true });
    } catch {
      /* ignore */
    }
    try {
      rmSync(`${p}.tmp`, { force: true });
    } catch {
      /* ignore */
    }
  }
});

describe("RoaringBitmap64.serializeFileAsync atomicity", () => {
  it("leaves no <path>.tmp behind on success", async () => {
    const dest = join(tmpdir(), `rb64-atomic-ok-${process.pid}-${Date.now()}.bin`);
    cleanupPaths.push(dest);
    const bm = new RoaringBitmap64();
    bm.add(7n);
    await bm.serializeFileAsync(dest, "portable");
    expect(existsSync(dest)).toBe(true);
    expect(existsSync(`${dest}.tmp`)).toBe(false);
  });
});
