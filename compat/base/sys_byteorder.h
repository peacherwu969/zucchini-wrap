#ifndef ZUCCHINI_STANDALONE_BASE_SYS_BYTEORDER_H_
#define ZUCCHINI_STANDALONE_BASE_SYS_BYTEORDER_H_

#include <cstdint>

namespace base {
inline uint16_t ByteSwap(uint16_t x) { return __builtin_bswap16(x); }
inline uint32_t ByteSwap(uint32_t x) { return __builtin_bswap32(x); }
inline uint64_t ByteSwap(uint64_t x) { return __builtin_bswap64(x); }
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
inline uint16_t NetToHost16(uint16_t x) { return ByteSwap(x); }
inline uint32_t NetToHost32(uint32_t x) { return ByteSwap(x); }
inline uint64_t NetToHost64(uint64_t x) { return ByteSwap(x); }
inline uint16_t HostToNet16(uint16_t x) { return ByteSwap(x); }
inline uint32_t HostToNet32(uint32_t x) { return ByteSwap(x); }
inline uint64_t HostToNet64(uint64_t x) { return ByteSwap(x); }
#else
inline uint16_t NetToHost16(uint16_t x) { return x; }
inline uint32_t NetToHost32(uint32_t x) { return x; }
inline uint64_t NetToHost64(uint64_t x) { return x; }
inline uint16_t HostToNet16(uint16_t x) { return x; }
inline uint32_t HostToNet32(uint32_t x) { return x; }
inline uint64_t HostToNet64(uint64_t x) { return x; }
#endif
}  // namespace base

#endif
