import { existsSync, mkdtempSync, readdirSync, readFileSync, rmSync, symlinkSync, writeFileSync } from "node:fs";
import { tmpdir } from "node:os";
import { join } from "node:path";
import { afterEach, describe, expect, it } from "vitest";
import { RoaringBitmap64 } from "../../";

const cleanupPaths: string[] = [];
afterEach(() => {
  for (const p of cleanupPaths.splice(0)) {
    try {
      rmSync(p, { recursive: true, force: true });
    } catch {
      /* ignore */
    }
  }
});

describe("RoaringBitmap64.serializeFileAsync atomicity", () => {
  it("leaves no <path>.tmp.* leftovers on success", async () => {
    const dir = mkdtempSync(join(tmpdir(), "rb64-atomic-ok-"));
    cleanupPaths.push(dir);
    const dest = join(dir, "out.bin");
    const bm = new RoaringBitmap64();
    bm.add(7n);
    await bm.serializeFileAsync(dest, "portable");
    expect(existsSync(dest)).toBe(true);
    // Unique tmp suffix is `<dest>.tmp.<pid>.<counter>` — none of these should
    // remain after a successful write.
    const leftover = readdirSync(dir).filter((f) => f.startsWith("out.bin.tmp."));
    expect(leftover).toEqual([]);
  });

  it("does NOT follow a pre-existing symlink at the predicted tmp path", () => {
    // Pre-create a symlink at a hypothetical tmp path pointing at a decoy
    // file outside the destination dir. With the new unique-tmp scheme the
    // attacker cannot guess the exact path, so this is a soft test: we just
    // confirm the decoy is never truncated even if a guess succeeded.
    const dir = mkdtempSync(join(tmpdir(), "rb64-atomic-symlink-"));
    cleanupPaths.push(dir);
    const dest = join(dir, "out.bin");
    const decoy = join(dir, "decoy.bin");
    writeFileSync(decoy, "do-not-touch");
    let symlinkSupported = true;
    try {
      // Best-guess: pid + counter=0 from this process.
      symlinkSync(decoy, `${dest}.tmp.${process.pid}.0`);
    } catch {
      symlinkSupported = false;
    }
    if (!symlinkSupported) {
      // FS doesn't allow symlinks (FAT, locked-down test env) — skip.
      return;
    }
    const bm = new RoaringBitmap64();
    bm.add(1n);
    bm.add(2n);
    return bm
      .serializeFileAsync(dest, "portable")
      .catch(() => undefined)
      .then(() => {
        expect(readFileSync(decoy, "utf8")).toBe("do-not-touch");
      });
  });

  it("two concurrent writers leave a deserializable file matching one of the two inputs", async () => {
    const dir = mkdtempSync(join(tmpdir(), "rb64-atomic-concurrent-"));
    cleanupPaths.push(dir);
    const dest = join(dir, "out.bin");

    const a = new RoaringBitmap64([1n, 2n, 3n]);
    const b = new RoaringBitmap64([10n, 20n, 30n]);

    const results = await Promise.allSettled([
      a.serializeFileAsync(dest, "portable"),
      b.serializeFileAsync(dest, "portable"),
    ]);
    // At least one writer must succeed (rename is atomic; both can succeed
    // with one overwriting the other on POSIX).
    expect(results.some((r) => r.status === "fulfilled")).toBe(true);

    // The on-disk file must deserialize to one of the two source bitmaps —
    // anything else means we observed a torn write where bytes from both
    // writers interleaved. We compare by *contents*, not byte-exact, because
    // two valid roaring portable encodings of the same set may legitimately
    // differ in container-shape choices.
    const deserialized = RoaringBitmap64.deserialize(readFileSync(dest));
    const arr = deserialized.toArray();
    const stringify = (xs: bigint[]) => xs.map((x) => x.toString()).join(",");
    const matchesA = stringify(arr) === stringify(a.toArray());
    const matchesB = stringify(arr) === stringify(b.toArray());
    expect(matchesA || matchesB).toBe(true);
  });
});
