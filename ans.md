
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