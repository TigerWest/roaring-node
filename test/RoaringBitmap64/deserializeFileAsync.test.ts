import { promises as fs } from "node:fs";
import * as os from "node:os";
import * as path from "node:path";
import { describe, expect, it } from "vitest";
import { RoaringBitmap64 } from "../..";

describe("RoaringBitmap64.deserializeFileAsync errors", () => {
  it("rejects a missing file", async () => {
    await expect(
      RoaringBitmap64.deserializeFileAsync("/no/such/path/that/does/not/exist.bin", "portable"),
    ).rejects.toThrow(/open|not.*exist|file/i);
  });

  it("rejects a corrupt file", async () => {
    const dir = await fs.mkdtemp(path.join(os.tmpdir(), "rb64-"));
    const file = path.join(dir, "corrupt.bin");
    await fs.writeFile(file, Buffer.from([0, 1, 2, 3, 4, 5]));
    await expect(RoaringBitmap64.deserializeFileAsync(file, "portable")).rejects.toThrow(/invalid/);
    await fs.rm(dir, { recursive: true });
  });

  it("rejects a frozen format", async () => {
    const dir = await fs.mkdtemp(path.join(os.tmpdir(), "rb64-"));
    const file = path.join(dir, "x.bin");
    await fs.writeFile(file, Buffer.alloc(0));
    await expect(RoaringBitmap64.deserializeFileAsync(file, "unsafe_frozen_croaring" as any)).rejects.toThrow(
      /portable/,
    );
    await fs.rm(dir, { recursive: true });
  });
});
