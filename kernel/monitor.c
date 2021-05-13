/*
 * Copyright (c) 2020 Institute of Parallel And Distributed Systems (IPADS), Shanghai Jiao Tong University (SJTU)
 * OS-Lab-2020 (i.e., ChCore) is licensed under the Mulan PSL v1.
 * You can use this software according to the terms and conditions of the Mulan PSL v1.
 * You may obtain a copy of Mulan PSL v1 at:
 *   http://license.coscl.org.cn/MulanPSL
 *   THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR
 *   IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR
 *   PURPOSE.
 *   See the Mulan PSL v1 for more details.
 */

// Simple command-line kernel monitor useful for
// controlling the kernel and exploring the system interactively.

#include <common/printk.h>
#include <common/types.h>

static inline __attribute__ ((always_inline))
/* 读取栈帧 */
u64 read_fp()
{
	u64 fp;
	__asm __volatile("mov %0, x29":"=r"(fp));
	return fp;
}

/* lab1用来跟踪栈帧的 */
__attribute__ ((optimize("O1")))
int stack_backtrace()
{
/*
32字节
{
oldFp;//8
lr;//8
arg;//8
}fp
 */
  /* fp上会存储 oldFp, lr, arg */
  typedef struct Fp {
    struct Fp *oldFp;
    u64 lr;
    u64 arg;
  } Fp;

  printk("Stack backtrace:\n");
  /* 获得 stack_backtrace 栈底fp（高地址）*/
  Fp *fp = (Fp *)read_fp();
  /* 打印上一层的栈寄存器和参数 */
  while (fp->oldFp) {
    /* 保存在栈中的已经是上一层的内容了 */
    /* 不过这里我们需要的lr得是上一层栈帧的lr的所以... */
    /* 注意到这一层栈会保存上一层的x19，然后本层的x19=参数x0,
    所以我们只用本层fp->arg就可以知道上一层的参数（科科，这是定理么） */
    printk("LR %lx FP %lx Args %lx\n", fp->oldFp->lr, fp->oldFp, fp->arg);
    fp = (Fp *)fp->oldFp;
  }
	return 0;
}