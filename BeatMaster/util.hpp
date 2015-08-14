#ifndef _UTIL_HPP
#define _UTIL_HPP

namespace util {

// Length is measured in sizeof unsigned int.
// Manually unrolled the loop for a bit of speed.
inline void memcpy(void *dst, void *src, int len) {
  unsigned int *ds_a{reinterpret_cast<unsigned int *>(dst)};
  unsigned int *sr_a{reinterpret_cast<unsigned int *>(src)};
  int half_len{len >> 1};
  while (half_len) {
    *ds_a = *sr_a;
    ++ds_a;
    ++sr_a;
    *ds_a = *sr_a;
    ++ds_a;
    ++sr_a;
    --half_len;
  }
}

// Length is measured in sizeof unsigned int.
// Manually unrolled the loop for a bit of speed.
inline void memset(void *dst, unsigned int v, int len) {
  unsigned int *ds_a{reinterpret_cast<unsigned int *>(dst)};
  int half_len{len >> 1};
  while (half_len) {
    *ds_a = v;
    ++ds_a;
    *ds_a = v;
    ++ds_a;
    --half_len;
  }
}

// This structure represents a linear chunk of RAM. We pre-allocate the memory
// so that subsequent allocations from the pool can succeed quickly. As it
// stands, we cannot deallocate form the pool, but that will be added in future.
struct mem_pool {
protected:
  int m_bytes;
  unsigned char *m_pool;
  unsigned char *m_end;

public:
  // Allocate the pool with the required size. Initialize the memory to 0.
  mem_pool(int bytes) : m_bytes(bytes) {
    m_pool = new unsigned char[m_bytes];
    m_end = m_pool;
    util::memset(m_pool, 0, m_bytes / sizeof(unsigned int));
  }

  // Simply free up the reserved memory.
  ~mem_pool() { delete[] m_pool; }

  // Allocate a chunk of this memory to whatever purpose.
  unsigned char *alloc(int bytes) {
    unsigned char *ret{0};
    if (bytes >= m_bytes) // Bogus allocation
      return ret;

    // We could find a chunk that fits somewhere at the end of the block.
    if ((m_pool + m_bytes) - (m_end + sizeof(int) + sizeof(bool)) >= bytes) {
      int *start{reinterpret_cast<int *>(m_end)};
      *start = bytes;
      ret = m_end + sizeof(int);
      *ret = true; // memory is allocated.
      ++ret;
      m_end += bytes + sizeof(int) + sizeof(bool);
      return ret;
    }

// TODO(shaheed.abdol) - Add alloc/dealloc scheme.
#if 0
    // We couldn't find a free chunk at the end of the block.
    // Walk through the allocated block to find a freed chunk.
    {
      unsigned char *alias = m_pool;
      int *block_size{reinterpret_cast<int *>(alias)};
      bool notfound = true;
      while (notfound) {
        int next_block{*block_size};
        if (next_block == 0)
          return ret; // something went mad wrong.

        alias += sizeof(int);
        bool allocated{*reinterpret_cast<bool *>(alias)};
        if (!allocated) {
          // Allocate the block of memory ...
        }
        // ...
      }
    }
#endif // 0
    return ret;
  }
};

} // namespace util

#endif // _UTIL_HPP