#include "compat.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>

typedef struct
{
  char prefix;
  ALIGN64(unsigned char bytes[129]);
} aligned_test_t;

_Static_assert(_Alignof(aligned_test_t) == 64, "C11 member alignment");
_Static_assert(offsetof(aligned_test_t, bytes) % 64 == 0, "C11 member offset");

static void test_large_allocations(void)
{
  const size_t sizes[] = {1, 129, 2 * 1024 * 1024 - 1,
                          2 * 1024 * 1024, 8 * 1024 * 1024 + 3};
  for (int iprefer = 0; iprefer <= 1; iprefer++)
  {
    for (size_t isize = 0; isize < sizeof(sizes) / sizeof(sizes[0]); isize++)
    {
      compat_large_allocation_t allocation;
      unsigned char *bytes = compat_large_alloc(sizes[isize], iprefer,
                                                 &allocation);
      assert(bytes != NULL && (uintptr_t)bytes % 64 == 0);
      assert(bytes == allocation.pointer && allocation.size >= sizes[isize]);
      if (!iprefer)
        assert(allocation.page_mode == COMPAT_PAGES_REGULAR);
      memset(bytes, 0xa5, sizes[isize]);
      for (size_t ibyte = 0; ibyte < sizes[isize]; ibyte++)
        assert(bytes[ibyte] == 0xa5);
      compat_large_free(&allocation);
      assert(allocation.pointer == NULL && allocation.size == 0);
      compat_large_free(&allocation);
    }
  }
  compat_large_allocation_t allocation;
  assert(compat_large_alloc(0, 1, &allocation) == NULL);
  assert(allocation.pointer == NULL);
  assert(compat_large_alloc(128, 1, NULL) == NULL);
  compat_large_free(NULL);
  puts("large allocation, ordinary fallback and cleanup tests passed");
}

int main(void)
{
  test_large_allocations();
  if (compat_mulhi_u64(0, UINT64_MAX) != 0 ||
      compat_mulhi_u64(1, UINT64_MAX) != 0 ||
      compat_mulhi_u64(UINT64_MAX, UINT64_MAX) != UINT64_MAX - 1 ||
      compat_mulhi_u64(UINT64_C(1) << 63, 2) != 1 ||
      compat_mulhi_u64(UINT64_C(1) << 32, UINT64_C(1) << 32) != 1)
    return 1;
#if defined(__SIZEOF_INT128__)
  uint64_t value = UINT64_C(0x123456789abcdef0);
  for (int i = 0; i < 10000; ++i)
  {
    value = value * UINT64_C(6364136223846793005) + 1;
    uint64_t other = value ^ UINT64_C(0xfedcba9876543210);
    if (compat_mulhi_u64(value, other) !=
        (uint64_t)(((__uint128_t)value * other) >> 64))
      return 1;
  }
#endif
  int prefetch_value = 0;
  compat_prefetch_read(&prefetch_value);

  ALIGN64(unsigned char stack[129]);
  aligned_test_t object;
  unsigned char *heap;
  COMPAT_MALLOC(heap, 129)
  if (!heap || (uintptr_t)heap % 64 || (uintptr_t)stack % 64 ||
      (uintptr_t)object.bytes % 64)
  {
    FREE(heap)
    fputs("compat alignment test failed\n", stderr);
    return 1;
  }
  heap[128] = 42;
  FREE(heap)

  /* Repeating the test also checks cleanup/reinitialization. */
  for (int i = 0; i < 3; ++i)
    if (compat_init() != 0)
      return 1;
  puts("compat initialization tests passed");
  return 0;
}
