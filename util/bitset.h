// Copyright 2010-2011 Google
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// Various utility functions on bitsets.

#ifndef UTIL_BITSET_H_
#define UTIL_BITSET_H_

#include <string>
#include <vector>

#include "base/basictypes.h"
#include "base/logging.h"
#include "base/util.h"

namespace operations_research {

// Basic bit operations

// Useful constants: word and double word will all bits set.
static const uint64 kAllBits64 = GG_ULONGLONG(0xFFFFFFFFFFFFFFFF);
static const uint32 kAllBits32 = 0xFFFFFFFFU;

// Returns a word with only bit pos set.
inline uint64 OneBit64(int pos) { return GG_ULONGLONG(1) << pos; }
inline uint32 OneBit32(int pos) { return 1U << pos; }

// Returns the number of bits set in n.
inline uint64 BitCount64(uint64 n) {
  const uint64 m1  = GG_ULONGLONG(0x5555555555555555);
  const uint64 m2  = GG_ULONGLONG(0x3333333333333333);
  const uint64 m4  = GG_ULONGLONG(0x0F0F0F0F0F0F0F0F);
  const uint64 h01 = GG_ULONGLONG(0x0101010101010101);
  n -= (n >> 1) & m1;
  n = (n & m2) + ((n >> 2) & m2);
  n = (n + (n >> 4)) & m4;
  n = (n * h01) >> 56;
  return n;
}
inline uint32 BitCount32(uint32 n) {
  n -= (n >> 1) & 0x55555555UL;
  n = (n & 0x33333333) + ((n >> 2) & 0x33333333UL);
  n = (n + (n >> 4)) & 0x0F0F0F0FUL;
  n = n + (n >> 8);
  n = n + (n >> 16);
  return n & 0x0000003FUL;
}

// Returns a word with only the least significant bit of n set.
inline uint64 LeastSignificantBitWord64(uint64 n) {
  return n & ~(n-1);
}
inline uint32 LeastSignificantBitWord32(uint32 n) {
  return n & ~(n-1);
}

// Returns the least significant bit position in n.
// Discussions around lsb computation.
// bsr/bsf assembly instruction are quite slow, but still a little
// win on PIII. On k8, bsf is much slower than the C-equivalent!
// Using inline assembly code yields a 10% performance gain.
// Use de Bruijn hashing instead of bsf is always a win, on both P3 and K8.
#define USE_DEBRUIJN true       // if true, use de Bruijn bit forward scanner
#if defined(ARCH_K8) || defined(ARCH_PIII)
#define USE_ASM_LEAST_SIGNIFICANT_BIT true  // if true, use assembly lsb
#endif

inline int LeastSignificantBitPosition64(uint64 n) {
#ifdef USE_ASM_LEAST_SIGNIFICANT_BIT
#ifdef ARCH_K8
  int64 pos;
  __asm__("bsfq %1, %0\n" : "=r"(pos) : "r"(n));
  return pos;
#else
  int pos;
  if (n & GG_ULONGLONG(0xFFFFFFFF)) {
    __asm__("bsfl %1, %0\n" : "=r"(pos) : "r"(n));
  } else {
    __asm__("bsfl %1, %0\n" : "=r"(pos) : "r"(n >> 32));
    pos += 32;
  }
  return pos;
#endif
#elif defined(USE_DEBRUIJN)
  // de Bruijn sequence
  static const uint64 kSeq = GG_ULONGLONG(0x0218a392dd5fb34f);
  static const int kTab[64] = {
    // initialized by 'kTab[(kSeq << i) >> 58] = i
    0,  1,  2,  7,  3, 13,  8, 19,  4, 25, 14, 28,  9, 52, 20, 58,
    5, 17, 26, 56, 15, 38, 29, 40, 10, 49, 53, 31, 21, 34, 59, 42,
    63,  6, 12, 18, 24, 27, 51, 57, 16, 55, 37, 39, 48, 30, 33, 41,
    62, 11, 23, 50, 54, 36, 47, 32, 61, 22, 35, 46, 60, 45, 44, 43,
  };
  return kTab[((n & -n) * kSeq) >> 58];
#else
  if (n == 0) {
    return -1;
  }
  int pos = 63;
  if (n & 0x00000000FFFFFFFFLL) {
    pos -= 32;
  } else {
    n >>= 32;
  }
  if (n & 0x000000000000FFFFLL) {
    pos -= 16;
  } else {
    n >>= 16;
  }
  if (n & 0x00000000000000FFLL) {
    pos -=  8;
  } else {
    n >>=  8;
  }
  if (n & 0x000000000000000FLL) {
    pos -=  4;
  } else {
    n >>=  4;
  }
  if (n & 0x0000000000000003LL) {
    pos -=  2;
  } else {
    n >>=  2;
  }
  if (n & 0x0000000000000001LL) {
    pos -=  1;
  }
  return pos;
#endif
}

inline int LeastSignificantBitPosition32(uint32 n) {
#ifdef USE_ASM_LEAST_SIGNIFICANT_BIT
  int pos;
  __asm__("bsfl %1, %0\n" : "=r"(pos) : "r"(n));
  return pos;
#elif defined(USE_DEBRUIJN)
  static const uint32 kSeq = 0x077CB531U;  // de Bruijn sequence
  static const int kTab[32] = {
    // initialized by 'kTab[(kSeq << i) >> 27] = i
     0,  1,  3,  7, 14, 29, 27, 23, 15, 31, 30, 28, 25, 18,  5, 11,
    22, 13, 26, 21, 10, 20,  9, 19,  6, 12, 24, 17,  2,  4,  8, 16
  };
  return kTab[((n & -n) * kSeq) >> 27];
#else
  if (n == 0) {
    return -1;
  }
  int pos = 31;
  if (n & 0x0000FFFFL) {
    pos -= 16;
  } else {
    n >>= 16;
  }
  if (n & 0x000000FFL) {
    pos -=  8;
  } else {
    n >>=  8;
  }
  if (n & 0x0000000FL) {
    pos -=  4;
  } else {
    n >>=  4;
  }
  if (n & 0x00000003L) {
    pos -=  2;
  } else {
    n >>=  2;
  }
  if (n & 0x00000001L) {
    pos -=  1;
  }
  return pos;
#endif
}

// Returns the most significant bit position in n.

inline int MostSignificantBitPosition64(uint64 n) {
#if USE_ASM_LEAST_SIGNIFICANT_BIT
#ifndef ARCH_K8
  int pos;
  if (n & GG_ULONGLONG(0xFFFFFFFF00000000)) {
    __asm__("bsrl %1, %0\n" : "=r"(pos) : "ro"(n >> 32) : "cc");
    pos += 32;
  } else {
    __asm__("bsrl %1, %0\n" : "=r"(pos) : "ro"(n) : "cc");
  }
  return pos;
#else
  int64 pos;
  __asm__("bsrq  %1, %0\n" : "=r"(pos) : "ro"(n) : "cc");
  return pos;
#endif
#else
  int b = 0;
  if (0 != (n & (kAllBits64 << (1 << 5)))) {
    b |= (1 << 5);
    n >>= (1 << 5);
  }
  if (0 != (n & (kAllBits64 << (1 << 4)))) {
    b |= (1 << 4);
    n >>= (1 << 4);
  }
  if (0 != (n & (kAllBits64 << (1 << 3)))) {
    b |= (1 << 3);
    n >>= (1 << 3);
  }
  if (0 != (n & (kAllBits64 << (1 << 2)))) {
    b |= (1 << 2);
    n >>= (1 << 2);
  }
  if (0 != (n & (kAllBits64 << (1 << 1)))) {
    b |= (1 << 1);
    n >>= (1 << 1);
  }
  if (0 != (n & (kAllBits64 << (1 << 0)))) {
    b |= (1 << 0);
  }
  return b;
#endif
}

inline int MostSignificantBitPosition32(uint32 n) {
#if USE_ASM_LEAST_SIGNIFICANT_BIT
  int pos;
  __asm__("bsrl %1, %0\n" : "=r"(pos) : "ro"(n) : "cc");
  return pos;
#else
  int b = 0;
  if (0 != (n & (kAllBits32 << (1 << 4)))) {
    b |= (1 << 4);
    n >>= (1 << 4);
  }
  if (0 != (n & (kAllBits32 << (1 << 3)))) {
    b |= (1 << 3);
    n >>= (1 << 3);
  }
  if (0 != (n & (kAllBits32 << (1 << 2)))) {
    b |= (1 << 2);
    n >>= (1 << 2);
  }
  if (0 != (n & (kAllBits32 << (1 << 1)))) {
    b |= (1 << 1);
    n >>= (1 << 1);
  }
  if (0 != (n & (kAllBits32 << (1 << 0)))) {
    b |= (1 << 0);
  }
  return b;
#endif
}

#undef USE_DEBRUIJN
#undef USE_ASM_BITCOUNT

// Returns a word with bits from s to e set.
inline uint64 OneRange64(uint64 s, uint64 e) {
  DCHECK_LE(s, 63);
  DCHECK_LE(e, 63);
  DCHECK_LE(s, e);
  return (kAllBits64 << s) ^ ((kAllBits64 - 1) << e);
}

inline uint32 OneRange32(uint32 s, uint32 e) {
  DCHECK_LE(s, 31);
  DCHECK_LE(e, 31);
  DCHECK_LE(s, e);
  return (kAllBits32 << s) ^ ((kAllBits32 - 1) << e);
}

// Returns a word with s least significant bits unset.
inline uint64 IntervalUp64(uint64 s) {
  DCHECK_LE(s, 63);
  return kAllBits64 << s;
}

inline uint32 IntervalUp32(uint32 s) {
  DCHECK_LE(s, 31);
  return kAllBits32 << s;
}

// Returns a word with the s most significant bits unset.
inline uint64 IntervalDown64(uint64 s) {
  DCHECK_LE(s, 63);
  return kAllBits64 >> (63 - s);
}

inline uint32 IntervalDown32(uint32 s) {
  DCHECK_LE(s, 31);
  return kAllBits32 >> (31 - s);
}

// ----- Bitset operators -----
// Bitset: array of uint32/uint64 words

// Bit operators used to manipulates bitsets.

// Returns the bit number in the word computed by BitOffsetXX,
// corresponding to the bit at position pos in the bitset.
// Note: '& 63' is faster than '% 64'
// TODO(user): rename BitPos and BitOffset to something more understandable.
inline uint64 BitPos64(uint64 pos) { return (pos & 63); }
inline uint32 BitPos32(uint32 pos) { return (pos & 31); }

// Returns the word number corresponding to bit number pos.
inline uint64 BitOffset64(uint64 pos) { return (pos >> 6); }
inline uint32 BitOffset32(uint32 pos) { return (pos >> 5); }

// Returns the number of words needed to store size bits.
inline uint64 BitLength64(uint64 size) { return ((size + 63) >> 6); }
inline uint32 BitLength32(uint32 size) { return ((size + 31) >> 5); }

// Returns the bit number in the bitset of the first bit of word number v.
inline uint64 BitShift64(uint64 v) { return v << 6; }
inline uint32 BitShift32(uint32 v) { return v << 5; }

// Returns true if the bit pos is set in bitset.
inline bool IsBitSet64(const uint64* const bitset, uint64 pos) {
  return (bitset[BitOffset64(pos)] & OneBit64(BitPos64(pos)));
}
inline bool IsBitSet32(const uint32* const bitset, uint32 pos) {
  return (bitset[BitOffset32(pos)] & OneBit32(BitPos32(pos)));
}

// Sets the bit pos to true in bitset.
inline void SetBit64(uint64* const bitset, uint64 pos) {
  bitset[BitOffset64(pos)] |= OneBit64(BitPos64(pos));
}
inline void SetBit32(uint32* const bitset, uint32 pos) {
  bitset[BitOffset32(pos)] |= OneBit32(BitPos32(pos));
}

// Sets the bit pos to false in bitset.
inline void ClearBit64(uint64* const bitset, uint64 pos) {
  bitset[BitOffset64(pos)] &= ~OneBit64(BitPos64(pos));
}
inline void ClearBit32(uint32* const bitset, uint32 pos) {
  bitset[BitOffset32(pos)] &= ~OneBit32(BitPos32(pos));
}

// Returns the number of bits set in bitset between positions start and end.
uint64 BitCountRange64(const uint64* const bitset, uint64 start, uint64 end);
uint32 BitCountRange32(const uint32* const bitset, uint32 start, uint32 end);

// Returns true if no bits are set in bitset between start and end.
bool IsEmptyRange64(const uint64* const bitset, uint64 start, uint64 end);
bool IsEmptyRange32(const uint32* const bitset, uint32 start, uint32 end);

// Returns the first bit set in bitset between start and max_bit.
int64 LeastSignificantBitPosition64(const uint64* const bitset,
                                    uint64 start, uint64 end);
int LeastSignificantBitPosition32(const uint32* const bitset,
                                  uint32 start, uint32 end);

// Returns the last bit set in bitset between min_bit and start.
int64 MostSignificantBitPosition64(const uint64* const bitset,
                                   uint64 start, uint64 end);
int MostSignificantBitPosition32(const uint32* const bitset,
                                 uint32 start, uint32 end);

// Unsafe versions of the functions above where respectively end and start
// are supposed to be set.
int64 UnsafeLeastSignificantBitPosition64(const uint64* const bitset,
                                          uint64 start, uint64 end);
int32 UnsafeLeastSignificantBitPosition32(const uint32* const bitset,
                                          uint32 start, uint32 end);

int64 UnsafeMostSignificantBitPosition64(const uint64* const bitset,
                                         uint64 start, uint64 end);
int32 UnsafeMostSignificantBitPosition32(const uint32* const bitset,
                                         uint32 start, uint32 end);

}  // namespace operations_research

#endif  // UTIL_BITSET_H_
