#include <common/asm.h>

.extern arm64_elX_to_el1
.extern boot_cpu_stack
.extern secondary_boot_flag
.extern clear_bss_flag
.extern init_c

/* 哦，是只有一个主处理器来“做事”对么？ */
BEGIN_FUNC(_start)
	mrs	x8, mpidr_el1 //mrs将mpidr_el1保存到x8 (mpidr_el1 多处理器关联寄存器 就是当前核心处理器单元的cpuid)
	and	x8, x8,	#0xFF /* x8是32位寄存器，保留后8位 */
	cbz	x8, primary /* 若低8位（Aff0）== 0 主处理器跳转到primary */

  /* hang all secondary processors before we intorduce multi-processors */
secondary_hang:
	bl secondary_hang /* 非主处理器自旋 */

primary:

	/* Turn to el1 from other exception levels. */
  /* bootloader 通过函数arm64_elX_to_el1将处理器的异常级别从其他级别切换到 EL1。*/
	bl 	arm64_elX_to_el1
	/* Prepare stack pointer and jump to C. */
	adr 	x0, boot_cpu_stack /* 将 boot_cpu_stack 标号所在的地址写入x0 */
	add 	x0, x0, #0x1000 /* x0+=4096 栈是高地址长到低地址，因此我们也先跨过去4086,达到最高地址 */
	mov 	sp, x0 /* 设置栈指针 */

	bl 	init_c /* 跳转init_c函数 */

	/* Should never be here */
	b	.
END_FUNC(_start)
