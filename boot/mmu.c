#include "image.h"

typedef unsigned long u64;
typedef unsigned int u32;

/* Physical memory address space: 0-1G */
#define PHYSMEM_START	(0x0UL) //物理内存的开始
#define PERIPHERAL_BASE (0x3F000000UL) //peripheral外设基址
#define PHYSMEM_END	(0x40000000UL) //物理内存的结束
/* The number of entries in one page table page */
#define PTP_ENTRIES 512 //一页中的项数
/* The size of one page table page */
#define PTP_SIZE 4096 //页表中的页大小

/* TTBR0存放用户空间的一级页表基址，TTBR1存放内核空间的一级页表基址 */
#define ALIGN(n) __attribute__((__aligned__(n)))
/* l0 l1 l2应该是多级页表的意思 */
u64 boot_ttbr0_l0[PTP_ENTRIES] ALIGN(PTP_SIZE);
u64 boot_ttbr0_l1[PTP_ENTRIES] ALIGN(PTP_SIZE);
u64 boot_ttbr0_l2[PTP_ENTRIES] ALIGN(PTP_SIZE);

u64 boot_ttbr1_l0[PTP_ENTRIES] ALIGN(PTP_SIZE);
u64 boot_ttbr1_l1[PTP_ENTRIES] ALIGN(PTP_SIZE);
u64 boot_ttbr1_l2[PTP_ENTRIES] ALIGN(PTP_SIZE);

#define IS_VALID (1UL << 0)
#define IS_TABLE (1UL << 1)

#define UXN	       (0x1UL << 54)
#define ACCESSED       (0x1UL << 10)
#define INNER_SHARABLE (0x3UL << 8)
#define NORMAL_MEMORY  (0x4UL << 2)
#define DEVICE_MEMORY  (0x0UL << 2)

#define SIZE_2M  (2UL*1024*1024)

#define GET_L0_INDEX(x) (((x) >> (12 + 9 + 9 + 9)) & 0x1ff)
#define GET_L1_INDEX(x) (((x) >> (12 + 9 + 9)) & 0x1ff)
#define GET_L2_INDEX(x) (((x) >> (12 + 9)) & 0x1ff)

