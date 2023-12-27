// Copyright 2019 The TCMalloc Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "absl/types/span.h"
#include "tcmalloc/common.h"
#include "tcmalloc/internal/config.h"
#include "tcmalloc/size_class_info.h"
#include "tcmalloc/sizemap.h"

GOOGLE_MALLOC_SECTION_BEGIN
namespace tcmalloc {
namespace tcmalloc_internal {

// Columns in the following tables:
// - bytes: size of the size class
// - pages: number of pages per span
// - batch: preferred number of objects for transfers between caches
// - cap: maximum cpu cache capacity
// - class: size class number
// - objs: number of objects per span
// - waste: fixed per-size-class overhead due to end-of-span fragmentation
//   and other factors. For instance, if we have a 96 byte size class, and use
//   a single 8KiB page, then we will hold 85 objects per span, and have 32
//   bytes left over. There is also a fixed component of 48 bytes of TCMalloc
//   metadata per span. Together, the fixed overhead would be wasted/allocated
//   = (32 + 48) / (8192 - 32) ~= 0.98%.
// - inc: increment from the previous size class. This caps the dynamic
//   overhead component based on mismatches between the number of bytes
//   requested and the number of bytes provided by the size class. Together
//   they sum to the total overhead; for instance if you asked for a 50-byte
//   allocation that rounds up to a 64-byte size class, the dynamic overhead
//   would be 28%, and if waste were 22% it would mean (on average) 25 bytes
//   of overhead for allocations of that size.

// clang-format off
#if defined(__cpp_aligned_new) && __STDCPP_DEFAULT_NEW_ALIGNMENT__ <= 8
#if TCMALLOC_PAGE_SHIFT == 13
static_assert(kMaxSize == 262144, "kMaxSize mismatch");
static const int kCount = 82;
static_assert(kCount <= kNumBaseClasses);
static constexpr SizeClassInfo kSizeClassesList[kCount] = {
//  bytes pages batch   cap    class  objs  waste     inc
  {     0,    0,    0,    0},  //  0     0  0.00%   0.00%
  {     8,    1,   32, 2024},  //  0  1024  0.58%   0.00%
  {    16,    1,   32, 2024},  //  1   512  0.58% 100.00%
  {    32,    1,   32, 2027},  //  2   256  0.58% 100.00%
  {    64,    1,   32, 2024},  //  3   128  0.58% 100.00%
  {    72,    1,   32, 1275},  //  4   113  1.26%  12.50%
  {    80,    1,   32, 2024},  //  5   102  0.97%  11.11%
  {    88,    1,   32, 1031},  //  6    93  0.68%  10.00%
  {    96,    1,   32, 1206},  //  7    85  0.97%   9.09%
  {   104,    1,   32,  489},  //  8    78  1.55%   8.33%
  {   112,    1,   32,  804},  //  9    73  0.78%   7.69%
  {   120,    1,   32,  505},  // 10    68  0.97%   7.14%
  {   128,    1,   32,  957},  // 11    64  0.58%   6.67%
  {   136,    1,   32,  355},  // 12    60  0.97%   6.25%
  {   144,    1,   32,  646},  // 13    56  2.14%   5.88%
  {   160,    1,   32,  721},  // 14    51  0.97%  11.11%
  {   176,    1,   32,  378},  // 15    46  1.75%  10.00%
  {   192,    1,   32,  491},  // 16    42  2.14%   9.09%
  {   208,    1,   32,  326},  // 17    39  1.55%   8.33%
  {   224,    1,   32,  284},  // 18    36  2.14%   7.69%
  {   240,    1,   32,  266},  // 19    34  0.97%   7.14%
  {   256,    1,   32,  613},  // 20    32  0.58%   6.67%
  {   264,    1,   32,  155},  // 21    31  0.68%   3.12%
  {   280,    1,   32,  292},  // 22    29  1.46%   6.06%
  {   312,    1,   32,  347},  // 23    26  1.55%  11.43%
  {   336,    1,   32,  360},  // 24    24  2.14%   7.69%
  {   352,    1,   32,  188},  // 25    23  1.75%   4.76%
  {   384,    1,   32,  244},  // 26    21  2.14%   9.09%
  {   408,    1,   32,  213},  // 27    20  0.97%   6.25%
  {   424,    1,   32,  162},  // 28    19  2.23%   3.92%
  {   448,    1,   32,  232},  // 29    18  2.14%   5.66%
  {   480,    1,   32,  194},  // 30    17  0.97%   7.14%
  {   512,    1,   32,  409},  // 31    16  0.58%   6.67%
  {   576,    1,   32,  252},  // 32    14  2.14%  12.50%
  {   640,    1,   32,  214},  // 33    12  6.80%  11.11%
  {   704,    1,   32,  188},  // 34    11  6.02%  10.00%
  {   768,    1,   32,  185},  // 35    10  6.80%   9.09%
  {   896,    1,   32,  203},  // 36     9  2.14%  16.67%
  {  1024,    1,   32,  377},  // 37     8  0.58%  14.29%
  {  1152,    2,   32,  192},  // 38    14  1.85%  12.50%
  {  1280,    2,   32,  170},  // 39    12  6.52%  11.11%
  {  1408,    2,   32,  160},  // 40    11  5.74%  10.00%
  {  1536,    2,   32,  166},  // 41    10  6.52%   9.09%
  {  1792,    2,   32,  163},  // 42     9  1.85%  16.67%
  {  2048,    2,   32,  202},  // 43     8  0.29%  14.29%
  {  2304,    2,   28,  158},  // 44     7  1.85%  12.50%
  {  2688,    2,   24,  149},  // 45     6  1.85%  16.67%
  {  2816,    3,   23,  134},  // 46     8  8.51%   4.76%
  {  3200,    2,   20,  141},  // 47     5  2.63%  13.64%
  {  3456,    3,   18,  133},  // 48     7  1.75%   8.00%
  {  3584,    4,   18,  131},  // 49     9  1.71%   3.70%
  {  4096,    1,   16,  350},  // 50     2  0.58%  14.29%
  {  4736,    3,   13,  140},  // 51     5  3.83%  15.62%
  {  5376,    2,   12,  132},  // 52     3  1.85%  13.51%
  {  6144,    3,   10,  140},  // 53     4  0.19%  14.29%
  {  7168,    7,    9,  134},  // 54     8  0.08%  16.67%
  {  8192,    1,    8,  207},  // 55     1  0.58%  14.29%
  {  9472,    5,    6,  134},  // 56     4  7.61%  15.62%
  { 10240,    4,    6,  129},  // 57     3  6.39%   8.11%
  { 12288,    3,    5,  134},  // 58     2  0.19%  20.00%
  { 13568,    5,    4,  129},  // 59     3  0.74%  10.42%
  { 14336,    7,    4,  128},  // 60     4  0.08%   5.66%
  { 16384,    2,    4,  141},  // 61     1  0.29%  14.29%
  { 20480,    5,    3,  132},  // 62     2  0.12%  25.00%
  { 24576,    3,    2,  131},  // 63     1  0.19%  20.00%
  { 28672,    7,    2,  130},  // 64     2  0.08%  16.67%
  { 32768,    4,    2,  143},  // 65     1  0.15%  14.29%
  { 40960,    5,    2,  130},  // 66     1  0.12%  25.00%
  { 49152,    6,    2,  128},  // 67     1  0.10%  20.00%
  { 57344,    7,    2,  128},  // 68     1  0.08%  16.67%
  { 65536,    8,    2,  133},  // 69     1  0.07%  14.29%
  { 73728,    9,    2,  129},  // 70     1  0.07%  12.50%
  { 81920,   10,    2,  128},  // 71     1  0.06%  11.11%
  { 98304,   12,    2,  128},  // 72     1  0.05%  20.00%
  {114688,   14,    2,  128},  // 73     1  0.04%  16.67%
  {131072,   16,    2,  128},  // 74     1  0.04%  14.29%
  {139264,   17,    2,  128},  // 75     1  0.03%   6.25%
  {155648,   19,    2,  127},  // 76     1  0.03%  11.76%
  {172032,   21,    2,  127},  // 77     1  0.03%  10.53%
  {204800,   25,    2,  127},  // 78     1  0.02%  19.05%
  {229376,   28,    2,  127},  // 79     1  0.02%  12.00%
  {262144,   32,    2,  128},  // 80     1  0.02%  14.29%
};
constexpr absl::Span<const SizeClassInfo> kSizeClasses(kSizeClassesList);
#elif TCMALLOC_PAGE_SHIFT == 15
static_assert(kMaxSize == 262144, "kMaxSize mismatch");
static const int kCount = 74;
static_assert(kCount <= kNumBaseClasses);
static constexpr SizeClassInfo kSizeClassesList[kCount] = {
//  bytes pages batch   cap    class  objs  waste     inc
  {     0,    0,    0,    0},  //  0     0  0.00%   0.00%
  {     8,    1,   32, 1824},  //  0  4096  0.15%   0.00%
  {    16,    1,   32, 1824},  //  1  2048  0.15% 100.00%
  {    32,    1,   32, 1824},  //  2  1024  0.15% 100.00%
  {    64,    1,   32, 1824},  //  3   512  0.15% 100.00%
  {    72,    1,   32, 1241},  //  4   455  0.17%  12.50%
  {    80,    1,   32, 1824},  //  5   409  0.29%  11.11%
  {    88,    1,   32, 1267},  //  6   372  0.24%  10.00%
  {    96,    1,   32, 1590},  //  7   341  0.24%   9.09%
  {   104,    1,   32,  718},  //  8   315  0.17%   8.33%
  {   112,    1,   32,  844},  //  9   292  0.34%   7.69%
  {   120,    1,   32,  678},  // 10   273  0.17%   7.14%
  {   128,    1,   32, 1447},  // 11   256  0.15%   6.67%
  {   136,    1,   32,  428},  // 12   240  0.54%   6.25%
  {   144,    1,   32,  599},  // 13   227  0.39%   5.88%
  {   160,    1,   32,  744},  // 14   204  0.54%  11.11%
  {   176,    1,   32,  461},  // 15   186  0.24%  10.00%
  {   192,    1,   32,  603},  // 16   170  0.54%   9.09%
  {   208,    1,   32,  297},  // 17   157  0.49%   8.33%
  {   240,    1,   32,  686},  // 18   136  0.54%  15.38%
  {   256,    1,   32,  811},  // 19   128  0.15%   6.67%
  {   280,    1,   32,  385},  // 20   117  0.17%   9.38%
  {   304,    1,   32,  289},  // 21   107  0.88%   8.57%
  {   320,    1,   32,  203},  // 22   102  0.54%   5.26%
  {   352,    1,   32,  398},  // 23    93  0.24%  10.00%
  {   400,    1,   32,  298},  // 24    81  1.27%  13.64%
  {   448,    1,   32,  255},  // 25    73  0.34%  12.00%
  {   512,    1,   32,  480},  // 26    64  0.15%  14.29%
  {   576,    1,   32,  238},  // 27    56  1.71%  12.50%
  {   640,    1,   32,  284},  // 28    51  0.54%  11.11%
  {   704,    1,   32,  223},  // 29    46  1.32%  10.00%
  {   768,    1,   32,  198},  // 30    42  1.71%   9.09%
  {   896,    1,   32,  257},  // 31    36  1.71%  16.67%
  {  1024,    1,   32,  364},  // 32    32  0.15%  14.29%
  {  1152,    1,   32,  197},  // 33    28  1.71%  12.50%
  {  1280,    1,   32,  175},  // 34    25  2.49%  11.11%
  {  1408,    1,   32,  175},  // 35    23  1.32%  10.00%
  {  1536,    1,   32,  163},  // 36    21  1.71%   9.09%
  {  1792,    1,   32,  158},  // 37    18  1.71%  16.67%
  {  1920,    1,   32,  126},  // 38    17  0.54%   7.14%
  {  2048,    1,   32,  170},  // 39    16  0.15%   6.67%
  {  2176,    1,   30,  162},  // 40    15  0.54%   6.25%
  {  2304,    1,   28,  130},  // 41    14  1.71%   5.88%
  {  2688,    1,   24,  153},  // 42    12  1.71%  16.67%
  {  3200,    1,   20,  142},  // 43    10  2.49%  19.05%
  {  3584,    1,   18,  127},  // 44     9  1.71%  12.00%
  {  4096,    1,   16,  321},  // 45     8  0.15%  14.29%
  {  4608,    1,   14,  135},  // 46     7  1.71%  12.50%
  {  5376,    1,   12,  128},  // 47     6  1.71%  16.67%
  {  6528,    1,   10,  143},  // 48     5  0.54%  21.43%
  {  8192,    1,    8,  165},  // 49     4  0.15%  25.49%
  {  9344,    2,    7,  127},  // 50     7  0.27%  14.06%
  { 10880,    1,    6,  120},  // 51     3  0.54%  16.44%
  { 13056,    2,    5,  122},  // 52     5  0.46%  20.00%
  { 13952,    3,    4,  116},  // 53     7  0.70%   6.86%
  { 16384,    1,    4,  146},  // 54     2  0.15%  17.43%
  { 19072,    3,    3,  125},  // 55     5  3.04%  16.41%
  { 21760,    2,    3,  117},  // 56     3  0.46%  14.09%
  { 24576,    3,    2,  117},  // 57     4  0.05%  12.94%
  { 28672,    7,    2,  121},  // 58     8  0.02%  16.67%
  { 32768,    1,    2,  135},  // 59     1  0.15%  14.29%
  { 38144,    5,    2,  117},  // 60     4  6.90%  16.41%
  { 40960,    4,    2,  114},  // 61     3  6.28%   7.38%
  { 49152,    3,    2,  115},  // 62     2  0.05%  20.00%
  { 57344,    7,    2,  117},  // 63     4  0.02%  16.67%
  { 65536,    2,    2,  123},  // 64     1  0.07%  14.29%
  { 81920,    5,    2,  118},  // 65     2  0.03%  25.00%
  { 98304,    3,    2,  115},  // 66     1  0.05%  20.00%
  {114688,    7,    2,  115},  // 67     2  0.02%  16.67%
  {131072,    4,    2,  142},  // 68     1  0.04%  14.29%
  {163840,    5,    2,  115},  // 69     1  0.03%  25.00%
  {196608,    6,    2,  115},  // 70     1  0.02%  20.00%
  {229376,    7,    2,  113},  // 71     1  0.02%  16.67%
  {262144,    8,    2,  117},  // 72     1  0.02%  14.29%
};
constexpr absl::Span<const SizeClassInfo> kSizeClasses(kSizeClassesList);
#elif TCMALLOC_PAGE_SHIFT == 18
static_assert(kMaxSize == 262144, "kMaxSize mismatch");
static const int kCount = 85;
static_assert(kCount <= kNumBaseClasses);
static constexpr SizeClassInfo kSizeClassesList[kCount] = {
//  bytes pages batch   cap    class  objs  waste     inc
  {     0,    0,    0,    0},  //  0     0  0.00%   0.00%
  {     8,    1,   32, 1912},  //  0 32768  0.02%   0.00%
  {    16,    1,   32, 1912},  //  1 16384  0.02% 100.00%
  {    32,    1,   32, 1912},  //  2  8192  0.02% 100.00%
  {    64,    1,   32, 1918},  //  3  4096  0.02% 100.00%
  {    72,    1,   32, 1912},  //  4  3640  0.04%  12.50%
  {    80,    1,   32, 1691},  //  5  3276  0.04%  11.11%
  {    88,    1,   32,  632},  //  6  2978  0.05%  10.00%
  {    96,    1,   32,  898},  //  7  2730  0.04%   9.09%
  {   104,    1,   32,  510},  //  8  2520  0.04%   8.33%
  {   112,    1,   32,  758},  //  9  2340  0.04%   7.69%
  {   128,    1,   32, 1197},  // 10  2048  0.02%  14.29%
  {   144,    1,   32,  992},  // 11  1820  0.04%  12.50%
  {   160,    1,   32,  841},  // 12  1638  0.04%  11.11%
  {   176,    1,   32,  348},  // 13  1489  0.05%  10.00%
  {   192,    1,   32,  415},  // 14  1365  0.04%   9.09%
  {   208,    1,   32,  299},  // 15  1260  0.04%   8.33%
  {   232,    1,   32,  623},  // 16  1129  0.10%  11.54%
  {   256,    1,   32,  737},  // 17  1024  0.02%  10.34%
  {   280,    1,   32,  365},  // 18   936  0.04%   9.38%
  {   312,    1,   32,  538},  // 19   840  0.04%  11.43%
  {   336,    1,   32,  448},  // 20   780  0.04%   7.69%
  {   376,    1,   32,  220},  // 21   697  0.05%  11.90%
  {   416,    1,   32,  295},  // 22   630  0.04%  10.64%
  {   472,    1,   32,  275},  // 23   555  0.09%  13.46%
  {   512,    1,   32,  339},  // 24   512  0.02%   8.47%
  {   576,    1,   32,  266},  // 25   455  0.04%  12.50%
  {   704,    1,   32,  320},  // 26   372  0.12%  22.22%
  {   768,    1,   32,  181},  // 27   341  0.12%   9.09%
  {   896,    1,   32,  212},  // 28   292  0.21%  16.67%
  {  1024,    1,   32,  340},  // 29   256  0.02%  14.29%
  {  1152,    1,   32,  194},  // 30   227  0.26%  12.50%
  {  1280,    1,   32,  170},  // 31   204  0.41%  11.11%
  {  1408,    1,   32,  148},  // 32   186  0.12%  10.00%
  {  1664,    1,   32,  258},  // 33   157  0.36%  18.18%
  {  1920,    1,   32,  212},  // 34   136  0.41%  15.38%
  {  2048,    1,   32,  183},  // 35   128  0.02%   6.67%
  {  2176,    1,   30,  312},  // 36   120  0.41%   6.25%
  {  2304,    1,   28,  153},  // 37   113  0.70%   5.88%
  {  2560,    1,   25,  146},  // 38   102  0.41%  11.11%
  {  2816,    1,   23,  129},  // 39    93  0.12%  10.00%
  {  3072,    1,   21,  130},  // 40    85  0.41%   9.09%
  {  3328,    1,   19,  147},  // 41    78  0.99%   8.33%
  {  3584,    1,   18,  126},  // 42    73  0.21%   7.69%
  {  3840,    1,   17,  126},  // 43    68  0.41%   7.14%
  {  4096,    1,   16,  273},  // 44    64  0.02%   6.67%
  {  4224,    1,   15,  132},  // 45    62  0.12%   3.12%
  {  4736,    1,   13,  136},  // 46    55  0.65%  12.12%
  {  5248,    1,   12,  147},  // 47    49  1.92%  10.81%
  {  5760,    1,   11,  127},  // 48    45  1.14%   9.76%
  {  6528,    1,   10,  134},  // 49    40  0.41%  13.33%
  {  7168,    1,    9,  123},  // 50    36  1.58%   9.80%
  {  8192,    1,    8,  167},  // 51    32  0.02%  14.29%
  {  9344,    1,    7,  130},  // 52    28  0.21%  14.06%
  { 10880,    1,    6,  126},  // 53    24  0.41%  16.44%
  { 11904,    1,    5,  129},  // 54    22  0.12%   9.41%
  { 13056,    1,    5,  126},  // 55    20  0.41%   9.68%
  { 13696,    1,    4,  120},  // 56    19  0.75%   4.90%
  { 14464,    1,    4,  121},  // 57    18  0.70%   5.61%
  { 15360,    1,    4,  121},  // 58    17  0.41%   6.19%
  { 16384,    1,    4,  139},  // 59    16  0.02%   6.67%
  { 17408,    1,    3,  123},  // 60    15  0.41%   6.25%
  { 18688,    1,    3,  125},  // 61    14  0.21%   7.35%
  { 20096,    1,    3,  120},  // 62    13  0.36%   7.53%
  { 21760,    1,    3,  121},  // 63    12  0.41%   8.28%
  { 23808,    1,    2,  125},  // 64    11  0.12%   9.41%
  { 26112,    1,    2,  122},  // 65    10  0.41%   9.68%
  { 29056,    1,    2,  120},  // 66     9  0.26%  11.27%
  { 32768,    1,    2,  170},  // 67     8  0.02%  12.78%
  { 37376,    1,    2,  122},  // 68     7  0.21%  14.06%
  { 43648,    1,    2,  120},  // 69     6  0.12%  16.78%
  { 45568,    2,    2,  119},  // 70    11  4.40%   4.40%
  { 52352,    1,    2,  120},  // 71     5  0.16%  14.89%
  { 56064,    2,    2,  119},  // 72     9  3.77%   7.09%
  { 65536,    1,    2,  122},  // 73     4  0.02%  16.89%
  { 74880,    2,    2,  120},  // 74     7  0.03%  14.26%
  { 87296,    1,    2,  120},  // 75     3  0.12%  16.58%
  {104832,    2,    2,  120},  // 76     5  0.03%  20.09%
  {112256,    3,    2,  119},  // 77     7  0.09%   7.08%
  {131072,    1,    2,  120},  // 78     2  0.02%  16.76%
  {149760,    3,    2,  119},  // 79     5  4.79%  14.26%
  {174720,    2,    2,  119},  // 80     3  0.03%  16.67%
  {196608,    3,    2,  119},  // 81     4  0.01%  12.53%
  {209664,    4,    2,  119},  // 82     5  0.03%   6.64%
  {262144,    1,    2,  122},  // 83     1  0.02%  25.03%
};
constexpr absl::Span<const SizeClassInfo> kSizeClasses(kSizeClassesList);
#elif TCMALLOC_PAGE_SHIFT == 12
static_assert(kMaxSize == 8192, "kMaxSize mismatch");
static const int kCount = 42;
static_assert(kCount <= kNumBaseClasses);
static constexpr SizeClassInfo kSizeClassesList[kCount] = {
//  bytes pages batch   cap    class  objs  waste     inc
  {     0,    0,    0,    0},  //  0     0  0.00%   0.00%
  {     8,    1,   32, 2622},  //  0   512  1.16%   0.00%
  {    16,    1,   32, 2622},  //  1   256  1.16% 100.00%
  {    32,    1,   32, 2622},  //  2   128  1.16% 100.00%
  {    64,    1,   32, 2622},  //  3    64  1.16% 100.00%
  {    72,    1,   32,  927},  //  4    56  2.70%  12.50%
  {    80,    1,   32, 2622},  //  5    51  1.54%  11.11%
  {    96,    1,   32, 2160},  //  6    42  2.70%  20.00%
  {   104,    1,   32,  670},  //  7    39  2.12%   8.33%
  {   112,    1,   32, 1197},  //  8    36  2.70%   7.69%
  {   128,    1,   32, 1607},  //  9    32  1.16%  14.29%
  {   144,    1,   32, 1292},  // 10    28  2.70%  12.50%
  {   160,    1,   32, 1167},  // 11    25  3.47%  11.11%
  {   176,    1,   32,  563},  // 12    23  2.32%  10.00%
  {   192,    1,   32,  610},  // 13    21  2.70%   9.09%
  {   208,    1,   32,  394},  // 14    19  4.63%   8.33%
  {   224,    1,   32,  551},  // 15    18  2.70%   7.69%
  {   240,    1,   32,  319},  // 16    17  1.54%   7.14%
  {   256,    1,   32,  598},  // 17    16  1.16%   6.67%
  {   272,    1,   32,  260},  // 18    15  1.54%   6.25%
  {   288,    1,   32,  301},  // 19    14  2.70%   5.88%
  {   336,    1,   32,  579},  // 20    12  2.70%  16.67%
  {   408,    1,   32,  250},  // 21    10  1.54%  21.43%
  {   448,    1,   32,  225},  // 22     9  2.70%   9.80%
  {   512,    1,   32,  739},  // 23     8  1.16%  14.29%
  {   576,    2,   32,  338},  // 24    14  2.14%  12.50%
  {   640,    2,   32,  188},  // 25    12  6.80%  11.11%
  {   768,    2,   32,  334},  // 26    10  6.80%  20.00%
  {   896,    2,   32,  287},  // 27     9  2.14%  16.67%
  {  1024,    2,   32,  964},  // 28     8  0.58%  14.29%
  {  1152,    3,   32,  210},  // 29    10  6.61%  12.50%
  {  1280,    3,   32,  164},  // 30     9  6.61%  11.11%
  {  1536,    3,   32,  204},  // 31     8  0.39%  20.00%
  {  2048,    4,   32,  530},  // 32     8  0.29%  33.33%
  {  2304,    4,   28,  191},  // 33     7  1.85%  12.50%
  {  2688,    4,   24,  181},  // 34     6  1.85%  16.67%
  {  3200,    4,   20,  166},  // 35     5  2.63%  19.05%
  {  4096,    4,   16,  624},  // 36     4  0.29%  28.00%
  {  4736,    5,   13,  213},  // 37     4  7.72%  15.62%
  {  6144,    3,   10,  168},  // 38     2  0.39%  29.73%
  {  7168,    7,    9,  169},  // 39     4  0.17%  16.67%
  {  8192,    4,    8,  236},  // 40     2  0.29%  14.29%
};
constexpr absl::Span<const SizeClassInfo> kSizeClasses(kSizeClassesList);
#else
#error "Unsupported TCMALLOC_PAGE_SHIFT value!"
#endif
#else
#if TCMALLOC_PAGE_SHIFT == 13
static_assert(kMaxSize == 262144, "kMaxSize mismatch");
static const int kCount = 85;
static_assert(kCount <= kNumBaseClasses);
static constexpr SizeClassInfo kSizeClassesList[kCount] = {
//  bytes pages batch   cap    class  objs  waste     inc
  {     0,    0,    0,    0},  //  0     0  0.00%   0.00%
  {     8,    1,   32, 2369},  //  0  1024  0.58%   0.00%
  {    16,    1,   32, 2369},  //  1   512  0.58% 100.00%
  {    32,    1,   32, 2369},  //  2   256  0.58% 100.00%
  {    64,    1,   32, 2369},  //  3   128  0.58% 100.00%
  {    80,    1,   32, 2369},  //  4   102  0.97%  25.00%
  {    96,    1,   32, 1596},  //  5    85  0.97%  20.00%
  {   112,    1,   32,  911},  //  6    73  0.78%  16.67%
  {   128,    1,   32, 1035},  //  7    64  0.58%  14.29%
  {   144,    1,   32,  699},  //  8    56  2.14%  12.50%
  {   160,    1,   32,  586},  //  9    51  0.97%  11.11%
  {   176,    1,   32,  333},  // 10    46  1.75%  10.00%
  {   192,    1,   32,  418},  // 11    42  2.14%   9.09%
  {   208,    1,   32,  296},  // 12    39  1.55%   8.33%
  {   224,    1,   32,  264},  // 13    36  2.14%   7.69%
  {   240,    1,   32,  251},  // 14    34  0.97%   7.14%
  {   256,    1,   32,  507},  // 15    32  0.58%   6.67%
  {   272,    1,   32,  231},  // 16    30  0.97%   6.25%
  {   288,    1,   32,  264},  // 17    28  2.14%   5.88%
  {   304,    1,   32,  205},  // 18    26  4.08%   5.56%
  {   320,    1,   32,  250},  // 19    25  2.91%   5.26%
  {   336,    1,   32,  269},  // 20    24  2.14%   5.00%
  {   352,    1,   32,  193},  // 21    23  1.75%   4.76%
  {   368,    1,   32,  173},  // 22    22  1.75%   4.55%
  {   384,    1,   32,  209},  // 23    21  2.14%   4.35%
  {   400,    1,   32,  190},  // 24    20  2.91%   4.17%
  {   416,    1,   32,  187},  // 25    19  4.08%   4.00%
  {   448,    1,   32,  236},  // 26    18  2.14%   7.69%
  {   480,    1,   32,  198},  // 27    17  0.97%   7.14%
  {   512,    1,   32,  356},  // 28    16  0.58%   6.67%
  {   576,    1,   32,  241},  // 29    14  2.14%  12.50%
  {   640,    1,   32,  213},  // 30    12  6.80%  11.11%
  {   704,    1,   32,  193},  // 31    11  6.02%  10.00%
  {   768,    1,   32,  191},  // 32    10  6.80%   9.09%
  {   896,    1,   32,  205},  // 33     9  2.14%  16.67%
  {  1024,    1,   32,  332},  // 34     8  0.58%  14.29%
  {  1152,    2,   32,  197},  // 35    14  1.85%  12.50%
  {  1280,    2,   32,  180},  // 36    12  6.52%  11.11%
  {  1408,    2,   32,  172},  // 37    11  5.74%  10.00%
  {  1536,    2,   32,  178},  // 38    10  6.52%   9.09%
  {  1792,    2,   32,  175},  // 39     9  1.85%  16.67%
  {  2048,    2,   32,  204},  // 40     8  0.29%  14.29%
  {  2304,    2,   28,  171},  // 41     7  1.85%  12.50%
  {  2688,    2,   24,  165},  // 42     6  1.85%  16.67%
  {  2816,    3,   23,  154},  // 43     8  8.51%   4.76%
  {  3200,    2,   20,  160},  // 44     5  2.63%  13.64%
  {  3456,    3,   18,  153},  // 45     7  1.75%   8.00%
  {  3584,    4,   18,  152},  // 46     9  1.71%   3.70%
  {  4096,    1,   16,  312},  // 47     2  0.58%  14.29%
  {  4736,    3,   13,  158},  // 48     5  3.83%  15.62%
  {  5376,    2,   12,  153},  // 49     3  1.85%  13.51%
  {  6144,    3,   10,  158},  // 50     4  0.19%  14.29%
  {  6528,    4,   10,  150},  // 51     5  0.54%   6.25%
  {  7168,    7,    9,  152},  // 52     8  0.08%   9.80%
  {  8192,    1,    8,  207},  // 53     1  0.58%  14.29%
  {  9472,    5,    6,  154},  // 54     4  7.61%  15.62%
  { 10240,    4,    6,  150},  // 55     3  6.39%   8.11%
  { 12288,    3,    5,  154},  // 56     2  0.19%  20.00%
  { 13568,    5,    4,  150},  // 57     3  0.74%  10.42%
  { 14336,    7,    4,  149},  // 58     4  0.08%   5.66%
  { 16384,    2,    4,  160},  // 59     1  0.29%  14.29%
  { 20480,    5,    3,  153},  // 60     2  0.12%  25.00%
  { 24576,    3,    2,  152},  // 61     1  0.19%  20.00%
  { 28672,    7,    2,  152},  // 62     2  0.08%  16.67%
  { 32768,    4,    2,  161},  // 63     1  0.15%  14.29%
  { 40960,    5,    2,  150},  // 64     1  0.12%  25.00%
  { 49152,    6,    2,  149},  // 65     1  0.10%  20.00%
  { 57344,    7,    2,  149},  // 66     1  0.08%  16.67%
  { 65536,    8,    2,  153},  // 67     1  0.07%  14.29%
  { 73728,    9,    2,  150},  // 68     1  0.07%  12.50%
  { 81920,   10,    2,  149},  // 69     1  0.06%  11.11%
  { 90112,   11,    2,  148},  // 70     1  0.05%  10.00%
  { 98304,   12,    2,  149},  // 71     1  0.05%   9.09%
  {106496,   13,    2,  148},  // 72     1  0.05%   8.33%
  {114688,   14,    2,  148},  // 73     1  0.04%   7.69%
  {131072,   16,    2,  149},  // 74     1  0.04%  14.29%
  {139264,   17,    2,  149},  // 75     1  0.03%   6.25%
  {147456,   18,    2,  148},  // 76     1  0.03%   5.88%
  {155648,   19,    2,  148},  // 77     1  0.03%   5.56%
  {172032,   21,    2,  148},  // 78     1  0.03%  10.53%
  {188416,   23,    2,  148},  // 79     1  0.03%   9.52%
  {204800,   25,    2,  148},  // 80     1  0.02%   8.70%
  {221184,   27,    2,  148},  // 81     1  0.02%   8.00%
  {237568,   29,    2,  146},  // 82     1  0.02%   7.41%
  {262144,   32,    2,  148},  // 83     1  0.02%  10.34%
};
constexpr absl::Span<const SizeClassInfo> kSizeClasses(kSizeClassesList);
#elif TCMALLOC_PAGE_SHIFT == 15
static_assert(kMaxSize == 262144, "kMaxSize mismatch");
static const int kCount = 77;
static_assert(kCount <= kNumBaseClasses);
static constexpr SizeClassInfo kSizeClassesList[kCount] = {
//  bytes pages batch   cap    class  objs  waste     inc
  {     0,    0,    0,    0},  //  0     0  0.00%   0.00%
  {     8,    1,   32, 2249},  //  0  4096  0.15%   0.00%
  {    16,    1,   32, 2249},  //  1  2048  0.15% 100.00%
  {    32,    1,   32, 2249},  //  2  1024  0.15% 100.00%
  {    64,    1,   32, 2249},  //  3   512  0.15% 100.00%
  {    80,    1,   32, 2249},  //  4   409  0.29%  25.00%
  {    96,    1,   32, 2100},  //  5   341  0.24%  20.00%
  {   112,    1,   32, 1138},  //  6   292  0.34%  16.67%
  {   128,    1,   32, 1563},  //  7   256  0.15%  14.29%
  {   144,    1,   32,  739},  //  8   227  0.39%  12.50%
  {   160,    1,   32,  615},  //  9   204  0.54%  11.11%
  {   176,    1,   32,  402},  // 10   186  0.24%  10.00%
  {   192,    1,   32,  509},  // 11   170  0.54%   9.09%
  {   208,    1,   32,  279},  // 12   157  0.49%   8.33%
  {   224,    1,   32,  359},  // 13   146  0.34%   7.69%
  {   240,    1,   32,  355},  // 14   136  0.54%   7.14%
  {   256,    1,   32,  666},  // 15   128  0.15%   6.67%
  {   288,    1,   32,  382},  // 16   113  0.83%  12.50%
  {   304,    1,   32,  234},  // 17   107  0.88%   5.56%
  {   320,    1,   32,  208},  // 18   102  0.54%   5.26%
  {   352,    1,   32,  355},  // 19    93  0.24%  10.00%
  {   384,    1,   32,  244},  // 20    85  0.54%   9.09%
  {   400,    1,   32,  176},  // 21    81  1.27%   4.17%
  {   448,    1,   32,  246},  // 22    73  0.34%  12.00%
  {   480,    1,   32,  254},  // 23    68  0.54%   7.14%
  {   512,    1,   32,  304},  // 24    64  0.15%   6.67%
  {   576,    1,   32,  234},  // 25    56  1.71%  12.50%
  {   640,    1,   32,  269},  // 26    51  0.54%  11.11%
  {   704,    1,   32,  222},  // 27    46  1.32%  10.00%
  {   768,    1,   32,  204},  // 28    42  1.71%   9.09%
  {   832,    1,   32,  208},  // 29    39  1.12%   8.33%
  {   896,    1,   32,  182},  // 30    36  1.71%   7.69%
  {  1024,    1,   32,  328},  // 31    32  0.15%  14.29%
  {  1152,    1,   32,  203},  // 32    28  1.71%  12.50%
  {  1280,    1,   32,  186},  // 33    25  2.49%  11.11%
  {  1408,    1,   32,  186},  // 34    23  1.32%  10.00%
  {  1536,    1,   32,  178},  // 35    21  1.71%   9.09%
  {  1792,    1,   32,  174},  // 36    18  1.71%  16.67%
  {  1920,    1,   32,  149},  // 37    17  0.54%   7.14%
  {  2048,    1,   32,  183},  // 38    16  0.15%   6.67%
  {  2176,    1,   30,  177},  // 39    15  0.54%   6.25%
  {  2304,    1,   28,  153},  // 40    14  1.71%   5.88%
  {  2432,    1,   26,  150},  // 41    13  3.66%   5.56%
  {  2688,    1,   24,  160},  // 42    12  1.71%  10.53%
  {  2944,    1,   22,  149},  // 43    11  1.32%   9.52%
  {  3200,    1,   20,  153},  // 44    10  2.49%   8.70%
  {  3584,    1,   18,  150},  // 45     9  1.71%  12.00%
  {  4096,    1,   16,  297},  // 46     8  0.15%  14.29%
  {  4608,    1,   14,  157},  // 47     7  1.71%  12.50%
  {  5376,    1,   12,  152},  // 48     6  1.71%  16.67%
  {  6528,    1,   10,  163},  // 49     5  0.54%  21.43%
  {  7168,    2,    9,  143},  // 50     9  1.63%   9.80%
  {  8192,    1,    8,  177},  // 51     4  0.15%  14.29%
  {  9344,    2,    7,  150},  // 52     7  0.27%  14.06%
  { 10880,    1,    6,  145},  // 53     3  0.54%  16.44%
  { 13056,    2,    5,  146},  // 54     5  0.46%  20.00%
  { 13952,    3,    4,  142},  // 55     7  0.70%   6.86%
  { 16384,    1,    4,  165},  // 56     2  0.15%  17.43%
  { 19072,    3,    3,  148},  // 57     5  3.04%  16.41%
  { 21760,    2,    3,  143},  // 58     3  0.46%  14.09%
  { 24576,    3,    2,  143},  // 59     4  0.05%  12.94%
  { 26112,    4,    2,  142},  // 60     5  0.43%   6.25%
  { 28672,    7,    2,  145},  // 61     8  0.02%   9.80%
  { 32768,    1,    2,  157},  // 62     1  0.15%  14.29%
  { 38144,    5,    2,  143},  // 63     4  6.90%  16.41%
  { 40960,    4,    2,  141},  // 64     3  6.28%   7.38%
  { 49152,    3,    2,  142},  // 65     2  0.05%  20.00%
  { 57344,    7,    2,  143},  // 66     4  0.02%  16.67%
  { 65536,    2,    2,  147},  // 67     1  0.07%  14.29%
  { 81920,    5,    2,  144},  // 68     2  0.03%  25.00%
  { 98304,    3,    2,  142},  // 69     1  0.05%  20.00%
  {114688,    7,    2,  141},  // 70     2  0.02%  16.67%
  {131072,    4,    2,  161},  // 71     1  0.04%  14.29%
  {163840,    5,    2,  141},  // 72     1  0.03%  25.00%
  {196608,    6,    2,  142},  // 73     1  0.02%  20.00%
  {229376,    7,    2,  136},  // 74     1  0.02%  16.67%
  {262144,    8,    2,  143},  // 75     1  0.02%  14.29%
};
constexpr absl::Span<const SizeClassInfo> kSizeClasses(kSizeClassesList);
#elif TCMALLOC_PAGE_SHIFT == 18
static_assert(kMaxSize == 262144, "kMaxSize mismatch");
static const int kCount = 88;
static_assert(kCount <= kNumBaseClasses);
static constexpr SizeClassInfo kSizeClassesList[kCount] = {
//  bytes pages batch   cap    class  objs  waste     inc
  {     0,    0,    0,    0},  //  0     0  0.00%   0.00%
  {     8,    1,   32, 2368},  //  0 32768  0.02%   0.00%
  {    16,    1,   32, 2368},  //  1 16384  0.02% 100.00%
  {    32,    1,   32, 2368},  //  2  8192  0.02% 100.00%
  {    64,    1,   32, 2371},  //  3  4096  0.02% 100.00%
  {    80,    1,   32, 2368},  //  4  3276  0.04%  25.00%
  {    96,    1,   32, 1006},  //  5  2730  0.04%  20.00%
  {   112,    1,   32,  834},  //  6  2340  0.04%  16.67%
  {   128,    1,   32,  871},  //  7  2048  0.02%  14.29%
  {   144,    1,   32,  733},  //  8  1820  0.04%  12.50%
  {   160,    1,   32,  633},  //  9  1638  0.04%  11.11%
  {   176,    1,   32,  302},  // 10  1489  0.05%  10.00%
  {   192,    1,   32,  347},  // 11  1365  0.04%   9.09%
  {   208,    1,   32,  268},  // 12  1260  0.04%   8.33%
  {   224,    1,   32,  466},  // 13  1170  0.04%   7.69%
  {   256,    1,   32,  584},  // 14  1024  0.02%  14.29%
  {   288,    1,   32,  446},  // 15   910  0.04%  12.50%
  {   320,    1,   32,  342},  // 16   819  0.04%  11.11%
  {   336,    1,   32,  321},  // 17   780  0.04%   5.00%
  {   368,    1,   32,  199},  // 18   712  0.07%   9.52%
  {   400,    1,   32,  257},  // 19   655  0.07%   8.70%
  {   448,    1,   32,  259},  // 20   585  0.04%  12.00%
  {   480,    1,   32,  188},  // 21   546  0.04%   7.14%
  {   512,    1,   32,  275},  // 22   512  0.02%   6.67%
  {   576,    1,   32,  246},  // 23   455  0.04%  12.50%
  {   640,    1,   32,  235},  // 24   409  0.16%  11.11%
  {   704,    1,   32,  197},  // 25   372  0.12%  10.00%
  {   768,    1,   32,  190},  // 26   341  0.12%   9.09%
  {   896,    1,   32,  210},  // 27   292  0.21%  16.67%
  {  1024,    1,   32,  296},  // 28   256  0.02%  14.29%
  {  1152,    1,   32,  198},  // 29   227  0.26%  12.50%
  {  1280,    1,   32,  182},  // 30   204  0.41%  11.11%
  {  1408,    1,   32,  168},  // 31   186  0.12%  10.00%
  {  1536,    1,   32,  168},  // 32   170  0.41%   9.09%
  {  1664,    1,   32,  221},  // 33   157  0.36%   8.33%
  {  1920,    1,   32,  209},  // 34   136  0.41%  15.38%
  {  2048,    1,   32,  191},  // 35   128  0.02%   6.67%
  {  2176,    1,   30,  278},  // 36   120  0.41%   6.25%
  {  2304,    1,   28,  171},  // 37   113  0.70%   5.88%
  {  2560,    1,   25,  165},  // 38   102  0.41%  11.11%
  {  2816,    1,   23,  155},  // 39    93  0.12%  10.00%
  {  3072,    1,   21,  155},  // 40    85  0.41%   9.09%
  {  3328,    1,   19,  167},  // 41    78  0.99%   8.33%
  {  3584,    1,   18,  153},  // 42    73  0.21%   7.69%
  {  3840,    1,   17,  153},  // 43    68  0.41%   7.14%
  {  4096,    1,   16,  251},  // 44    64  0.02%   6.67%
  {  4224,    1,   15,  156},  // 45    62  0.12%   3.12%
  {  4736,    1,   13,  160},  // 46    55  0.65%  12.12%
  {  5120,    1,   12,  158},  // 47    51  0.41%   8.11%
  {  5632,    1,   11,  160},  // 48    46  1.19%  10.00%
  {  6144,    1,   10,  153},  // 49    42  1.58%   9.09%
  {  6528,    1,   10,  154},  // 50    40  0.41%   6.25%
  {  7168,    1,    9,  150},  // 51    36  1.58%   9.80%
  {  8192,    1,    8,  180},  // 52    32  0.02%  14.29%
  {  8704,    1,    7,  150},  // 53    30  0.41%   6.25%
  {  9344,    1,    7,  153},  // 54    28  0.21%   7.35%
  { 10368,    1,    6,  151},  // 55    25  1.14%  10.96%
  { 11392,    1,    5,  154},  // 56    23  0.07%   9.88%
  { 12416,    1,    5,  153},  // 57    21  0.56%   8.99%
  { 13056,    1,    5,  150},  // 58    20  0.41%   5.15%
  { 13696,    1,    4,  149},  // 59    19  0.75%   4.90%
  { 14464,    1,    4,  149},  // 60    18  0.70%   5.61%
  { 15360,    1,    4,  149},  // 61    17  0.41%   6.19%
  { 16384,    1,    4,  161},  // 62    16  0.02%   6.67%
  { 17408,    1,    3,  150},  // 63    15  0.41%   6.25%
  { 18688,    1,    3,  151},  // 64    14  0.21%   7.35%
  { 20096,    1,    3,  149},  // 65    13  0.36%   7.53%
  { 21760,    1,    3,  149},  // 66    12  0.41%   8.28%
  { 23808,    1,    2,  151},  // 67    11  0.12%   9.41%
  { 26112,    1,    2,  150},  // 68    10  0.41%   9.68%
  { 29056,    1,    2,  149},  // 69     9  0.26%  11.27%
  { 32768,    1,    2,  182},  // 70     8  0.02%  12.78%
  { 37376,    1,    2,  150},  // 71     7  0.21%  14.06%
  { 43648,    1,    2,  149},  // 72     6  0.12%  16.78%
  { 45568,    2,    2,  148},  // 73    11  4.40%   4.40%
  { 52352,    1,    2,  149},  // 74     5  0.16%  14.89%
  { 56064,    2,    2,  148},  // 75     9  3.77%   7.09%
  { 65536,    1,    2,  150},  // 76     4  0.02%  16.89%
  { 74880,    2,    2,  148},  // 77     7  0.03%  14.26%
  { 87296,    1,    2,  148},  // 78     3  0.12%  16.58%
  {104832,    2,    2,  148},  // 79     5  0.03%  20.09%
  {112256,    3,    2,  148},  // 80     7  0.09%   7.08%
  {131072,    1,    2,  148},  // 81     2  0.02%  16.76%
  {149760,    3,    2,  148},  // 82     5  4.79%  14.26%
  {174720,    2,    2,  148},  // 83     3  0.03%  16.67%
  {196608,    3,    2,  148},  // 84     4  0.01%  12.53%
  {209664,    4,    2,  148},  // 85     5  0.03%   6.64%
  {262144,    1,    2,  150},  // 86     1  0.02%  25.03%
};
constexpr absl::Span<const SizeClassInfo> kSizeClasses(kSizeClassesList);
#elif TCMALLOC_PAGE_SHIFT == 12
static_assert(kMaxSize == 8192, "kMaxSize mismatch");
static const int kCount = 45;
static_assert(kCount <= kNumBaseClasses);
static constexpr SizeClassInfo kSizeClassesList[kCount] = {
//  bytes pages batch   cap    class  objs  waste     inc
  {     0,    0,    0,    0},  //  0     0  0.00%   0.00%
  {     8,    1,   32, 2906},  //  0   512  1.16%   0.00%
  {    16,    1,   32, 2906},  //  1   256  1.16% 100.00%
  {    32,    1,   32, 2910},  //  2   128  1.16% 100.00%
  {    64,    1,   32, 2906},  //  3    64  1.16% 100.00%
  {    80,    1,   32, 2906},  //  4    51  1.54%  25.00%
  {    96,    1,   32, 1880},  //  5    42  2.70%  20.00%
  {   112,    1,   32, 1490},  //  6    36  2.70%  16.67%
  {   128,    1,   32, 1411},  //  7    32  1.16%  14.29%
  {   144,    1,   32, 1144},  //  8    28  2.70%  12.50%
  {   160,    1,   32, 1037},  //  9    25  3.47%  11.11%
  {   176,    1,   32,  525},  // 10    23  2.32%  10.00%
  {   192,    1,   32,  563},  // 11    21  2.70%   9.09%
  {   208,    1,   32,  380},  // 12    19  4.63%   8.33%
  {   224,    1,   32,  512},  // 13    18  2.70%   7.69%
  {   240,    1,   32,  316},  // 14    17  1.54%   7.14%
  {   256,    1,   32,  553},  // 15    16  1.16%   6.67%
  {   272,    1,   32,  267},  // 16    15  1.54%   6.25%
  {   288,    1,   32,  301},  // 17    14  2.70%   5.88%
  {   304,    1,   32,  261},  // 18    13  4.63%   5.56%
  {   336,    1,   32,  457},  // 19    12  2.70%  10.53%
  {   368,    1,   32,  226},  // 20    11  2.32%   9.52%
  {   400,    1,   32,  207},  // 21    10  3.47%   8.70%
  {   448,    1,   32,  241},  // 22     9  2.70%  12.00%
  {   512,    1,   32,  673},  // 23     8  1.16%  14.29%
  {   576,    2,   32,  333},  // 24    14  2.14%  12.50%
  {   640,    2,   32,  206},  // 25    12  6.80%  11.11%
  {   768,    2,   32,  329},  // 26    10  6.80%  20.00%
  {   896,    2,   32,  290},  // 27     9  2.14%  16.67%
  {  1024,    2,   32,  864},  // 28     8  0.58%  14.29%
  {  1152,    3,   32,  224},  // 29    10  6.61%  12.50%
  {  1280,    3,   32,  184},  // 30     9  6.61%  11.11%
  {  1536,    3,   32,  219},  // 31     8  0.39%  20.00%
  {  1792,    4,   32,  193},  // 32     9  1.85%  16.67%
  {  2048,    4,   32,  483},  // 33     8  0.29%  14.29%
  {  2304,    4,   28,  207},  // 34     7  1.85%  12.50%
  {  2688,    4,   24,  199},  // 35     6  1.85%  16.67%
  {  3200,    4,   20,  187},  // 36     5  2.63%  19.05%
  {  3584,    7,   18,  184},  // 37     8  0.17%  12.00%
  {  4096,    4,   16,  570},  // 38     4  0.29%  14.29%
  {  4736,    5,   13,  226},  // 39     4  7.72%  15.62%
  {  5376,    4,   12,  182},  // 40     3  1.85%  13.51%
  {  6144,    3,   10,  186},  // 41     2  0.39%  14.29%
  {  7168,    7,    9,  190},  // 42     4  0.17%  16.67%
  {  8192,    4,    8,  246},  // 43     2  0.29%  14.29%
};
constexpr absl::Span<const SizeClassInfo> kSizeClasses(kSizeClassesList);
#else
#error "Unsupported TCMALLOC_PAGE_SHIFT value!"
#endif
#endif
// clang-format on

}  // namespace tcmalloc_internal
}  // namespace tcmalloc
GOOGLE_MALLOC_SECTION_END
