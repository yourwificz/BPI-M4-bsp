/*
 * (C) Copyright 2002
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

/* for now: just dummy functions to satisfy the linker */

#include <common.h>
#include <malloc.h>
#include <asm/armv8/mmu.h>

__weak void flush_cache(unsigned long start, unsigned long size)
{
#if defined(CONFIG_CPU_ARM1136)

#if !defined(CONFIG_SYS_ICACHE_OFF)
	asm("mcr p15, 0, r1, c7, c5, 0"); /* invalidate I cache */
#endif

#if !defined(CONFIG_SYS_DCACHE_OFF)
	asm("mcr p15, 0, r1, c7, c14, 0"); /* Clean+invalidate D cache */
#endif

#endif /* CONFIG_CPU_ARM1136 */

#ifdef CONFIG_CPU_ARM926EJS
#if !(defined(CONFIG_SYS_ICACHE_OFF) && defined(CONFIG_SYS_DCACHE_OFF))
	/* test and clean, page 2-23 of arm926ejs manual */
	asm("0: mrc p15, 0, r15, c7, c10, 3\n\t" "bne 0b\n" : : : "memory");
	/* disable write buffer as well (page 2-22) */
	asm("mcr p15, 0, %0, c7, c10, 4" : : "r" (0));
#endif
#endif /* CONFIG_CPU_ARM926EJS */
	return;
}

/*
 * Default implementation:
 * do a range flush for the entire range
 */
__weak void flush_dcache_all(void)
{
	flush_cache(0, ~0);
}

/*
 * Default implementation of enable_caches()
 * Real implementation should be in platform code
 */
__weak void enable_caches(void)
{
	puts("WARNING: Caches not enabled\n");
}

#ifdef CONFIG_SYS_NONCACHED_MEMORY
/*
 * Reserve one MMU section worth of address space below the malloc() area that
 * will be mapped uncached.
 */
static unsigned long noncached_start;
static unsigned long noncached_end;
static unsigned long noncached_next;

#if defined(CONFIG_BSP_REALTEK) && !defined(CONFIG_SYS_DCACHE_OFF)
void noncached_init(void)
{
	phys_addr_t start, end;
	size_t size;

	start = CONFIG_SYS_NONCACHED_START;
	size = CONFIG_SYS_NONCACHED_SIZE;
	end = start + size;

	printf("mapping memory 0x%08lx-0x%08lx non-cached\n", start, end);

	noncached_start = start;
	noncached_end = end;
	noncached_next = start;
#ifdef CONFIG_ARM64
	mmu_set_region(start, size, MT_DEVICE_NGNRNE);
#else /* !CONFIG_ARM64 */
	mmu_set_region_dcache_behaviour(start, size, DCACHE_OFF);
#endif /* CONFIG_ARM64 */
}
#else /* !#if defined(CONFIG_BSP_REALTEK) && !defined(CONFIG_SYS_DCACHE_OFF) */
void noncached_init(void)
{
	phys_addr_t start, end;
	size_t size;

	end = ALIGN(mem_malloc_start, MMU_SECTION_SIZE) - MMU_SECTION_SIZE;
	size = ALIGN(CONFIG_SYS_NONCACHED_MEMORY, MMU_SECTION_SIZE);
	start = end - size;

	debug("mapping memory %pa-%pa non-cached\n", &start, &end);

	noncached_start = start;
	noncached_end = end;
	noncached_next = start;

#ifndef CONFIG_SYS_DCACHE_OFF
	mmu_set_region_dcache_behaviour(noncached_start, size, DCACHE_OFF);
#endif
}
#endif //CONFIG_BSP_REALTEK

phys_addr_t noncached_alloc(size_t size, size_t align)
{
	phys_addr_t next = ALIGN(noncached_next, align);

	if (next >= noncached_end || (noncached_end - next) < size)
		return 0;

	debug("allocated %zu bytes of uncached memory @%pa\n", size, &next);
	noncached_next = next + size;

	return next;
}
#endif /* CONFIG_SYS_NONCACHED_MEMORY */
