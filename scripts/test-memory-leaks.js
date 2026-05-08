#!/usr/bin/env node --expose-gc

/* eslint-disable node/no-unsupported-features/node-builtins */
/* eslint-disable no-console */
/* global gc */

const { runMain } = require("./lib/utils");

const _gc = typeof gc !== "undefined" ? gc : () => {};

async function testMemoryLeaks() {
  _gc();
  _gc();

  const { RoaringBitmap32, RoaringBitmap64, getRoaringUsedMemory } = require("../");

  _gc();
  _gc();

  console.log();
  console.log("RoaringUsedMemory", getRoaringUsedMemory());
  console.log("RoaringBitmap32.getInstancesCount", RoaringBitmap32.getInstancesCount());
  console.log("RoaringBitmap64.getInstancesCount", RoaringBitmap64.getInstancesCount());

  _gc();
  _gc();

  let a = new RoaringBitmap32([1, 2, 3]);
  let b = new RoaringBitmap32([1, 2]);

  let a64 = new RoaringBitmap64([1n, 2n, 3n, 1n << 33n, 1n << 40n]);
  let b64 = new RoaringBitmap64([1n, 2n, 1n << 33n]);

  console.time("buildup");

  const operation = (i) => {
    switch (i % 4) {
      case 1:
        return RoaringBitmap32.and(a, b);
      case 2:
        return RoaringBitmap32.xor(a, b);
      case 3:
        return RoaringBitmap32.andNot(a, b);
      default:
        return RoaringBitmap32.or(a, b);
    }
  };

  const operation64 = (i) => {
    switch (i % 4) {
      case 1:
        return RoaringBitmap64.and(a64, b64);
      case 2:
        return RoaringBitmap64.xor(a64, b64);
      case 3:
        return RoaringBitmap64.andNot(a64, b64);
      default:
        return RoaringBitmap64.or(a64, b64);
    }
  };

  let promises = [];
  for (let i = 0; i < 10000; i++) {
    const bmp = operation(i);
    if (i < 5) {
      promises.push(bmp.toUint32ArrayAsync());
    }
    promises.push(bmp.serializeAsync(i & 4 ? "croaring" : i & 8 ? "portable" : "unsafe_frozen_croaring"));

    const bmp64 = operation64(i);
    bmp64.serialize(i & 4 ? "portable" : "unsafe_frozen_croaring");
    if (i < 5) {
      promises.push(RoaringBitmap64.fromArrayAsync(bmp64.toUint64Array()));
    }
  }

  console.log();
  console.log("RoaringUsedMemory", getRoaringUsedMemory());
  console.log("RoaringBitmap32.getInstancesCount", RoaringBitmap32.getInstancesCount());
  console.log("RoaringBitmap64.getInstancesCount", RoaringBitmap64.getInstancesCount());
  console.log();

  await Promise.all(promises);
  promises = null;

  console.timeEnd("buildup");
  console.log();

  _gc();
  _gc();

  console.table(process.memoryUsage());
  process.stdout.write("\nallocating");
  console.time("allocation");

  _gc();
  _gc();

  promises = [];
  for (let i = 0; i < 10000000; i++) {
    const bmp = operation(i);
    const bmp64 = operation64(i);
    if (i % 100000 === 0) {
      process.stdout.write(".");
      _gc();
      _gc();
      promises.push(bmp.serializeAsync("croaring").then(() => undefined));
      bmp64.serialize("portable");
    }
  }
  await Promise.all(promises);
  promises = null;
  process.stdout.write("\n");
  console.timeEnd("allocation");

  console.log();
  console.log("RoaringUsedMemory", getRoaringUsedMemory());
  console.log("RoaringBitmap32.getInstancesCount", RoaringBitmap32.getInstancesCount());
  console.log("RoaringBitmap64.getInstancesCount", RoaringBitmap64.getInstancesCount());
  console.log();

  a = null;
  b = null;
  a64 = null;
  b64 = null;

  const promise = new Promise((resolve, reject) => {
    for (let i = 0; i < 10; ++i) {
      _gc();
    }
    setTimeout(() => {
      for (let i = 0; i < 10; ++i) {
        _gc();
      }
      setTimeout(() => {
        for (let i = 0; i < 10; ++i) {
          _gc();
        }
        console.table(process.memoryUsage());
        console.log();
        console.log("RoaringUsedMemory", getRoaringUsedMemory());
        console.log("RoaringBitmap32.getInstancesCount", RoaringBitmap32.getInstancesCount());
        console.log("RoaringBitmap64.getInstancesCount", RoaringBitmap64.getInstancesCount());
        console.log();

        setTimeout(() => {
          if (getRoaringUsedMemory() !== 0) {
            reject(new Error(`Memory leak detected. ${getRoaringUsedMemory()} bytes are still allocated.`));
          }

          if (RoaringBitmap32.getInstancesCount() !== 0) {
            reject(
              new Error(
                `Memory leak detected. ${RoaringBitmap32.getInstancesCount()} RoaringBitmap32 instances are still allocated.`,
              ),
            );
          }

          if (RoaringBitmap64.getInstancesCount() !== 0) {
            reject(
              new Error(
                `Memory leak detected. ${RoaringBitmap64.getInstancesCount()} RoaringBitmap64 instances are still allocated.`,
              ),
            );
          }
          resolve();
        });
      }, 150);
    }, 150);
  });

  await promise;
}

runMain(testMemoryLeaks, "test-memory-leaks");