/* 初始化boot的pagetable */
void init_boot_pt(void)
{
	u32 start_entry_idx;
	u32 end_entry_idx;
	u32 idx;
	u64 kva;

	/* TTBR0_EL1 0-1G */
  /* 似乎是在这里让ttbr0_l0和l1和l2产生某种关联
  IS_TABLE | IS_VALID 是符号位 */
	boot_ttbr0_l0[0] = ((u64) boot_ttbr0_l1) | IS_TABLE | IS_VALID;
	boot_ttbr0_l1[0] = ((u64) boot_ttbr0_l2) | IS_TABLE | IS_VALID;

	/* Usuable memory: PHYSMEM_START ~ PERIPHERAL_BASE */
	start_entry_idx = PHYSMEM_START / SIZE_2M;//0
	end_entry_idx = PERIPHERAL_BASE / SIZE_2M;//504

	/* Map each 2M page */
  /* 从物理地址0~0x3F000000 在l2的每2M都填上其物理地址和标志位 */
	for (idx = start_entry_idx; idx < end_entry_idx; ++idx) {
		boot_ttbr0_l2[idx] = (PHYSMEM_START + idx * SIZE_2M)
		    | UXN	/* Unprivileged execute never */ /* 非特权级不可执行 */
		    | ACCESSED	/* Set access flag */ /* 访问位 */
		    | INNER_SHARABLE	/* Sharebility */ /* 分享位 */
		    | NORMAL_MEMORY	/* Normal memory */ /* 普通内存 */
		    | IS_VALID; /* 是否有效 */
	}
  /* 树莓派外设的内存 是0x3F000000～0x3fffffff */
	/* Peripheral memory: PERIPHERAL_BASE ~ PHYSMEM_END */

	/* Raspi3b/3b+ Peripherals: 0x3f 00 00 00 - 0x3f ff ff ff */
	start_entry_idx = end_entry_idx;
	end_entry_idx = PHYSMEM_END / SIZE_2M;

	/* Map each 2M page */
  /* 504-512 */
  /* 外设的物理内存的设置*/
	for (idx = start_entry_idx; idx < end_entry_idx; ++idx) {
		boot_ttbr0_l2[idx] = (PHYSMEM_START + idx * SIZE_2M)
		    | UXN	/* Unprivileged execute never */ /* 非特权级不可访问 */
		    | ACCESSED	/* Set access flag */ /* 访问位 */
		    | DEVICE_MEMORY	/* Device memory */ /* 设备的内存 */
		    | IS_VALID; /* 是否有效 */
	}

	/*
	 * TTBR1_EL1 0-1G
	 * KERNEL_VADDR: L0 pte index: 510; L1 pte index: 0; L2 pte index: 0.
	 */
  /* 获得虚拟地址 */
	kva = KERNEL_VADDR;
  /* 将ttbr1的虚拟地址的L0位置上的第510个位设置为L1基址 并设置位*/
	boot_ttbr1_l0[GET_L0_INDEX(kva)] = ((u64) boot_ttbr1_l1)
	    | IS_TABLE | IS_VALID;
  /* 将ttbr1的虚拟地址的L1位置上的第0个位设置为L2基址 并设置位*/
	boot_ttbr1_l1[GET_L1_INDEX(kva)] = ((u64) boot_ttbr1_l2)
	    | IS_TABLE | IS_VALID;

	start_entry_idx = GET_L2_INDEX(kva);
	/* Note: assert(start_entry_idx == 0) */
	end_entry_idx = start_entry_idx + PERIPHERAL_BASE / SIZE_2M;
	/* Note: assert(end_entry_idx < PTP_ENTIRES) */

	/*
	 * Map each 2M page
	 * Usuable memory: PHYSMEM_START ~ PERIPHERAL_BASE
	 */
  /* 同理从0~504块2Mpage,在ttbr1_l2存上物理地址和标志位 */
	for (idx = start_entry_idx; idx < end_entry_idx; ++idx) {
		boot_ttbr1_l2[idx] = (PHYSMEM_START + idx * SIZE_2M)
		    | UXN	/* Unprivileged execute never */
		    | ACCESSED	/* Set access flag */
		    | INNER_SHARABLE	/* Sharebility */
		    | NORMAL_MEMORY	/* Normal memory */
		    | IS_VALID;
	}

	/* Peripheral memory: PERIPHERAL_BASE ~ PHYSMEM_END */
	start_entry_idx = end_entry_idx;
	end_entry_idx = PHYSMEM_END / SIZE_2M;

	/* Map each 2M page */
  /* 504-512 */
	for (idx = start_entry_idx; idx < end_entry_idx; ++idx) {
		boot_ttbr1_l2[idx] = (PHYSMEM_START + idx * SIZE_2M)
		    | UXN	/* Unprivileged execute never */
		    | ACCESSED	/* Set access flag */
		    | DEVICE_MEMORY	/* Device memory */
		    | IS_VALID;
	}

  /* 本地设备 */
	/*
	 * Local peripherals, e.g., ARM timer, IRQs, and mailboxes
	 *
	 * 0x4000_0000 .. 0xFFFF_FFFF
	 * 1G is enough. Map 1G page here.
	 */
	kva = KERNEL_VADDR + PHYSMEM_END;
  /* GET_L1_INDEX(kva) == 1 */
	boot_ttbr1_l1[GET_L1_INDEX(kva)] = PHYSMEM_END | UXN	/* Unprivileged execute never */
	    | ACCESSED		/* Set access flag */
	    | DEVICE_MEMORY	/* Device memory */
	    | IS_VALID;
}

/*
boot_ttbr0_l0[0] = boot_ttbr0_l1) | FLAG;
boot_ttbr0_l1[0] = boot_ttbr0_l2) | FLAG;
boot_ttbr0_l2[0-504] = (PHYSMEM_START + idx * SIZE_2M) |FLAG1
boot_ttbr0_l2[504-512] = (PHYSMEM_START + idx * SIZE_2M) |FLAG2

boot_ttbr1_l0[510]=boot_ttbr1_l1|FLAG
boot_ttbr1_l1[0]=boot_ttbr1_l2|FLAG
boot_ttbr1_l1[1]= PHYSMEM_END |FLAG3
boot_ttbr1_l2[0-504] = (PHYSMEM_START + idx * SIZE_2M )|FLAG1
boot_ttbr1_l2[504-512] = (PHYSMEM_START + idx * SIZE_2M) |FLAG2
 */
