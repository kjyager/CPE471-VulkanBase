// Ignore warnings present in the VMA library itself.

#pragma warning(push) 
#pragma warning(disable:4068)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wall"
#pragma GCC diagnostic ignored "-Wunknown-pragmas"
#pragma GCC diagnostic ignored "-Wunused-variable"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"
#pragma clang diagnostic ignored "-Wnested-anon-types"

/* Special implementation file for the VulkanMemoryAllocator library */
#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"

#pragma clang diagnostic pop
#pragma GCC diagnostic pop
#pragma warning(pop)