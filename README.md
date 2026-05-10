# roaring

Official port of [Roaring Bitmaps](http://roaringbitmap.org) for NodeJS as a native addon.

It is interoperable with other implementations via the [Roaring format](https://github.com/RoaringBitmap/RoaringFormatSpec/).
It takes advantage of AVX2 or SSE4.2 instructions on 64 bit platforms that supports it.

Roaring bitmaps are compressed bitmaps. They can be hundreds of times faster.

For a precompiled binary of this package compatible with AWS Lambda NodeJS v8.10.0, use [roaring-aws](https://www.npmjs.com/package/roaring-aws).

## Supported node versions

Node 20, 22, 24, 25 are currently supported.

Node 8 and 10 support was dropped in release 2.0

Node 12 and 14 support was dropped in release 2.3

Node 16 and 21 support was dropped in release 2.6

Node 18 support was dropped in release 2.7

## Worker thread support

Directly transferring an instance without copy between worker threads is not currently supported, but you can create a frozen view on a SharedArrayBuffer using bufferAlignedAllocShared and pass it to the worker thread.

## Installation

```sh
npm install --save roaring
```

### Linux libc variants

Prebuilt binaries are now published separately for `glibc` and `musl` targets. When running on Alpine or any other musl-based distribution the installer will request the `-musl` artifact; on other Linux distributions it will request the `-glibc` build. If a matching build is not available `npm install` falls back to compiling from source, so ensure the usual build toolchain is present.

## References

- This package - <https://www.npmjs.com/package/roaring>
- Source code and build tools for this package - <https://github.com/SalvatorePreviti/roaring-node>
- Roaring Bitmaps - <http://roaringbitmap.org/>
- Portable Roaring bitmaps in C - <https://github.com/RoaringBitmap/CRoaring>
- Portable Roaring bitmaps in C (unity build) - https://github.com/lemire/CRoaringUnityBuild

# Licenses

- This package is provided as open source software using Apache License.
- CRoaring is provided as open source software using Apache License.

# API

See the [roaring module documentation](https://salvatorepreviti.github.io/roaring-node/modules.html)

See the [RoaringBitmap32 class documentation](https://salvatorepreviti.github.io/roaring-node/classes/RoaringBitmap32.html)

# Code sample:

```javascript
// npm install --save roaring
// create this file as demo.js
// type node demo.js

const RoaringBitmap32 = require("roaring/RoaringBitmap32");

const bitmap1 = new RoaringBitmap32([1, 2, 3, 4, 5]);
bitmap1.addMany([100, 1000]);
console.log("bitmap1.toArray():", bitmap1.toArray());

const bitmap2 = new RoaringBitmap32([3, 4, 1000]);
console.log("bitmap2.toArray():", bitmap2.toArray());

const bitmap3 = new RoaringBitmap32();
console.log("bitmap1.size:", bitmap1.size);
console.log("bitmap3.has(3):", bitmap3.has(3));
bitmap3.add(3);
console.log("bitmap3.has(3):", bitmap3.has(3));

bitmap3.add(111);
bitmap3.add(544);
bitmap3.orInPlace(bitmap1);
bitmap1.runOptimize();
bitmap1.shrinkToFit();
console.log("contentToString:", bitmap3.contentToString());

console.log("bitmap3.toArray():", bitmap3.toArray());
console.log("bitmap3.maximum():", bitmap3.maximum());
console.log("bitmap3.rank(100):", bitmap3.rank(100));

const iterated = [];
for (const value of bitmap3) {
  iterated.push(value);
}
console.log("iterated:", iterated);

const serialized = bitmap3.serialize(false);
console.log("serialized:", serialized.toString("base64"));
console.log(
  "deserialized:",
  RoaringBitmap32.deserialize(serialized, false).toArray(),
);
```

# Other

Wanna play an open source game made by the author of this library? Try [Dante](https://github.com/SalvatorePreviti/js13k-2022)

# Development, local building

Clone the repository and install all the dependencies

```
git clone https://github.com/SalvatorePreviti/roaring-node.git

cd roaring-node

npm install
```

# To rebuild the roaring unity build updating to the latest version

```
./scripts/update-roaring.sh

npm run build

```

### To run the unit test

```
npm test
```

### To regenerate the API documentation

```
npm run doc
```

### To run the performance benchmarks

```sh
npm run benchmarks
```

It will produce a result similar to this one:

```
Platform : Darwin 17.6.0 x64
CPU      : Intel(R) Core(TM) i7-7700HQ CPU @ 2.80GHz AVX2
Cores    : 4 physical - 8 logical
Memory   : 16.00 GB
NodeJS   : v10.5.0 - V8 v6.7.288.46-node.8

* running 8 files...

• suite intersection (in place)
  65536 elements
  ✔ Set                  186.82 ops/sec  ±2.33%  66 runs  -99.98%
  ✔ FastBitSet       100,341.63 ops/sec  ±2.10%  85 runs  -87.10%
  ✔ RoaringBitmap32  777,765.97 ops/sec  ±2.14%  87 runs  fastest
  ➔ Fastest is RoaringBitmap32

• suite intersection (new)
  1048576 elements
  ✔ Set                  3.49 ops/sec   ±3.82%  13 runs  -99.89%
  ✔ FastBitSet       1,463.87 ops/sec   ±1.13%  84 runs  -55.54%
  ✔ RoaringBitmap32  3,292.51 ops/sec  ±20.89%  44 runs  fastest
  ➔ Fastest is RoaringBitmap32

• suite add
  65535 elements
  ✔ Set.add                                438.37 ops/sec  ±4.48%  68 runs  -85.21%
  ✔ RoaringBitmap32.tryAdd                 489.70 ops/sec  ±3.19%  76 runs  -83.48%
  ✔ RoaringBitmap32.add                    528.09 ops/sec  ±3.38%  76 runs  -82.18%
  ✔ RoaringBitmap32.addMany Array        1,652.62 ops/sec  ±4.20%  64 runs  -44.25%
  ✔ RoaringBitmap32.addMany Uint32Array  2,964.19 ops/sec  ±1.47%  86 runs  fastest
  ➔ Fastest is RoaringBitmap32.addMany Uint32Array

• suite iterator
  65536 elements
  ✔ Set              1,648.55 ops/sec  ±1.21%  86 runs  fastest
  ✔ RoaringBitmap32  1,239.06 ops/sec  ±1.62%  86 runs  -24.84%
  ➔ Fastest is Set

• suite intersection size
  262144 elements
  ✔ Set                   29.10 ops/sec  ±5.65%  51 runs  -99.99%
  ✔ FastBitSet        15,938.45 ops/sec  ±2.01%  86 runs  -94.09%
  ✔ RoaringBitmap32  269,502.51 ops/sec  ±2.08%  84 runs  fastest
  ➔ Fastest is RoaringBitmap32

• suite union (in place)
  65536 elements
  ✔ Set                  298.53 ops/sec  ±3.07%  65 runs  -99.97%
  ✔ FastBitSet       144,914.65 ops/sec  ±2.10%  75 runs  -83.38%
  ✔ RoaringBitmap32  871,794.31 ops/sec  ±3.85%  82 runs  fastest
  ➔ Fastest is RoaringBitmap32

• suite union size
  262144 elements
  ✔ Set                   17.69 ops/sec  ±3.40%  33 runs  -99.99%
  ✔ FastBitSet         8,481.07 ops/sec  ±1.54%  84 runs  -97.02%
  ✔ RoaringBitmap32  284,749.52 ops/sec  ±1.35%  86 runs  fastest
  ➔ Fastest is RoaringBitmap32

• suite union (new)
  1048576 elements
  ✔ Set                  1.83 ops/sec   ±6.60%   9 runs  -99.90%
  ✔ FastBitSet         744.68 ops/sec   ±1.11%  85 runs  -60.45%
  ✔ RoaringBitmap32  1,882.93 ops/sec  ±16.32%  44 runs  fastest
  ➔ Fastest is RoaringBitmap32


* completed: 53610.279ms
```

Works also on M1

````
Platform : Darwin 21.1.0 arm64
CPU      : Apple M1 Pro
Cores    : 10 physical - 10 logical
Memory   : 16.00 GB
NodeJS   : v16.13.1 - V8 v9.4.146.24-node.14

* running 8 files...

• suite union (in place)
  65536 elements
  ✔ Set                    456.60 ops/sec  ±1.88%  77 runs  -99.97%
  ✔ FastBitSet         232,794.03 ops/sec  ±0.94%  91 runs  -86.52%
  ✔ RoaringBitmap32  1,727,310.24 ops/sec  ±1.12%  95 runs  fastest
  ➔ Fastest is RoaringBitmap32

• suite intersection size
  262144 elements
  ✔ Set                   71.20 ops/sec  ±2.30%  62 runs  -99.98%
  ✔ FastBitSet        21,525.29 ops/sec  ±0.71%  97 runs  -93.72%
  ✔ RoaringBitmap32  342,892.37 ops/sec  ±0.93%  95 runs  fastest
  ➔ Fastest is RoaringBitmap32

• suite intersection (in place)
  65536 elements
  ✔ Set                    280.62 ops/sec  ±1.66%  76 runs  -99.97%
  ✔ FastBitSet         136,148.03 ops/sec  ±0.56%  96 runs  -87.11%
  ✔ RoaringBitmap32  1,055,978.14 ops/sec  ±1.16%  92 runs  fastest
  ➔ Fastest is RoaringBitmap32

• suite union size
  262144 elements
  ✔ Set                   37.95 ops/sec  ±2.33%  51 runs  -99.99%
  ✔ FastBitSet        12,111.37 ops/sec  ±0.67%  96 runs  -96.35%
  ✔ RoaringBitmap32  331,510.04 ops/sec  ±1.02%  95 runs  fastest
  ➔ Fastest is RoaringBitmap32

• suite intersection (new)
  1048576 elements
  ✔ Set                  6.54 ops/sec  ±5.41%  21 runs  -99.93%
  ✔ FastBitSet       2,087.94 ops/sec  ±5.23%  42 runs  -78.88%
  ✔ RoaringBitmap32  9,888.24 ops/sec  ±1.77%  51 runs  fastest
  ➔ Fastest is RoaringBitmap32

• suite union (new)
  1048576 elements
  ✔ Set                  3.80 ops/sec  ±7.91%  14 runs  -99.93%
  ✔ FastBitSet       1,693.77 ops/sec  ±3.90%  64 runs  -70.63%
  ✔ RoaringBitmap32  5,767.35 ops/sec  ±1.74%  51 runs  fastest
  ➔ Fastest is RoaringBitmap32

• suite iterator
  65536 elements
  ✔ Set.iterator              10,033.75 ops/sec  ±1.40%  92 runs  fastest
  ✔ Set.forEach                1,595.08 ops/sec  ±2.02%  89 runs  -84.10%
  ✔ RoaringBitmap32.iterator   2,879.57 ops/sec  ±1.06%  93 runs  -71.30%
  ✔ RoaringBitmap32.forEach    1,764.98 ops/sec  ±0.68%  97 runs  -82.41%
  ➔ Fastest is Set.iterator

• suite add
  65535 elements
  ✔ Set.add                                508.76 ops/sec  ±1.27%   86 runs  -91.37%
  ✔ RoaringBitmap32.tryAdd                 707.70 ops/sec  ±0.81%   96 runs  -87.99%
  ✔ RoaringBitmap32.add                    699.27 ops/sec  ±1.08%   90 runs  -88.13%
  ✔ RoaringBitmap32.addMany Array        4,457.70 ops/sec  ±0.54%   90 runs  -24.35%
  ✔ RoaringBitmap32.addMany Uint32Array  5,892.57 ops/sec  ±0.07%  101 runs  fastest
  ➔ Fastest is RoaringBitmap32.addMany Uint32Array


* completed: 27.170s```

````

Works on M1 Max with `RoaringBitmap64` (BigInt 64-bit) included

```
Platform : Darwin 25.2.0 arm64
CPU      : Apple M1 Max
Cores    : 10 physical - 10 logical
Memory   : 32.00 GB
NodeJS   : v22.12.0 - V8 v12.4.254.21-node.21

• suite intersection (new)
  65536 elements
  ✔ Set                  7.78 ops/sec   ±5.77%    10 runs  -99.86%
  ✔ Set<bigint>          3.06 ops/sec   ±5.43%    10 runs  -99.95%
  ✔ FastBitSet       4,706.32 ops/sec   ±2.77%  2354 runs  -17.49%
  ✔ RoaringBitmap32  5,703.56 ops/sec  ±35.86%  2852 runs  fastest
  ✔ RoaringBitmap64  5,437.72 ops/sec  ±34.10%  2721 runs  -4.66%
  ➔ Fastest is RoaringBitmap32

• suite union (new)
  65536 elements
  ✔ Set                 11.55 ops/sec   ±8.83%    10 runs  -99.60%
  ✔ Set<bigint>          5.41 ops/sec   ±6.05%    10 runs  -99.81%
  ✔ FastBitSet       2,854.64 ops/sec   ±2.41%  1428 runs  -0.66%
  ✔ RoaringBitmap32  2,873.69 ops/sec  ±35.77%  1437 runs  fastest
  ✔ RoaringBitmap64  2,698.09 ops/sec  ±34.81%  1359 runs  -6.11%
  ➔ Fastest is RoaringBitmap32

• suite add
  65535 elements
  ✔ Set.add                                 400.88 ops/sec  ±1.26%   201 runs  -92.28%
  ✔ Set<bigint>.add                         267.40 ops/sec  ±5.14%   134 runs  -94.85%
  ✔ RoaringBitmap32.tryAdd                  611.78 ops/sec  ±0.26%   306 runs  -88.22%
  ✔ RoaringBitmap64.tryAdd                  502.61 ops/sec  ±0.55%   252 runs  -90.32%
  ✔ RoaringBitmap32.add                     616.40 ops/sec  ±0.18%   309 runs  -88.13%
  ✔ RoaringBitmap64.add                     506.35 ops/sec  ±0.46%   254 runs  -90.25%
  ✔ RoaringBitmap32.addMany Array         4,530.14 ops/sec  ±0.46%  2266 runs  -12.73%
  ✔ RoaringBitmap64.addMany bigint[]        578.14 ops/sec  ±0.31%   290 runs  -88.86%
  ✔ RoaringBitmap32.addMany Uint32Array   5,191.21 ops/sec  ±1.67%  2596 runs  fastest
  ✔ RoaringBitmap64.addMany BigUint64Array 3,575.72 ops/sec  ±0.12%  1788 runs  -31.12%
  ➔ Fastest is RoaringBitmap32.addMany Uint32Array

• suite iterator
  65536 elements
  ✔ Set.iterator                         11,987.37 ops/sec  ±0.10%  5994 runs  fastest
  ✔ Set<bigint>.iterator                  3,426.62 ops/sec  ±1.33%  1714 runs  -71.41%
  ✔ Set.forEach                           2,715.65 ops/sec  ±0.18%  1358 runs  -77.35%
  ✔ RoaringBitmap32.iterator              2,664.41 ops/sec  ±0.19%  1333 runs  -77.77%
  ✔ RoaringBitmap64.iterator                 61.87 ops/sec  ±0.47%    31 runs  -99.48%
  ✔ RoaringBitmap32.forEach               2,347.53 ops/sec  ±0.17%  1174 runs  -80.42%
  ✔ RoaringBitmap64.toUint64Array + loop  4,638.03 ops/sec  ±0.94%  2320 runs  -61.31%
  ✔ RoaringBitmap64.toArray + loop          275.49 ops/sec  ±3.06%   138 runs  -97.70%
  ➔ Fastest is Set.iterator

• suite deserialize small (1k)
  ✔ RoaringBitmap32  1,226,929.90 ops/sec  ±10.17%  613465 runs  fastest
  ✔ RoaringBitmap64  1,045,753.72 ops/sec  ±12.65%  522877 runs  -14.77%
  ➔ Fastest is RoaringBitmap32

• suite deserialize medium (100k)
  ✔ RoaringBitmap32  96,486.18 ops/sec  ±7.25%  48252 runs  fastest
  ✔ RoaringBitmap64  86,293.45 ops/sec  ±7.87%  43147 runs  -10.56%
  ➔ Fastest is RoaringBitmap32

• suite deserialize large (1M)
  ✔ RoaringBitmap32  10,179.76 ops/sec  ±9.00%  5090 runs  fastest
  ✔ RoaringBitmap64   9,753.14 ops/sec  ±9.19%  4877 runs  -4.19%
  ➔ Fastest is RoaringBitmap32

• suite union size
  262144 elements
  ✔ Set                  62.54 ops/sec  ±2.33%      32 runs  -99.98%
  ✔ Set<bigint>          31.12 ops/sec  ±2.86%      16 runs  -99.99%
  ✔ FastBitSet       13,817.12 ops/sec  ±0.15%    6909 runs  -96.21%
  ✔ RoaringBitmap32 364,981.41 ops/sec  ±0.11%  182491 runs  fastest
  ✔ RoaringBitmap64 302,505.17 ops/sec  ±0.13%  151253 runs  -17.12%
  ➔ Fastest is RoaringBitmap32

• suite intersection size
  262144 elements
  ✔ Set                  90.87 ops/sec  ±2.06%      46 runs  -99.97%
  ✔ Set<bigint>          27.30 ops/sec  ±5.63%      14 runs  -99.99%
  ✔ FastBitSet       23,499.30 ops/sec  ±0.13%   11750 runs  -92.97%
  ✔ RoaringBitmap32 333,281.37 ops/sec  ±3.83%  166641 runs  -0.37%
  ✔ RoaringBitmap64 334,505.58 ops/sec  ±0.42%  167253 runs  fastest
  ➔ Fastest is RoaringBitmap64

• suite union (in place)
  65536 elements
  ✔ Set                 174.11 ops/sec  ±2.15%    88 runs  -95.76%
  ✔ Set<bigint>         111.61 ops/sec  ±5.08%    57 runs  -97.28%
  ✔ FastBitSet       2,057.15 ops/sec  ±1.77%  1029 runs  -49.90%
  ✔ RoaringBitmap32  4,105.73 ops/sec  ±2.44%  2053 runs  fastest
  ✔ RoaringBitmap64  1,622.55 ops/sec  ±0.21%   812 runs  -60.48%
  ➔ Fastest is RoaringBitmap32

• suite intersection (in place)
  65536 elements
  ✔ Set                 156.12 ops/sec  ±2.98%    79 runs  -95.88%
  ✔ Set<bigint>          90.01 ops/sec  ±8.97%    46 runs  -97.62%
  ✔ FastBitSet       2,087.31 ops/sec  ±1.22%  1044 runs  -44.88%
  ✔ RoaringBitmap32  3,786.80 ops/sec  ±1.44%  1894 runs  fastest
  ✔ RoaringBitmap64  1,572.23 ops/sec  ±0.34%   787 runs  -58.48%
  ➔ Fastest is RoaringBitmap32
```

### Memory benchmarks (RoaringBitmap64 vs `Set<bigint>`)

`RoaringBitmap64` is internally a map of 32-bit bitmaps, so on values that
spread across many high 32-bit buckets it can use more memory than a native
`Set<bigint>`. To measure this, run:

```sh
npm run benchmarks:memory
```

The benchmark builds the same values with `Set<number>`, `Set<bigint>`,
`RoaringBitmap32`, and `RoaringBitmap64` across several distributions.

Sample output (Apple M1, Node 22, `n` is the number of values inserted):

```
## 1. Dense low-range (every 3rd integer in [0, 3n))  (n = 1,000,000)
impl                        container/heap      per value     serialized   array/run/bitset
Set<number>                      20.01 MiB    20.98 B/val              —   —
Set<bigint>                      42.89 MiB    44.97 B/val              —   —
RoaringBitmap32                  368.0 KiB     0.38 B/val      368.4 KiB   0/0/46
RoaringBitmap64                  368.0 KiB     0.38 B/val      368.4 KiB   0/0/46

## 2. Sparse random 32-bit values  (n = 100,000)
impl                        container/heap      per value     serialized   array/run/bitset
Set<number>                       3.26 MiB    34.23 B/val              —   —
Set<bigint>                       4.79 MiB    50.23 B/val              —   —
RoaringBitmap32                  195.3 KiB     2.00 B/val      658.3 KiB   59261/0/0
RoaringBitmap64                  195.3 KiB     2.00 B/val      658.3 KiB   59261/0/0

## 3. Contiguous run [0, n)  (n = 1,000,000)
impl                        container/heap      per value     serialized   array/run/bitset
Set<number>                      20.00 MiB    20.98 B/val              —   —
Set<bigint>                      42.89 MiB    44.98 B/val              —   —
RoaringBitmap32                  128.0 KiB     0.13 B/val      128.1 KiB   0/0/16
RoaringBitmap32 (run-opt)             96 B     0.00 B/val          230 B   0/16/0
RoaringBitmap64                  128.0 KiB     0.13 B/val      128.1 KiB   0/0/16
RoaringBitmap64 (run-opt)             96 B     0.00 B/val          242 B   0/16/0

## 4. Very sparse 64-bit (one value per high bucket)  (n = 10,000)
impl                        container/heap      per value     serialized   array/run/bitset
Set<bigint>                      560.2 KiB    57.36 B/val              —   —
RoaringBitmap64                   19.5 KiB     2.00 B/val      214.9 KiB   10000/0/0

## 5. Clustered 64-bit (1024 high buckets x 1024 vals each)  (n = 1,048,576)
impl                        container/heap      per value     serialized   array/run/bitset
Set<bigint>                      44.01 MiB    44.01 B/val              —   —
RoaringBitmap64                   2.00 MiB     2.00 B/val       2.02 MiB   1024/0/0
RoaringBitmap64 (run-opt)          6.0 KiB     0.01 B/val       19.0 KiB   0/1024/0
```

Columns:

- `container/heap` — `bytesIn{Array,Run,Bitset}Containers` from
  `statistics()` for roaring rows; process heap delta for `Set` rows.
- `serialized` — `getSerializationSizeInBytes("portable")`.
- `array/run/bitset` — container-mix after construction; `(run-opt)` rows
  are after `runOptimize()` + `shrinkToFit()`.

## Branches

Branch `publish` contains the latest published stable version.

Branch `master` is the development branch that may contain code not yet published or ready for production.
If you want to contribute and submit a pull request, use the master branch.

## For collaborators

To release a new version:

- To update to the latest version of CRoaring run `./scripts/update-roaring.sh`, this will pull the CRoaring as submodule to its latest version from CRoaring dev branch.
- Ensure the version is manually increased in `package.json`
- Locally, run `npm run build` and `npm run doc` to generate the final C source and generate the doc
- Be sure `master` contains all the changes, and all is pushed.
- Merge `master` into `publish` with a pull request, this will create a new release and prebuild and publish all the binaries for the new release. This will also update the docs.
- Verify that the github action `Publish` went well and is completed. This action is triggered by any change in `publish` branch.
- Go to https://github.com/SalvatorePreviti/roaring-node/releases and verify the release is present and published
- Run manually the github action `publish to npm` via the Button in the actions page only if all the previous steps were successful
