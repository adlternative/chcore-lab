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

#include <common/kprint.h>
#include <common/macro.h>
#include <common/uart.h>
#include <common/machine.h>

ALIGN(STACK_ALIGNMENT)
/* ffffff00000a0130 */
/* 4个 8192 大小的内核栈*/
char kernel_stack[PLAT_CPU_NUM][KERNEL_STACK_SIZE];
/* sp = ffffff00000a2130  kernel_stack+8192 */
int stack_backtrace();

/* 这里似乎加了一个编译器O1优化 */
// Test the stack backtrace function (lab 1 only)
__attribute__ ((optimize("O1")))
void stack_test(long x)
{
	kinfo("entering stack_test %d\n", x);
	if (x > 0)
		stack_test(x - 1);
	else
		stack_backtrace();
	kinfo("leaving stack_test %d\n", x);
}

void main(void *addr)
{
	/* Init uart */
  /* 树莓派 UART代表通用异步收发器。
  该设备能够将存储在其存储器
  映射寄存器之一中的值转换为
  高电压和低电压序列。 */
  /* 接着我们就可以通过一些接口向终端
    发送一些数据了 */
	uart_init();
  /* 打印一些日志 */
	kinfo("[ChCore] uart init finished\n");

	kinfo("Address of main() is 0x%lx\n", main);
	kinfo("123456 decimal is 0%o octal\n", 123456);
	kinfo("%d\n", 123456);//ok
	kinfo("%o\n", 123);//ok
	kinfo("%d\n", -123);//ok
  /* lab1的测试函数 */
	stack_test(5);
  /* 最后就打印了一条日志，然后就返回了 */
	break_point();
	return;

	/* Should provide panic and use here */
	BUG("[FATAL] Should never be here!\n");
}
