
2. 入口是`_init()`

3. 
```sh
$ readelf -S build/kernel.img -W
There are 9 section headers, starting at offset 0x20cd8:

节头：
  [Nr] Name              Type            Address          Off    Size   ES Flg Lk Inf Al
  [ 0]                   NULL            0000000000000000 000000 000000 00      0   0  0
  [ 1] init              PROGBITS        0000000000080000 010000 00b5b0 08 WAX  0   0 4096
  [ 2] .text             PROGBITS        ffffff000008c000 01c000 0011dc 00  AX  0   0  8
  [ 3] .rodata           PROGBITS        ffffff0000090000 020000 0000f8 01 AMS  0   0  8
  [ 4] .bss              NOBITS          ffffff0000090100 0200f8 008000 00  WA  0   0 16
  [ 5] .comment          PROGBITS        0000000000000000 0200f8 000032 01  MS  0   0  1
  [ 6] .symtab           SYMTAB          0000000000000000 020130 000858 18      7  46  8
  [ 7] .strtab           STRTAB          0000000000000000 020988 00030f 00      0   0  1
  [ 8] .shstrtab         STRTAB          0000000000000000 020c97 00003c 00      0   0  1
Key to Flags:
  W (write), A (alloc), X (execute), M (merge), S (strings), I (info),
  L (link order), O (extra OS processing required), G (group), T (TLS),
  C (compressed), x (unknown), o (OS specific), E (exclude),
  p (processor specific)
```
4. 
```sh
objdump -h build/kernel.img

build/kernel.img：     文件格式 elf64-little

节：
Idx Name          Size      VMA               LMA               File off  Algn
  0 init          0000b5b0  0000000000080000  0000000000080000  00010000  2**12
                  CONTENTS, ALLOC, LOAD, CODE
  1 .text         000011dc  ffffff000008c000  000000000008c000  0001c000  2**3
                  CONTENTS, ALLOC, LOAD, READONLY, CODE
  2 .rodata       000000f8  ffffff0000090000  0000000000090000  00020000  2**3
                  CONTENTS, ALLOC, LOAD, READONLY, DATA
  3 .bss          00008000  ffffff0000090100  0000000000090100  000200f8  2**4
                  ALLOC
  4 .comment      00000032  0000000000000000  0000000000000000  000200f8  2**0
                  CONTENTS, READONLY
```
（Load Memory Address，LMA）:加载内存地址
（Virtual Memory Address，VMA）:虚拟内存地址

默认`LMA==VMA`
注意到`.text`,`.rodata`,`.bss`,`.comment`的VMA和LMA似乎有所不同，见`scripts/linker-aarch64.lds.in`,
将`.text`LMA修正到`init`段之后。
```txt
Why do that?

In most cases the two addresses will be the same. An
example of when they might be different is when a data section is
loaded into ROM, and then copied into RAM when the program starts up
(this technique is often used to initialize global variables in a ROM
based system). In this case the ROM address would be the LMA, and the
RAM address would be the VMA. "
```
bootloader开始运行时仍处于实模式，既不支持虚拟内存，也无法访问0xffffff00000级别的内存区域，寻址的时候用的是LMA。bootloader在运行过程中将切换到保护模式，并完成内核代码从低地址段到高地址段的映射。故进入内核后VMA变成了上图中的数值，寻址的时候也是用的VMA。


6. 
`x0-x7`:参数寄存器
`x9-x15`:临时寄存器，调用者保护（子例程调用前保存，调用后恢复）
`x19-x30`:被调用者保护（子例程开始保存，结束恢复）
`x29 FP`:栈底
`x30 LR`:返回地址
`SP`:栈顶
ARMV8:
在AArch64架构，通用寄存器X0-X30是64bit宽度 8B；
在AArch32架构，通用寄存器W0-W30是32bit宽度 4B；

```
(gdb) x/10a $sp
0xffffff00000a20f0 <kernel_stack+8128>: 0xffffff00000a2110 <kernel_stack+8160>  0xffffff000008c10c <main+128>
0xffffff00000a2100 <kernel_stack+8144>: 0xffffff00000a00b8      0xffffffc0
0xffffff00000a2110 <kernel_stack+8160>: 0x0     0xffffff000008c018
0xffffff00000a2120 <kernel_stack+8176>: 0x0     0x0
0xffffff00000a2130 <kernel_stack+8192>: 0x0     0x0
```

```
(gdb) disassable
   //sp.push(fp,lr) sp-=32 fp=0xffffff00000a00b8 ,lr=0xffffff000008c10c
   0xffffff000008c020 <+0>:     stp     x29, x30, [sp, #-32]!
   //x29 = sp
   0xffffff000008c024 <+4>:     mov     x29, sp
   // *(sp+16) = x19
   0xffffff000008c028 <+8>:     str     x19, [sp, #16]
   // x19=x0 x0是传入的参数
   0xffffff000008c02c <+12>:    mov     x19, x0
   // x1=x0
   0xffffff000008c030 <+16>:    mov     x1, x0
   // x0=0xffffff00000a0000
   0xffffff000008c034 <+20>:    adrp    x0, 0xffffff00000a0000
   //x0+=0
   0xffffff000008c038 <+24>:    add     x0, x0, #0x0
   //goto printk
=> 0xffffff000008c03c <+28>:    bl      0xffffff000008c978 <printk>
```

递归之后：栈里也就是保存了fp和lr和函数的参数?
```
(gdb) x/10a $sp
0xffffff00000a20b0 <kernel_stack+8064>: 0xffffff00000a20d0 <kernel_stack+8096>  0xffffff000008c070 <stack_test+80>
0xffffff00000a20c0 <kernel_stack+8080>: 0x4     0xffffffc0
0xffffff00000a20d0 <kernel_stack+8096>: 0xffffff00000a20f0 <kernel_stack+8128>  0xffffff000008c070 <stack_test+80>
0xffffff00000a20e0 <kernel_stack+8112>: 0x5     0xffffffc0
0xffffff00000a20f0 <kernel_stack+8128>: 0xffffff00000a2110 <kernel_stack+8160>  0xffffff000008c10c <main+128>
```


首先在`start.S`，主处理器使用初始化栈，
然后设置栈指针在内核栈栈顶（低地址），
然后跳转`init_c`，清空`bss`段，初始化
`uart`，`boot的页表`，使能`mmu`，然后跳转
`start_kernel`，设置了`TPIDR_EL1`寄存器，
设置内核栈，跳转到我们的`main`函数执行操
作系统的内容。
