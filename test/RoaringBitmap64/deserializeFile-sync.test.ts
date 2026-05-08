import * as fs from "node:fs";
import * as os from "node:os";
import * as path from "node:path";
import { describe, expect, it } from "vitest";
import { RoaringBitmap64 } from "../../";

describe("RoaringBitmap64.deserializeFile (synchronous)", () => {
  it("round-trips a portable serialization through a file", () => {
    const orig = new RoaringBitmap64([1n, 2n, 3n, 2n ** 33n, 2n ** 64n - 1n]);
    const buf = orig.serialize();
    const tmp = path.join(os.tmpdir(), `rb64-deserializeFile-sync-${process.pid}-${Date.now()}.bin`);
    try {
      fs.writeFileSync(tmp, buf);
      const decoded = RoaringBitmap64.deserializeFile(tmp);
      expect(decoded.toArray()).toEqual([1n, 2n, 3n, 2n ** 33n, 2n ** 64n - 1n]);
    } finally {
      try {
        fs.unlinkSync(tmp);
      } catch {
        /* ignore */
      }
    }
  });
});
