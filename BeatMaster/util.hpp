#ifndef _UTIL_HPP
#define _UTIL_HPP

namespace util {

// length is measured in sizeof unsigned int.
inline void memcpy(void *dst, void *src, int len) {
  unsigned int *ds_a{reinterpret_cast<unsigned int *>(dst)};
  unsigned int *sr_a{reinterpret_cast<unsigned int *>(src)};
  while (len) {
    *ds_a = *sr_a;
    ++ds_a;
    ++sr_a;
    --len;
  }
}

// length is measured in sizeof unsigned int.
inline void memset(void *dst, unsigned int v, int len) {
  unsigned int *ds_a{reinterpret_cast<unsigned int *>(dst)};
  while (len) {
    *ds_a = v;
    ++ds_a;
    --len;
  }
}

struct mem_pool {
protected:
  int m_bytes;
  unsigned char *m_pool;
  unsigned char *m_end;

public:
  mem_pool(int bytes) : m_bytes(bytes) {
    m_pool = new unsigned char[m_bytes];
    m_end = m_pool;
    util::memset(m_pool, 0, m_bytes / sizeof(unsigned int));
  }

  ~mem_pool() { delete[] m_pool; }

  unsigned char *alloc(int bytes) {
    unsigned char *ret{0};
    if (bytes >= m_bytes) // Bogus allocation
      return ret;

    if ((m_pool + m_bytes) - (m_end + sizeof(int)) >= bytes) {
      int *start{reinterpret_cast<int *>(m_end)};
      *start = bytes;
      ret = m_end + sizeof(int);
      m_end += bytes + sizeof(int);
    }

    return ret;
  }
};

} // namespace util

#endif // _UTIL_HPP