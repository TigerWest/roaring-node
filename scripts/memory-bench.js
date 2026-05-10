#!/usr/bin/env node --expose-gc

/* eslint-disable no-console */
/* global gc */

"use strict";

const _gc = typeof gc !== "undefined" ? gc : () => {};

const { RoaringBitmap32, RoaringBitmap64, getRoaringUsedMemory } = require("../");

function settle() {
  _gc();
  _gc();
  _gc();
}

function measureHeapDelta(buildFn) {
  settle();
  const before = process.memoryUsage().heapUsed;
  const value = buildFn();
  settle();
  const after = process.memoryUsage().heapUsed;
  return { value, heapDelta: Math.max(0, after - before) };
}

function measureRoaringDelta(buildFn) {
  settle();
  const before = getRoaringUsedMemory();
  const value = buildFn();
  settle();
  const after = getRoaringUsedMemory();
  return { value, roaringDelta: Math.max(0, after - before) };
}

function fmt(n) {
  if (n < 1024) return `${n} B`;
  if (n < 1024 * 1024) return `${(n / 1024).toFixed(1)} KiB`;
  if (n < 1024 * 1024 * 1024) return `${(n / 1024 / 1024).toFixed(2)} MiB`;
  return `${(n / 1024 / 1024 / 1024).toFixed(2)} GiB`;
}

function bytesPerValue(bytes, n) {
  if (n === 0) return "—";
  return `${(bytes / n).toFixed(2)} B/val`;
}

function statBytes(stats) {
  return stats.bytesInArrayContainers + stats.bytesInRunContainers + stats.bytesInBitsetContainers;
}

function runScenario(name, n, gen32, gen64, opts = {}) {
  const skip32 = !!opts.skip32;
  console.log(`\n## ${name}  (n = ${n.toLocaleString()})`);
  console.log("-".repeat(76));

  // ---------- native Set<number> (32-bit values) ----------
  const set32 = skip32
    ? null
    : measureHeapDelta(() => {
        const s = new Set();
        for (let i = 0; i < n; ++i) s.add(gen32(i));
        return s;
      });

  // ---------- native Set<bigint> (64-bit values) ----------
  const setBig = measureHeapDelta(() => {
    const s = new Set();
    for (let i = 0; i < n; ++i) s.add(gen64(i));
    return s;
  });

  // ---------- RoaringBitmap32 ----------
  let rb32Built = null;
  let rb32Stats = null;
  let rb32Serialized = null;
  let rb32StatsOpt = null;
  let rb32SerializedOpt = null;
  if (!skip32) {
    rb32Built = measureRoaringDelta(() => {
      const x = new RoaringBitmap32();
      for (let i = 0; i < n; ++i) x.add(gen32(i));
      return x;
    });
    const rb32 = rb32Built.value;
    rb32Stats = rb32.statistics();
    rb32Serialized = Number(rb32.getSerializationSizeInBytes("portable"));

    // ---------- RoaringBitmap32 + runOptimize ----------
    rb32.runOptimize();
    rb32.shrinkToFit();
    rb32StatsOpt = rb32.statistics();
    rb32SerializedOpt = Number(rb32.getSerializationSizeInBytes("portable"));
  }

  // ---------- RoaringBitmap64 ----------
  const rb64Built = measureRoaringDelta(() => {
    const x = new RoaringBitmap64();
    for (let i = 0; i < n; ++i) x.add(gen64(i));
    return x;
  });
  const rb64 = rb64Built.value;
  const rb64Stats = rb64.statistics();
  const rb64Serialized = Number(rb64.getSerializationSizeInBytes("portable"));

  rb64.runOptimize();
  rb64.shrinkToFit();
  const rb64StatsOpt = rb64.statistics();
  const rb64SerializedOpt = Number(rb64.getSerializationSizeInBytes("portable"));

  const layoutOf = (s) => `${s.arrayContainers}/${s.runContainers}/${s.bitsetContainers}`;

  const rows = [];
  if (set32) rows.push(["Set<number>", set32.heapDelta, "—", "—"]);
  rows.push(["Set<bigint>", setBig.heapDelta, "—", "—"]);
  if (rb32Stats) {
    rows.push(["RoaringBitmap32", statBytes(rb32Stats), rb32Serialized, layoutOf(rb32Stats)]);
    rows.push(["RoaringBitmap32 (run-opt)", statBytes(rb32StatsOpt), rb32SerializedOpt, layoutOf(rb32StatsOpt)]);
  }
  rows.push(["RoaringBitmap64", statBytes(rb64Stats), rb64Serialized, layoutOf(rb64Stats)]);
  rows.push(["RoaringBitmap64 (run-opt)", statBytes(rb64StatsOpt), rb64SerializedOpt, layoutOf(rb64StatsOpt)]);

  const labelW = Math.max(...rows.map((r) => r[0].length));
  console.log(
    `${"impl".padEnd(labelW)}   ${"container/heap".padStart(14)}   ${"per value".padStart(12)}   ${"serialized".padStart(12)}   array/run/bitset`,
  );

  for (const [label, bytes, ser, layout] of rows) {
    console.log(
      `${label.padEnd(labelW)}   ${fmt(bytes).padStart(14)}   ${bytesPerValue(bytes, n).padStart(12)}   ${(typeof ser === "number" ? fmt(ser) : ser).padStart(12)}   ${layout}`,
    );
  }
}

(function main() {
  console.log("# RoaringBitmap memory benchmark");
  console.log(`Node ${process.version}, ${process.platform} ${process.arch}`);
  console.log(`getRoaringUsedMemory at start: ${fmt(getRoaringUsedMemory())}`);

  // --- 1. Dense low-range: classic 32-bit territory ---
  runScenario(
    "1. Dense low-range (every 3rd integer in [0, 3n))",
    1_000_000,
    (i) => 3 * i,
    (i) => BigInt(3 * i),
  );

  // --- 2. Random sparse 32-bit values ---
  runScenario(
    "2. Sparse random 32-bit values",
    100_000,
    (i) => (i * 2654435761) >>> 0,
    (i) => BigInt((i * 2654435761) >>> 0),
  );

  // --- 3. Long contiguous runs (run-container friendly) ---
  runScenario(
    "3. Contiguous run [0, n)",
    1_000_000,
    (i) => i,
    (i) => BigInt(i),
  );

  // --- 4. Sparse 64-bit values across high buckets (worst case for rb64) ---
  // each value lands in a different 32-bit bucket -> one container per value
  runScenario("4. Very sparse 64-bit (one value per high bucket)", 10_000, null, (i) => (BigInt(i) << 32n) | 1n, {
    skip32: true,
  });

  // --- 5. Sparse 64-bit with clustered high bits ---
  runScenario(
    "5. Clustered 64-bit (1024 high buckets x 1024 vals each)",
    1024 * 1024,
    null,
    (i) => {
      const hi = BigInt(i >>> 10) & 0x3ffn;
      const lo = BigInt(i & 0x3ff);
      return (hi << 32n) | lo;
    },
    { skip32: true },
  );

  console.log(`\ngetRoaringUsedMemory at end: ${fmt(getRoaringUsedMemory())}`);
  console.log("\nNotes:");
  console.log("  - container/heap = bytes reported by statistics() for roaring rows;");
  console.log("    process heap delta (rough) for native Set rows.");
  console.log("  - serialized = bytes from getSerializationSizeInBytes('portable').");
  console.log("  - run-opt rows are after runOptimize() + shrinkToFit().");
})();
