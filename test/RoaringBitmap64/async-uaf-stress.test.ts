import { describe, expect, it } from "vitest";
import { RoaringBitmap64 } from "../../";

// Defense-in-depth. Does NOT fail before the fix on release builds — freed
// memory often still contains valid-looking bytes. Purpose: (1) exercise the
// dispose-race on every CI run so regressions surface under sanitizers; (2)
// assert the public contract — dispose during in-flight async must not crash
// and the promise must settle.
describe("RoaringBitmap64 async + dispose race", () => {
  it("serializeAsync followed by sync dispose does not crash", async () => {
    const bm = new RoaringBitmap64();
    for (let i = 0n; i < 50000n; i++) bm.add(i);
    const p = bm.serializeAsync("portable");
    bm.dispose();
    await expect(
      p.then(
        () => "ok",
        () => "rejected",
      ),
    ).resolves.toMatch(/^(ok|rejected)$/);
  });

  it("toUint64ArrayAsync followed by sync dispose does not crash", async () => {
    const bm = new RoaringBitmap64();
    for (let i = 0n; i < 50000n; i++) bm.add(i);
    const p = bm.toUint64ArrayAsync();
    bm.dispose();
    await expect(
      p.then(
        () => "ok",
        () => "rejected",
      ),
    ).resolves.toMatch(/^(ok|rejected)$/);
  });

  it("serializeFileAsync followed by sync dispose does not crash", async () => {
    const { tmpdir } = await import("node:os");
    const { join } = await import("node:path");
    const { rmSync } = await import("node:fs");
    const dest = join(tmpdir(), `rb64-uaf-${process.pid}-${Date.now()}.bin`);
    const bm = new RoaringBitmap64();
    for (let i = 0n; i < 50000n; i++) bm.add(i);
    const p = bm.serializeFileAsync(dest, "portable");
    bm.dispose();
    await expect(
      p.then(
        () => "ok",
        () => "rejected",
      ),
    ).resolves.toMatch(/^(ok|rejected)$/);
    try {
      rmSync(dest, { force: true });
    } catch {
      /* ignore */
    }
    try {
      rmSync(`${dest}.tmp`, { force: true });
    } catch {
      /* ignore */
    }
  });

  it("burst of async + dispose pairs is stable", async () => {
    const N = 50;
    const promises: Promise<string>[] = [];
    for (let i = 0; i < N; i++) {
      const bm = new RoaringBitmap64();
      for (let j = 0n; j < 5000n; j++) bm.add(j);
      const p = bm.serializeAsync("portable").then(
        () => "ok",
        () => "rejected",
      );
      bm.dispose();
      promises.push(p);
    }
    const results = await Promise.all(promises);
    expect(results.length).toBe(N);
    for (const r of results) expect(r).toMatch(/^(ok|rejected)$/);
  });
});
