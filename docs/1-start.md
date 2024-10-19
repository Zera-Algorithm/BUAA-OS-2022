# 内核、Boot和printf

## 实验目的

1.  从操作系统角度理解MIPS体系结构

2.  掌握操作系统启动的基本流程

3.  掌握ELF文件的结构和功能

在本章中，需要阅读并填写部分代码，使得MOS操作系统可以正常的运行起来。这一章介绍实验的难度较为简单。

## 操作系统的启动

### 内核在哪里？

我们知道计算机是由硬件和软件组成的，仅有一个裸机是什么也干不了的；
另一方面，软件也必须运行在硬件之上才能实现其价值。由此可见，硬件和软件是相互依存、密不可分的。
为了能较好的管理计算机系统的硬件资源，我们需要使用操作系统。
那么在操作系统设计课程中，我们管理的硬件在哪里呢？通过前面的内容学习，可以知道GXemul是一款计算机架构仿真器，在本实验可以通过仿真器模拟我们需要的CPU等硬件环境。在编写操作系统之前，我们每个人面前都是一个裸机。对于操作系统课程设计，我们编写代码的平台是Linux系统，运行程序的仿真实验平台是GXemul。我们编写的所有的代码，在服务器提供的
Linux平台上通过 Makefile
交叉编译产生可执行文件，最后使用GXemul对可执行文件运行同学们开发的操作系统。

::: note
操作系统的启动英文称作"boot"。这个词是bootstrap的缩写，意思是鞋带（靴子上的那种）。
之所以将操作系统启动称为boot，源自于一个英文的成语"pull oneself up by
one's bootstraps"，
直译过来就是用自己的鞋带把自己提起来。操作系统启动的过程正是这样一个极度纠结的过程。
硬件是在软件的控制下执行的，而当刚上电的时候，存储设备上的软件又需要由硬件载入到内存中去执行。
可是没有软件的控制，谁来指挥硬件去载入软件？因此，就产生了一个类似于鸡生蛋，蛋生鸡一样的问题。
硬件需要软件控制，软件又依赖硬件载入。就好像要用自己的鞋带把自己提起来一样。
早期的工程师们在这一问题上消耗了大量的精力。所以，他们后来将"启动"这一纠结的过程称为"boot"。
:::

操作系统最重要的部分是操作系统内核，因为内核需要直接与硬件交互管理各个硬件，从而利用硬件的功能为用户进程提供服务。
启动操作系统，我们就需要将内核代码在计算机结构上运行起来，一个程序要能够运行，其代码必须能够被CPU直接访问，所以不能在磁盘上，因为CPU无法直接访问磁盘；
另一方面，内存RAM是易失性存储器，掉电后将丢失全部数据，所以不可能将内核代码保存在内存中。
所以直观上可以认识到：①磁盘不能直接访问②内存掉电易失，内核文件有可能放置的位置只能是CPU能够直接访问的非易失性存储器------ROM或FLASH中。

但是，把操作系统内核放置在这种非易失存储器上会有一些问题：

1.  这种CPU能直接访问的非易失性存储器的存储空间一般会映射到CPU寻址空间的某个区域，这个是在硬件设计上决定的。
    显然这个区域的大小是有限的。功能比较简单的操作系统还能够放在其中，对于内核文件较大的普通操作系统而言显然是不够的。

2.  如果操作系统内核在CPU加电后直接启动，意味着一个计算机硬件上只能启动一个操作系统，这样的限制显然不是我们所希望的。

3.  把特定硬件相关的代码全部放在操作系统中也不利于操作系统的移植工作。

基于上述考虑，设计人员一般都会将硬件初始化的相关工作放在名为"bootloader"的程序中来完成。这样做的好处正对应上述的问题：

1.  将硬件初始化的相关工作从操作系统中抽出放在bootloader中实现，意味着通过这种方式实现了硬件启动和软件启动的分离。
    因此需要存储在非易失性存储器中的硬件启动相关指令不需要很多，能够很容易地保存在ROM或FLASH中。

2.  bootloader在硬件初始化完后，需要为软件启动（即操作系统内核的功能）做相应的准备，
    比如需要将内核镜像文件从存放它的存储器（比如磁盘）中读到RAM中。既然bootloader需要将内核镜像文件加载到内存中，
    那么它就能选择使用哪一个内核镜像进行加载。使用bootloader后，我们就能够在一个硬件上运行多个操作系统了。

3.  bootloader主要负责硬件启动相关工作，同时操作系统内核则能够专注于软件启动以及对用户提供服务的工作，
    从而降低了硬件相关代码和软件相关代码的耦合度，有助于操作系统的移植。需要注意的是这样做并不意味着操作系统不依赖硬件。
    由于操作系统直接管理着计算机所有的硬件资源，要想操作系统完全独立于处理器架构和硬件平台显然是不切实际的。
    使用bootloader更清晰地划分了硬件启动和软件启动的边界，使操作系统与硬件交互的抽象层次提高了，从而简化了操作系统的开发和移植工作。

### Bootloader

从操作系统的角度看，boot loader 的总目标就是正确地调用内核来执行。
另外，由于 boot loader 的实现依赖于 CPU 的体系结构，因此大多数boot
loader 都分为 stage1 和 stage2两大部分。

在stage 1时，此时需要初始化硬件设备，包括watchdog
timer、中断、时钟、内存等。需要注意的一个细节是，此时内存RAM尚未初始化完成，因而stage
1直接运行在存放bootloader的存储设备上（比如FLASH）。由于当前阶段不能在内存RAM中运行，其自身运行会受诸多限制，比如有些flash程序不可写，即使程序可写的flash也有存储空间限制。这就是为什么需要stage
2的原因。 stage 1除了初始化基本的硬件设备以外，会为加载stage
2准备RAM空间，然后将stage
2的代码复制到RAM空间，并且设置堆栈，最后跳转到stage 2的入口函数。

stage
2运行在RAM中，此时有足够的运行环境从而可以用C语言来实现较为复杂的功能。
这一阶段的工作包括，初始化这一阶段需要使用的硬件设备以及其他功能，然后将内核镜像文件从存储器读到RAM中，并为内核设置启动参数，
最后将CPU指令寄存器的内容设置为内核入口函数的地址，即可将控制权从bootloader转交给操作系统内核。

从CPU上电到操作系统内核被加载的整个启动的步骤如图[1.1](#fig:bootloader-steps){reference-type="ref"
reference="fig:bootloader-steps"}所示。

![启动的基本步骤](bootloader-steps){#fig:bootloader-steps width="10cm"}

需要注意的是，以上bootloader的两个工作阶段只是从功能上论述内核加载的过程，在具体实现上不同的系统可能有所差别，而且对于不同的硬件环境也会有些不同。
在我们常见的x86
PC的启动过程中，首先执行的是BIOS中的代码，主要完成硬件初始化相关的工作，
然后BIOS会从MBR（master boot
record，开机硬盘的第一个扇区）中读取开机信息。在Linux中常说的Grub和Lilo这两种开机管理程序就是保存在MBR中。

::: note
GRUB(GRand Unified
Bootloader)是GNU项目的一个多操作系统启动程序。简单地说，就是可以用于在有多个操作系统的机器上，
在刚开机的时候选择一个操作系统进行引导。如果安装过Ubuntu一类的发行版的话，
一开机出现的那个选择系统用的菜单就是GRUB提供的。
:::

（这里以Grub为例）BIOS加载MBR中的Grub代码后就把CPU交给了Grub，Grub的工作就是一步一步的加载自身代码，从而识别文件系统，
然后就能够将文件系统中的内核镜像文件加载到内存中，并将CPU控制权转交给操作系统内核。
这样看来，其实BIOS和Grub的前一部分构成了前述stage 1的工作，而stage
2的工作则是完全在Grub中完成的。

::: note
bootloader有两种操作模式：启动加载模式和下载模式。对于普通用户而言，bootloader只运行在启动加载模式，就是我们之前讲解的过程。
而下载模式仅仅对于开发人员有意义，区别是前者是通过本地设备中的内核镜像文件启动操作系统的，而后者是通过串口或以太网等通信手段将远端的内核镜像上载到内存的。
:::

### gxemul中的启动流程

从前面的分析，我们可以看到，操作系统的启动是一个非常复杂的过程。不过，幸运的是，由于MOS操作系统的目标是在gxemul仿真器上运行，
这个过程被大大简化了。gxemul仿真器支持直接加载elf格式的内核，也就是说，gxemul已经提供了bootloader全部功能。
MOS操作系统不需要再实现bootloader的功能了。换句话说，你可以假定，从MOS操作系统的运行第一行代码前，
我们就已经拥有一个正常的运行环境。

::: note
如果你以前对于操作系统的概念仅仅停留在很表面的层次上，那么这里你也许会有所疑惑，为什么这里要说"正常的运行环境"？
难道还能有"不正常的运行环境"？我们来举一个例子说明一下：假定我们刚加电，CPU开始从ROM上取指。
为了简化，我们假定这台机器上没有BIOS(Basic Input Output
System)，bootloader被直接烧在了ROM中（很多嵌入式环境就是这样做的）。
这时，由于内存没有被初始化，整个bootloader程序尚处于ROM中。程序中的变量也仍被储存在ROM上。
而ROM是只读的，所以任何对于变量的赋值操作都是不被允许的。
而当内存被初始化，bootloader将后续代码载入到内存中后，位于内存中的代码便能完整地使用的各类功能。
:::

gxemul支持加载elf格式内核，所以启动流程被简化为加载内核到内存，之后跳转到内核的入口，启动完毕。整个启动过程非常简单。
这里要注意，之所以简单还有一个原因就在于gxemul本身是仿真器，是一种软件而不是真正的硬件，
所以就不需要面对传统的bootloader碰到的那种复杂情况。

## Let's hack the kernel!

接下来，就要开始来生成MOS操作系统内核了。这一节中，我们将介绍如何修改内核并实现一些自定义的功能。

### Makefile------内核代码的地图

当我们使用ls命令看看都有哪些实验代码时，会发现似乎文件目录有点多，各个不同的文件夹名称大致说明了各自的功能，但是浏览每个文件还是有点难度。

不过我们看见有一个文件非常熟悉，叫做Makefile。

相信大家在lab0中，已经对Makefile有了初步的了解，这个Makefile文件即为构建整个操作系统所用的顶层Makefile。我们可以通过浏览这个文件来对整个操作系统的代码布局有个初步的了解：
可以说，Makefile就像源代码的地图，能告诉我们源代码是如何一步一步成为最终的可执行文件。代码[\[code:top-makefile\]](#code:top-makefile){reference-type="ref"
reference="code:top-makefile"}是实验代码最顶层的Makefile，
通过阅读它我们就能了解代码中很多宏观的东西。
(为了方便理解，加入了部分注释)

::: codeBoxWithCaption
顶层Makefile[]{#code:top-makefile label="code:top-makefile"}

``` {.make linenos=""}
# Main makefile
#
# Copyright (C) 2007 Beihang University
# Written by Zhu Like ( zlike@cse.buaa.edu.cn )
#

drivers_dir	  := drivers
boot_dir	  := boot
init_dir	  := init
lib_dir		  := lib
tools_dir	  := tools
test_dir	  := 
#上面定义了各种文件夹名称
vmlinux_elf	  := gxemul/vmlinux
#这个是我们最终需要生成的elf文件
link_script   := $(tools_dir)/scse0_3.lds #链接用的脚本

modules		  := boot drivers init lib
objects		  := $(boot_dir)/start.o			  \
				 $(init_dir)/main.o			  \
				 $(init_dir)/init.o			  \
			   	 $(drivers_dir)/gxconsole/console.o \
				 $(lib_dir)/*.o				  \
#定义了需要生成的各种文件

.PHONY: all $(modules) clean

all: $(modules) vmlinux #我们的“最终目标”

vmlinux: $(modules)
	$(LD) -o $(vmlinux_elf) -N -T $(link_script) $(objects)

##注意,这里调用了一个叫做$(LD)的程序

$(modules): 
	$(MAKE) --directory=$@

#进入各个子文件夹进行make

clean: 
	for d in $(modules);	\
		do					\
			$(MAKE) --directory=$$d clean; \
		done; \
	rm -rf *.o *~ $(vmlinux_elf)

include include.mk
```
:::

如果你以前没有接触过Makefile的话，仅仅凭借lab0的简单练习获得的知识，可能还不足以顺畅的阅读这份47行的Makefile。不必着急，我们来一行一行地解读它。
前6行是注释，不必介绍。接下来很开心的看到了我们熟悉的赋值符号。没错，这是Makefile中对变量的定义语句。7～23行定义了一些变量，包括各个子目录的相对路径，最终的可执行文件的路径(vmlinux_elf)，
linker
script的位置(link_script)。其中，最值得注意的两个变量分别是modules和objects。modules定义了内核所包含的所有模块，objects则表示要编译出内核所依赖的所有.o文件。
19到23行行末的斜杠代表这一行没有结束，下一行的内容和这一行是连在一起的。这种写法一般用于提高文件的可读性。可以把本该写在同一行的东西分布在多行中，使得文件更容易被人类阅读。

::: note
linker
script是用于指导链接器将多个.o文件链接成目标可执行文件的脚本。.o文件、linker
script等内容会在下面的小节中细致地讲解，大家这里只要知道这些文件是编译内核所必要的就好。
:::

26行的.PHONY表明列在其后的目标不受修改时间的约束。也就是说，一旦该规则被调用，无视make工具编译时有关时间戳的性质，无论依赖文件是否被修改，一定保证它被执行。

第28行定义all这一规则的依赖。all代表整个项目，由此可以知道，构建整个项目依赖于构建好所有的模块以及vmlinux。那么vmlinux是如何被构建的呢？
紧接着的30行定义了，vmlinux的构建依赖于所有的模块。在构建完所有模块后，将执行第31行的指令来产生vmlinux。
我们可以看到，第31行调用了链接器将之前构建各模块产生的所有.o文件在linker
script的指导下链接到一起，产生最终的vmlinux可执行文件。
第35行定义了每个模块的构建方法为调用对应模块目录下的Makefile。最后的40到45行定义了如何清理所有被构建出来的文件。

::: note
一般在写Makefile时，习惯将第一个规则命名为all，也就是构建整个项目的意思。如果调用make时没有指定目标，make会自动执行第一个目标，所以把all放在第一个目标的位置上，可以使得make命令默认构建整个项目。
:::

读到这里，有一点需要注意，在编译指令中使用了LD、MAKE等变量，但是我们似乎从来没有定义过这个变量。
那么这个变量定义在哪呢？

紧接着我们看到了第47行有一条include命令。看来，这个顶层的Makefile还引用了其他的东西。让我们来看看这个文件，被引用的文件如代码[\[code:include-mk\]](#code:include-mk){reference-type="ref"
reference="code:include-mk"}所示。

::: codeBoxWithCaption
include.mk[]{#code:include-mk label="code:include-mk"}

``` {.make linenos=""}
# Common includes in Makefile
#
# Copyright (C) 2007 Beihang University
# Written By Zhu Like ( zlike@cse.buaa.edu.cn )


CROSS_COMPILE := bin/mips_4KC-
CC	:= $(CROSS_COMPILE)gcc
CFLAGS	:= -O -G 0 -mno-abicalls -fno-builtin -Wa,-xgot -Wall -fPIC -Werror
LD	:= $(CROSS_COMPILE)ld
```
:::

原来LD变量是在这里被定义的（变量MAKE是Makefile中的自带变量，不是我们定义的）。
在该文件中，看到了一个非常熟悉的关键词------Cross
Compile(交叉编译)，也就是变量CC。
不难看出，这里的CROSS_COMPILE变量是在定义编译和链接等指令的前缀，或者说是交叉编译器的具体位置。
例如，在我们的实验以及测评环境中，LD最终调用的指令是"/OSLAB/compiler/usr/bin/mips_4KC-ld"。
通过修改该变量，就可以方便地设定交叉编译工具链的位置。

::: exercise
[]{#exercise1-include.mk label="exercise1-include.mk"}
请修改include.mk文件，使交叉编译器的路径正确。之后执行make指令，如果配置一切正确，则会在gxemul目录下生成vmlinux的内核文件。
:::

::: note
可以自己尝试去目录/OSLAB/compiler/usr/bin/看一眼，确信交叉编译路径不是我们随口说的，而是确确实实在那里。
:::

至此，我们就可以大致掌握阅读Makefile的方法了。善于运用make的功能可以给你带来很多惊喜哦。提示：可以试着使用一下make
clean。
如果你觉得每次用gxemul运行内核都需要打很长的指令这件事很麻烦，那么可以尝试在Makefile中添加运行内核这一功能，使得通过make就能自动运行内核。关于Makefile还有许多有趣的知识，同学们可以自行了解学习。

最后，简要总结一下实验代码中其他目录的组织以及其中的重要文件：

-   tool目录下只有 scse0_3.lds 这个 linker script
    文件，我们会在下面的小节中详细讲解。

-   gxemul 目录存放着 gxemul
    仿真器启动内核文件时所需要的配置文件，另外代码编译完成后生成的名为"vmlinux"的内核文件也会放在这个目录下。

-   boot 目录中需要注意的是 start.S 文件。这个文件中的 \_start
    函数是CPU控制权被转交给内核后执行的第一个函数，
    主要工作是初始化硬件相关的设置，为之后操作系统初始化做准备，最后跳转到init/main.c文件中定义的
    main 函数。

-   init 目录中主要有两个代码文件 main.c 和
    init.c，其作用是初始化操作系统。
    通过分析函数调用关系，我们可以发现最终调用的函数是 init.c 文件中的
    mips_init() 函数。
    在本实验中此函数只是简单的打印输出，而在之后的实验中会逐步添加新的内核功能，所以诸如内存初始化等具体方面的初始化函数都会在这里被调用。

-   include 目录中存放系统头文件。在本实验中需要用到的头文件是 mmu.h
    文件， 这个文件中有一张内存布局图，我们在填写 linker script
    的时候需要根据这个图来设置相应段的加载地址。

-   lib 目录存放一些常用库函数，这里主要存放一些打印的函数。

-   driver
    目录中存放驱动相关的代码，这里主要存放的是终端输出相关的函数。

### ELF------深入探究编译与链接

如果你已经尝试过运行内核，那么你会发现它现在是根本运行不起来的。因为我们还有一些重要的步骤没有做。但是在做这些之前，我们不得不补充一些重要的，
但又有些琐碎的知识。在这里，我们需要知道可执行文件的格式，进一步去了解一段代码是如何从编译一步一步变成一个可执行文件以及可执行文件又是如何被执行的。

请注意在本章中阐述的内容一部分关于Linux实验环境，即在Linux环境下如何模拟我们编写的操作系统，另一部分则是关于我们将要编写的操作系统，在看琐碎的知识点时，请务必注意我们在讨论哪一部分，本章很多概念的混淆或模糊都是由于同学们没有区分开这两部分内容。

::: codeBoxWithCaption
一个简单的C程序[]{#code:hello-c label="code:hello-c"}

``` {.c linenos=""}
#include <stdio.h>

int main()
{
    printf("Hello World!\n");
    return 0;
}
```
:::

以代码[\[code:hello-c\]](#code:hello-c){reference-type="ref"
reference="code:hello-c"}为例，讲述我们这个冗长的故事。首先探究这样一个问题：**含有多个C文件的工程是如何编译成一个可执行文件的？**

这段代码相信你非常熟悉了，不知你有没有注意到过这样一个小细节：printf的定义在哪里？
[^1]
我们都学过，C语言中函数必须有定义才能被调用，那么printf的定义在哪里呢？你一定会笑一笑说，别傻了，不就在stdio.h中吗？我们在程序开头通过include引用了它。
然而事实真的是这样吗？我们来进去看一看stdio.h里到底有些什么。

::: codeBoxWithCaption
stdio.h中关于printf的内容[]{#code:part-stdio-h
label="code:part-stdio-h"}

``` {.c linenos=""}
/*
 *	ISO C99 Standard: 7.19 Input/output	<stdio.h>
 */

/* Write formatted output to stdout.

   This function is a possible cancellation point and therefore not
   marked with __THROW.  */
extern int printf (const char *__restrict __format, ...);
```
:::

在代码[\[code:part-stdio-h\]](#code:part-stdio-h){reference-type="ref"
reference="code:part-stdio-h"}中，展示了从当前系统的stdio.h中摘录出的与printf相关的部分。可以看到，我们所引用的stdio.h中只有声明，但并没有printf的定义。
或者说，并没有printf的具体实现。可没有具体的实现，我们究竟是如何调用printf的呢？我们怎么能够调用一个没有实现的函数呢？

我们来一步一步探究，printf的实现究竟被放在了哪里，又究竟是在何时被插入到我们的程序中的。首先，要求编译器**只进行预处理（通过-E选项）**，而不编译。

``` {.c linenos=""}
/* 由于原输出太长，这里只能留下很少很少的一部分。 */
typedef unsigned char __u_char;
typedef unsigned short int __u_short;
typedef unsigned int __u_int;
typedef unsigned long int __u_long;


typedef signed char __int8_t;
typedef unsigned char __uint8_t;
typedef signed short int __int16_t;
typedef unsigned short int __uint16_t;
typedef signed int __int32_t;
typedef unsigned int __uint32_t;

typedef signed long int __int64_t;
typedef unsigned long int __uint64_t;

extern struct _IO_FILE *stdin;
extern struct _IO_FILE *stdout;
extern struct _IO_FILE *stderr;

extern int printf (const char *__restrict __format, ...);

int main()
{
    printf("Hello World!\n");
    return 0;
}
```

可以看到，C语言的预处理器将头文件的内容添加到了源文件中，但同时也能看到，这里一阶段并没有printf这一函数的定义。

之后，我们将gcc的-E选项换为-c选项，**只编译而不链接**，产生一个.o目标文件。
我们对其进行反汇编[^2]，结果如下

``` {.objdump linenos=""}
hello.o:     file format elf64-x86-64

Disassembly of section .text:

0000000000000000 <main>:
   0:   55                      push   %rbp
   1:   48 89 e5                mov    %rsp,%rbp
   4:   bf 00 00 00 00          mov    $0x0,%edi
   9:   e8 00 00 00 00          callq  e <main+0xe>
   e:   b8 00 00 00 00          mov    $0x0,%eax
  13:   5d                      pop    %rbp
  14:   c3                      retq
```

我们只需要注意中间那句callq即可，这一句是调用函数的指令。对照左侧的机器码，其中e8是call指令的操作码。根据MIPS指令的特点，
e8后面应该跟的是printf的地址。可在这里我们却发现，**本该填写printf地址的位置上被填写了一串0**。那个地址显然不可能是printf的地址。也就是说，直到这一步，
printf的具体实现依然不在我们的程序中。

最后，允许gcc进行链接，也就是**正常地编译**出可执行文件。然后，再用objdump进行反汇编。

``` {.objdump linenos=""}
hello:     file format elf64-x86-64


Disassembly of section .init:

00000000004003a8 <_init>:
  4003a8:       48 83 ec 08             sub    $0x8,%rsp
  4003ac:       48 8b 05 0d 05 20 00    mov    0x20050d(%rip),%rax
  4003b3:       48 85 c0                test   %rax,%rax
  4003b6:       74 05                   je     4003bd <_init+0x15>
  4003b8:       e8 43 00 00 00          callq  400400 <__gmon_start__@plt>
  4003bd:       48 83 c4 08             add    $0x8,%rsp
  4003c1:       c3                      retq   

Disassembly of section .plt:

00000000004003d0 <puts@plt-0x10>:
  4003d0:       ff 35 fa 04 20 00       pushq  0x2004fa(%rip)
  4003d6:       ff 25 fc 04 20 00       jmpq   *0x2004fc(%rip)
  4003dc:       0f 1f 40 00             nopl   0x0(%rax)

00000000004003e0 <puts@plt>:
  4003e0:       ff 25 fa 04 20 00       jmpq   *0x2004fa(%rip)
  4003e6:       68 00 00 00 00          pushq  $0x0
  4003eb:       e9 e0 ff ff ff          jmpq   4003d0 <_init+0x28>

00000000004003f0 <__libc_start_main@plt>:
  4003f0:       ff 25 f2 04 20 00       jmpq   *0x2004f2(%rip)
  4003f6:       68 01 00 00 00          pushq  $0x1
  4003fb:       e9 d0 ff ff ff          jmpq   4003d0 <_init+0x28>

0000000000400400 <__gmon_start__@plt>:
  400400:       ff 25 ea 04 20 00       jmpq   *0x2004ea(%rip)
  400406:       68 02 00 00 00          pushq  $0x2
  40040b:       e9 c0 ff ff ff          jmpq   4003d0 <_init+0x28>

Disassembly of section .text:

0000000000400410 <main>:
  400410:       48 83 ec 08             sub    $0x8,%rsp
  400414:       bf a4 05 40 00          mov    $0x4005a4,%edi
  400419:       e8 c2 ff ff ff          callq  4003e0 <puts@plt>
  40041e:       31 c0                   xor    %eax,%eax
  400420:       48 83 c4 08             add    $0x8,%rsp
  400424:       c3                      retq   

0000000000400425 <_start>:
  400425:       31 ed                   xor    %ebp,%ebp
  400427:       49 89 d1                mov    %rdx,%r9
  40042a:       5e                      pop    %rsi
  40042b:       48 89 e2                mov    %rsp,%rdx
  40042e:       48 83 e4 f0             and    $0xfffffffffffffff0,%rsp
  400432:       50                      push   %rax
  400433:       54                      push   %rsp
  400434:       49 c7 c0 90 05 40 00    mov    $0x400590,%r8
  40043b:       48 c7 c1 20 05 40 00    mov    $0x400520,%rcx
  400442:       48 c7 c7 10 04 40 00    mov    $0x400410,%rdi
  400449:       e8 a2 ff ff ff          callq  4003f0 <__libc_start_main@plt>
  40044e:       f4                      hlt    
  40044f:       90                      nop
```

篇幅所限，余下的部分没法再展示了（大约还有100来行）。

当你看到熟悉的"hello
world"被展开成如此"臃肿"的代码，可能不忍直视。但是别急，我们还是只把注意力放在主函数中，这一次，我们可以惊喜的看到，主函数里那一句callq后面已经不再是一串0了。
那里已经**被填入了一个地址**。从反汇编代码中我们也可以看到，这个地址就在这个可执行文件里，就在被标记为puts@plt的那个位置上。
虽然搞不清楚那个东西是什么，但显然那就是我们所调用的printf的具体实现了。

由此，我们不难推断，printf的实现是在**链接(Link)**这一步骤中被插入到最终的可执行文件中的。那么，了解这个细节究竟有什么用呢？
作为一个库函数，printf被大量的程序所使用。因此，每次都将其编译一遍实在太浪费时间了。printf的实现其实早就被编译成了二进制形式。
但此时，printf并未链接到程序中，它的状态与我们利用-c选项产生的hello.o相仿，都还处于未链接的状态。而在编译的最后，
链接器(Linker)会将所有的目标文件链接在一起，将之前未填写的地址等信息填上，形成最终的可执行文件，这就是链接的过程。

对于拥有多个c文件的工程来说，编译器会首先将所有的c文件以文件为单位，编译成.o文件。最后再将所有的.o文件以及函数库链接在一起，
形成最终的可执行文件。整个过程如图[1.2](#fig:link){reference-type="ref"
reference="fig:link"}所示。

![编译、链接的过程](link){#fig:link width="8cm"}

接下来，提出我们的下一个问题：**链接器通过哪些信息来链接多个目标文件呢？**答案就在于在目标文件（也就是我们通过-c选项生成的.o文件）。
在目标文件中，记录了代码各个段的具体信息。链接器通过这些信息来将目标文件链接到一起。而ELF(Executable
and Linkable Format)正是Unix上常用的一种目标文件格式。
其实，不仅仅是目标文件，可执行文件也是使用ELF格式记录的。这一点通过ELF的全称也可以看出来。

为了帮助你了解ELF文件，下一步需要进一步探究它的功能以及格式。

ELF文件是一种对可执行文件、目标文件和库使用的文件格式，跟Windows下的PE文件格式类似。ELF格式是UNIX系统实验室作为ABI(Application
Binary Interface)而开发和发布的，是SVR4 (UNIX System V Release
4.0)的标准可执行文件格式，现在已经是Linux支持的标准格式。我们在之前曾经看见过的.o文件就是ELF所包含的三种文件类型中的一种，称为可重定位(relocatable)文件，其它两种文件类型分别是可执行(executable)文件和共享对象(shared
object)文件，这两种文件都需要链接器对可重定位文件进行处理才能生成。

你可以使用file命令来获得文件的类型，如图[1.3](#fig:lab1-file){reference-type="ref"
reference="fig:lab1-file"}所示。

![file命令](lab1-file){#fig:lab1-file width="16cm" height="2cm"}

那么，ELF文件中都包含有什么东西呢？简而言之，就是和程序相关的所有必要信息，下图[1.4](#fig:lab1-elf-1){reference-type="ref"
reference="fig:lab1-elf-1"}说明了ELF文件的结构：

![ELF文件结构](lab1-elf-1){#fig:lab1-elf-1 width="8cm"}

通过上图可以知道，ELF文件从整体来说包含5个部分：

1.  ELF
    Header，包括程序的基本信息，比如体系结构和操作系统，同时也包含了Section
    Header Table和Program Header Table相对文件的偏移量(offset)。

2.  Program Header Table，也可以称为Segment
    Table，主要包含程序中各个Segment的信息，Segment的信息需要在运行时刻使用。

3.  Section Header
    Table，主要包含各个Section的信息，Section的信息需要在程序编译和链接的时候使用。

4.  Segments，就是各个Segment。Segment则记录了每一段数据（包括代码等内容）需要被载入到哪里，记录了用于指导应用程序加载的各类信息。

5.  Sections，就是各个Section。Section记录了程序的代码段、数据段等各个段的内容，主要是链接器在链接的过程中需要使用。

观察上图[1.4](#fig:lab1-elf-1){reference-type="ref"
reference="fig:lab1-elf-1"}我们可以发现，Program Header Table和Section
Header
Table指向了同样的地方，这就说明两者所代表的内容是重合的，这意味着什么呢？意味着两者只是同一个东西的不同视图！产生这种情况的原因在于ELF文件需要在两种场合使用：

1.  组成可重定位文件，参与可执行文件和可共享文件的链接。

2.  组成可执行文件或者可共享文件，在运行时为加载器提供信息。

我们已经了解了ELF文件的大体结构以及相应功能，现在，需要同学们自己动手，阅读一个简易的对32bitELF文件(little
endian)的解析程序，然后完成部分代码，来了解ELF文件各个部分的详细结构。

为了帮助大家进行理解，我们先对这个程序中涉及的三个关键数据结构做一下简要说明，请大家打开./readelf/kerelf.h这个文件，配合下方给出的注释和说明，仔细阅读其中的代码和注释，然后完成练习。
下面的代码都截取自./readelf/kerelf.h文件

``` {.c linenos=""}

/*文件的前面是各种变量类型定义，在此省略*/
/* The ELF file header.  This appears at the start of every ELF file.  */
/* ELF 文件的文件头。所有的ELF文件均以此为起始 */
#define EI_NIDENT (16)

typedef struct {
    unsigned char   e_ident[EI_NIDENT];     /* Magic number and other info */
    // 存放魔数以及其他信息
    Elf32_Half      e_type;                 /* Object file type */
    // 文件类型 
    Elf32_Half      e_machine;              /* Architecture */
    // 机器架构
    Elf32_Word      e_version;              /* Object file version */
    // 文件版本
    Elf32_Addr      e_entry;                /* Entry point virtual address */
    // 入口点的虚拟地址
    Elf32_Off       e_phoff;                /* Program header table file offset */
    // 程序头表所在处与此文件头的偏移
    Elf32_Off       e_shoff;                /* Section header table file offset */
    // 段头表所在处与此文件头的偏移
    Elf32_Word      e_flags;                /* Processor-specific flags */
    // 针对处理器的标记
    Elf32_Half      e_ehsize;               /* ELF header size in bytes */
    // ELF文件头的大小（单位为字节）
    Elf32_Half      e_phentsize;            /* Program header table entry size */
    // 程序头表入口大小
    Elf32_Half      e_phnum;                /* Program header table entry count */
    // 程序头表入口数
    Elf32_Half      e_shentsize;            /* Section header table entry size */
    // 段头表入口大小
    Elf32_Half      e_shnum;                /* Section header table entry count */
    // 段头表入口数
    Elf32_Half      e_shstrndx;             /* Section header string table index */
    // 段头字符串编号
} Elf32_Ehdr;

typedef struct
{
  // section name
  Elf32_Word sh_name;
  // section type
  Elf32_Word sh_type;
  // section flags
  Elf32_Word sh_flags;
  // section addr
  Elf32_Addr sh_addr;
  // offset from elf head of this entry
  Elf32_Off  sh_offset;
  // byte size of this section
  Elf32_Word sh_size;
  // link
  Elf32_Word sh_link;
  // extra info
  Elf32_Word sh_info;
  // alignment
  Elf32_Word sh_addralign;
  // entry size
  Elf32_Word sh_entsize;
}Elf32_Shdr;

typedef struct
{
  // segment type
  Elf32_Word p_type;
  // offset from elf file head of this entry
  Elf32_Off  p_offset;
  // virtual addr of this segment
  Elf32_Addr p_vaddr;
  // physical addr, in linux, this value is meanless and has same value of p_vaddr
  Elf32_Addr p_paddr;
  // file size of this segment
  Elf32_Word p_filesz;
  // memory size of this segment
  Elf32_Word p_memsz;
  // segment flag
  Elf32_Word p_flags;
  // alignment
  Elf32_Word p_align;
}Elf32_Phdr;
```

通过阅读代码可以发现，原来ELF的文件头，就是一个存了关于ELF文件信息的结构体。
首先，结构体中存储了ELF的魔数，以验证这是一个有效的ELF。
当我们验证了这是个ELF文件之后，便可以通过ELF头中提供的信息，进一步地解析ELF文件了。
在ELF头中，提供了段头表的入口偏移，这个是什么意思呢？
简单来说，假设binary为ELF的文件头地址，offset为入口偏移，那么binary+offset即为段头表第一项的地址。

::: exercise
[]{#exercise1-readelf label="exercise1-readelf"}
阅读./readelf文件夹中kerelf.h、readelf.c以及main.c三个文件中的代码，并完成readelf.c中缺少的代码，readelf函数需要输出elf文件的所有section
header的序号和地址信息，对每个section
header，输出格式为:\"%d:0x%x$\backslash$n\"，两个标识符分别代表序号和地址。
正确完成readelf.c代码之后，在readelf文件夹下执行make命令，即可生成可执行文件readelf，它接受文件名作为参数，对elf文件进行解析。
:::

::: note
请阅读Elf32_Ehdr和Elf32_Shdr这两个结构体的定义
关于如何取得后续的段头，可以采用代码中所提示的，先读取段头大小，随后每次累加。
也可以利用C语言中指针的性质，算出第一个段头的指针之后，当作数组进行访问。
:::

通过刚才的练习，相信你已经对ELF文件有了一个比较充分的了解，你可能想进一步解析ELF文件来获得更多的信息，不过这个工作Linux上已经有人做了。

同学可能发现，当我们完成exercise，生成可执行文件readelf并且想要运行时，如果不小心忘记打\"./\"，linux并没有给我们反馈"command
not found"，而是列出了一大堆help信息。
原来这是因为在Linux命令中，原本有就一个命令叫做readelf，它的使用格式是"readelf
\<option(s)\>
elf-file(s)"，用来显示一个或者多个elf格式的目标文件的信息。
我们可以通过它的选项来控制显示哪些信息。例如，执行readelf -S
testELF命令后，testELF文件中各个section的详细信息将以列表的形式展示出来。
可以利用readelf工具来验证我们自己写的简易版readelf输出的结果是否正确，还可以使用"readelf
--help"看到该命令各个选项及其对elf文件的解析方式，从而了解我们想要的信息。

::: thinking
[]{#think-endian label="think-endian"}
也许你会发现我们的readelf程序是不能解析之前生成的内核文件(内核文件是可执行文件)的，而我们刚才介绍的工具readelf则可以解析，这是为什么呢？(提示：尝试使用readelf
-h，观察不同)
:::

至此，lab1第一部分的内容已经完成。

现在，我们继续回到内核。

本实验最终生成的内核也是ELF格式，被模拟器载入到内核中。因此，我们暂且只关注ELF是如何被载入到内核中，并且被执行的，
而不再关心具体的链接细节。在阅读完上面编译和ELF的说明后，应该明确我们OS实验课如何编译链接产生实验操作系统的可执行文件，并且在gxemul中运行该ELF文件。ELF中有两个相似却不同的概念segment和section，我们不妨来看一下，之前那个hello
world程序的各个segment是什么样子。readelf工具可以方便地解析出elf文件的内容，这里使用它来分析我们的程序。
首先使用-l参数来查看各个segment的信息。

``` {.objdump linenos=""}
Elf 文件类型为 EXEC (可执行文件)
入口点 0x400e6e
共有 5 个程序头，开始于偏移量64

程序头：
  Type           Offset             VirtAddr           PhysAddr
                 FileSiz            MemSiz              Flags  Align
  LOAD           0x0000000000000000 0x0000000000400000 0x0000000000400000
                 0x00000000000b33c0 0x00000000000b33c0  R E    200000
  LOAD           0x00000000000b4000 0x00000000006b4000 0x00000000006b4000
                 0x0000000000001cd0 0x0000000000003f48  RW     200000
  NOTE           0x0000000000000158 0x0000000000400158 0x0000000000400158
                 0x0000000000000044 0x0000000000000044  R      4
  TLS            0x00000000000b4000 0x00000000006b4000 0x00000000006b4000
                 0x0000000000000020 0x0000000000000050  R      8
  GNU_STACK      0x0000000000000000 0x0000000000000000 0x0000000000000000
                 0x0000000000000000 0x0000000000000000  RW     10

 Section to Segment mapping:
  段节...
   00     .note.ABI-tag .note.gnu.build-id .rela.plt .init .plt .text 
   __libc_freeres_fn __libc_thread_freeres_fn .fini .rodata __libc_subfreeres 
   __libc_atexit __libc_thread_subfreeres .eh_frame .gcc_except_table 
   01     .tdata .init_array .fini_array .jcr .data.rel.ro .got .got.plt .data 
   .bss __libc_freeres_ptrs 
   02     .note.ABI-tag .note.gnu.build-id 
   03     .tdata .tbss 
   04
```

这些输出中，我们只需要关注这样几个部分：Offset代表该段(segment)的数据相对于ELF文件的偏移。VirtAddr代表该段最终需要被加载到内存的哪个位置。
FileSiz代表该段的数据在文件中的长度。MemSiz代表该段的数据在内存中所应当占的大小。

::: note
[]{#lab1-note-MemSiz label="lab1-note-MemSiz"}
MemSiz永远大于等于FileSiz。若MemSiz大于FileSiz，则操作系统在加载程序的时候，会首先将文件中记录的数据加载到对应的VirtAddr处。
之后，向内存中填0,直到该段在内存中的大小达到MemSiz为止。那么为什么MemSiz有时候会大于FileSiz呢？这里举这样一个例子：
C语言中未初始化的全局变量，我们需要为其分配内存，但它又不需要被初始化成特定数据。因此，在可执行文件中也只记录它需要占用内存(MemSiz)，
但在文件中却没有相应的数据（因为它并不需要初始化成特定数据）。故而在这种情况下，MemSiz会大于FileSiz。这也解释了，
为什么C语言中全局变量会有默认值0。这是因为操作系统在加载时将所有未初始化的全局变量所占的内存统一填了0。
:::

VirtAddr是尤为需要注意的。由于它的存在，就不难推测，Gxemul仿真器在加载我们的内核时，是按照内核这一可执行文件中所记录的地址，
将我们内核中的代码、数据等加载到相应位置。并将CPU的控制权交给内核。内核之所以不能够正常运行，显然是因为内核所处的地址是不正确的。
换句话说，**只要我们能够将内核加载到正确的位置上，内核就应该可以运行起来。**

思考到这里，我们又发现了几个重要的问题。

1.  当我们讲加载操作系统内核到正确的Gxemul模拟物理地址时，讨论的是Linux实验环境呢？还是我们编写的操作系统本身呢？

2.  什么叫做正确的位置？到底放在哪里才叫正确。

3.  哪个段被加载到哪里是记录在编译器编译出来的ELF文件里的，怎么才能修改它呢？

在接下来的小节中，我们将一点一点解决掉这三个问题。

### MIPS内存布局------寻找内核的正确位置

在这一节中，来解决关于内核应该被放在何处的问题。这里先简单介绍几个基本概念。在CPU的设计中，通常会有两种状态：用户态（user
mode）和内核态（kernel
mode）。用户态指非特权状态，在此状态下，执行的代码被硬件限定，不能进行某些操作，以防止带来安全隐患。内核态指特权状态，可以执行一些特权操作。MMU（Memory
Management
Unit）是内存管理单元，主要完成虚拟地址到物理地址的转换、内存保护等功能。同学们可以参考操作系统原理教材，了解详细的讲解。

在32位的MIPS
CPU中，程序地址空间会被划分为4个大区域。如图[1.5](#fig:memory-region){reference-type="ref"
reference="fig:memory-region"}所示。

![MIPS内存布局](memory-region){#fig:memory-region height="8cm"}

从硬件角度讲，这四个区域的情况如下：

1.  User Space(kuseg)
    0x00000000-0x7FFFFFFF(2G)：这些是用户态下可用的地址。在使用这些地址的时候，程序会通过MMU映射到实际的物理地址上。

2.  Kernel Space Unmapped Cached(kseg0)
    0x80000000-0x9FFFFFFF(512MB)：只需要将地址的高3
    位清零，这些地址就被转换为物理地址。
    也就是说，逻辑地址到物理地址的映射关系是硬件直接确定，不通过MMU。因为转换很简单，我们常常把这些地址称为"无需转换的"。
    一般情况下，都是通过cache 对这段区域的地址进行访问。

3.  Kernel Space Unmapped Uncached(kseg1)
    0xA0000000-0xBFFFFFFF(512MB)：这一段地址的转换规则与前者相似，
    通过将高3
    位清零的方法将地址映射为相应的物理地址，重复映射到了低端512MB
    的物理地址。需要注意的是，kseg1 不通过cache 进行存取。

4.  Kernel Space Mapped Cached(kseg2)
    0xC0000000-0xFFFFFFFF(1GB)：这段地址只能在内核态下使用并且需要MMU
    的转换。

看到这里，你也许还是很迷茫：完全不知道该把内核放在哪里呀！这里，我们再提供一个提示：需要通过MMU映射访问的地址得在建立虚拟内存机制后才能正常使用，
是由操作系统所管理的。因此，在载入内核时，只能选用不需要通过MMU的内存空间，因为此时尚未建立虚存机制。好了，满足这一条件的只有kseg0和kseg1了。
而kseg1是不经过cache的，一般用于访问外部设备。所以，我们的内核只能放在kseg0了。

那么具体放在哪里呢？这时，就需要仔细阅读代码了。在include/mmu.h里有我们的MOS操作系统内核完整的内存布局图（代码[\[code:memory-mmu-h\]](#code:memory-mmu-h){reference-type="ref"
reference="code:memory-mmu-h"}所示），
在之后的实验中，善用它可以带来意料之外的惊喜。

(Tip:在操作系统实验中，填空的代码往往只是核心部分，而对于实验的理解而言需要阅读的代码有很多，而且在阅读的过程中，希望能够梳理出脉络和体系，比如某些代码完成了什么功能，代码间依赖关系是什么？OS课程需要填空的代码并不多，希望本课程结束之后，同学们具有读懂一段较长的工程代码的能力。而每周的课上测试将建立在理解代码的基础上。)

::: codeBoxWithCaption
include/mmu.h中的内存布局图[]{#code:memory-mmu-h
label="code:memory-mmu-h"}

``` {.c linenos=""}
/*
 o     4G ----------->  +----------------------------+------------0x100000000
 o                      |       ...                  |  kseg3
 o                      +----------------------------+------------0xe000 0000
 o                      |       ...                  |  kseg2
 o                      +----------------------------+------------0xc000 0000
 o                      |   Interrupts & Exception   |  kseg1
 o                      +----------------------------+------------0xa000 0000
 o                      |      Invalid memory        |   /|\
 o                      +----------------------------+----|-------Physics Memory Max
 o                      |       ...                  |  kseg0
 o  VPT,KSTACKTOP-----> +----------------------------+----|-------0x8040 0000-------end
 o                      |       Kernel Stack         |    | KSTKSIZE            /|\
 o                      +----------------------------+----|------                |
 o                      |       Kernel Text          |    |                    PDMAP
 o      KERNBASE -----> +----------------------------+----|-------0x8001 0000    |
 o                      |   Interrupts & Exception   |   \|/                    \|/
 o      ULIM     -----> +----------------------------+------------0x8000 0000-------
 o                      |         User VPT           |     PDMAP                /|\
 o      UVPT     -----> +----------------------------+------------0x7fc0 0000    |
 o                      |         PAGES              |     PDMAP                 |
 o      UPAGES   -----> +----------------------------+------------0x7f80 0000    |
 o                      |         ENVS               |     PDMAP                 |
 o  UTOP,UENVS   -----> +----------------------------+------------0x7f40 0000    |
 o  UXSTACKTOP -/       |     user exception stack   |     BY2PG                 |
 o                      +----------------------------+------------0x7f3f f000    |
 o                      |       Invalid memory       |     BY2PG                 |
 o      USTACKTOP ----> +----------------------------+------------0x7f3f e000    |
 o                      |     normal user stack      |     BY2PG                 |
 o                      +----------------------------+------------0x7f3f d000    |
 a                      |                            |                           |
 a                      ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~                           |
 a                      .                            .                           |
 a                      .                            .                         kuseg
 a                      .                            .                           |
 a                      |~~~~~~~~~~~~~~~~~~~~~~~~~~~~|                           |
 a                      |                            |                           |
 o       UTEXT   -----> +----------------------------+                           |
 o                      |                            |     2 * PDMAP            \|/
 a     0 ------------>  +----------------------------+ -----------------------------
 o
*/
```
:::

相信大家已经发现了内核的正确位置了吧？

### Linker Script------控制加载地址

在发现了内核的正确位置后，只需要想办法让内核被加载到那里就OK了（把编写的操作系统放到模拟硬件的某个物理位置）。之前在分析ELF文件时我们曾看到过，编译器在生成ELF文件时就已经记录了各段所需要被加载到的位置。
同时，我们也发现，最终的可执行文件实际上是由链接器产生的（它将多个目标文件链接产生最终可执行文件）。因此，我们所需要做的，就是控制链接器的链接过程。

接下来，我们就要了解 Linker
Script。链接器的设计者们在设计链接器的时候面临这样一个问题：不同平台的ABI(Application
Binary Interface)都不一样，
怎样做才能增加链接器的通用性，使得它能为各个不同的平台生成可执行文件呢？于是，就有了Linker
Script。Linker Script中记录了各个section应该如何映射到segment，
以及各个segment应该被加载到的位置。下面的指令可以输出默认的链接脚本，你可以在自己的机器上尝试这一条指令：

``` {.bash linenos=""}
ld --verbose
```

这里，再补充一下关于ELF文件中section的概念。在链接过程中，目标文件被看成section的集合，并使用section
header table来描述各个section的组织。
换句话说，section记录了在链接过程中所需要的必要信息。其中最为重要的三个段为.text、.data、.bss。这三种段的意义是必须要掌握的：

.text

:   保存可执行文件的操作指令。

.data

:   保存已初始化的全局变量和静态变量。

.bss

:   保存未初始化的全局变量和静态变量。

以上的描述也许会显得比较抽象，这里我们来做一个实验。编写一个用于输出代码、全局已初始化变量和全局未初始化变量地址的代码（如代码[\[code:sections\]](#code:sections){reference-type="ref"
reference="code:sections"}所示）。
观察其运行结果与ELF文件中记录的.text、.data和.bss段相关信息之间的关系。

::: codeBoxWithCaption
用于输出各section地址的程序[]{#code:sections label="code:sections"}

``` {.c linenos=""}
#include <stdio.h>

char msg[]="Hello World!\n";
int count;

int main()
{
    printf("%X\n",msg);
    printf("%X\n",&count);
    printf("%X\n",main);

    return 0;
}
```
:::

该程序的一个可能的输出如下[^3]。

``` {.bash linenos=""}
user@debian ~/Desktop $ ./program 
80D4188
80D60A0
8048AAC
```

再看看ELF文件中记录的各section的相关信息（为了突出重点，这里只保留我们所关心的section）。

``` {.objdump linenos=""}
共有 29 个节头，从偏移量 0x9c258 开始：

节头：
  [Nr] Name              Type            Addr     Off    Size   ES Flg Lk Inf Al
  [ 4] .text             PROGBITS        08048140 000140 0620e4 00  AX  0   0 16
  [22] .data             PROGBITS        080d4180 08b180 000f20 00  WA  0   0 32
  [23] .bss              NOBITS          080d50c0 08c0a0 00136c 00  WA  0   0 64
```

对比二者，就可以清晰的知道**.text段包含了可执行文件中的代码**，**.data段包含了需要被初始化的全局变量和静态变量**，
而**.bss段包含了无需初始化的全局变量和静态变量**

接下来，通过Linker Script来尝试调整各段的位置。这里，我们选用GNU
LD官方帮助文档上的例子
(<https://www.sourceware.org/binutils/docs/ld/Simple-Example.html#Simple-Example>)，
该例子的完整代码如下所示：

``` {.objdump linenos=""}
SECTIONS
{
   . = 0x10000;
   .text : { *(.text) }
   . = 0x8000000;
   .data : { *(.data) }
   .bss : { *(.bss) }
}
```

在第三行的"."是一个特殊符号，用来做定位计数器，它根据输出段的大小增长。在SECTIONS命令开始的时候，它的值为0。通过设置"."即可设置接下来的section的起始地址。
"\*"是一个通配符，匹配所有的相应的段。例如".bss:{\*(.bss)}"表示将所有输入文件中的.bss段（右边的.bss）都放到输出的.bss段（左边的.bss）中。
为了能够编译通过（这个脚本过于简单，难以用于链接真正的程序），我们将原来的实验代码简化如下

``` {.c linenos=""}
char msg[]="Hello World!\n";
int count;

int main()
{
    return 0;
}
```

编译，并查看生产的可执行文件各section的信息。

``` {.objdump linenos=""}
user@debian ~/Desktop $ gcc -o test test.c -T test.lds -nostdlib -m32
user@debian ~/Desktop $ readelf -S test                              
共有 11 个节头，从偏移量 0x2164 开始：

节头：
  [Nr] Name              Type            Addr     Off    Size   ES Flg Lk Inf Al
  [ 2] .text             PROGBITS        00010024 001024 00000a 00  AX  0   0  1
  [ 5] .data             PROGBITS        08000000 002000 00000e 00  WA  0   0  1
  [ 6] .bss              NOBITS          08000010 00200e 000004 00  WA  0   0  4
```

可以看到，在使用了我们自定义的Linker
Script以后，生成的程序各个section的位置就被调整到了所指定的地址上。
segment是由section组合而成的，section的地址被调整了，那么最终segment的地址也会相应地被调整。
至此，我们就了解了如何通过lds文件控制各段被加载到的位置。

::: exercise
[]{#exercise1-scse0-3.lds label="exercise1-scse0-3.lds"}
填写tools/scse0_3.lds中空缺的部分，在lab1中，只需要填补.text、.data和.bss段将内核调整到正确的位置上即可。
:::

::: note
通过查看内存布局图，同学们应该能找到.text段的加载地址了，.data和.bss只需要紧随其后即可。同学们可以思考一下为什么要这么安排.data和.bss。注意lds文件编辑时"="两边的空格哦！
:::

再补充一点：关于链接后的程序从何处开始执行。程序执行的第一条指令称为entry
point， 我们在linker script中可以通过ENTRY(function
name)指令来设置程序入口。linker中程序入口的设置方法有以下五种：

1.  使用ld命令时，通过参数"-e"设置

2.  在linker scirpt中使用ENTRY()指令指定了程序入口

3.  如果定义start，则start就是程序入口

4.  .text段的第一个字节

5.  地址0处

阅读实验代码，想一想在我们的实验中，是采用何种方式指定程序入口的呢？

## MIPS汇编与C语言

在这一节中，将简单介绍MIPS汇编，以及常见的C语言语法与汇编的对应关系。在操作系统编程中，不可避免地要接触到汇编语言。
我们经常需要从C语言中调用一些汇编语言写成的函数，或者反过来，在汇编中跳转到C函数。为了能够实现这些，
需要了解C语言与汇编之间千丝万缕的联系。

下面以代码[\[code:c-example\]](#code:c-example){reference-type="ref"
reference="code:c-example"}为例，介绍典型的C语言中的语句对应的汇编代码。

::: codeBoxWithCaption
样例程序[]{#code:c-example label="code:c-example"}

``` {.c linenos=""}
int fib(int n)
{
    if (n == 0 || n == 1) {
        return 1;
    }
    return fib(n-1) + fib(n-2);
}

int main()
{
    int i;
    int sum = 0;
    for (i = 0; i < 10; ++i) {
        sum += fib(i);
    }

    return 0;
}
```
:::

### 循环与判断

这里你可能会问了，样例代码里只有循环啊！哪里有什么判断语句呀？事实上，由于MIPS汇编中没有循环这样的高级结构，
所有的循环均是采用判断加跳转语句实现的，所以我们将循环语句和判断语句合并在一起进行分析。
分析代码的第一步，就是要将循环等高级结构，用**判断加跳转**的方式替代。
例如，代码[\[code:c-example\]](#code:c-example){reference-type="ref"
reference="code:c-example"}第13-15行的循环语句，其最终的实现可能就如下面的C代码所展示的那样。

``` {.c linenos=""}
      i = 0;
      goto CHECK;
FOR:  sum += fib(i);
      ++i;
CHECK:if (i < 10) goto FOR;
```

将样例程序编译[^4]，
观察其反汇编代码。对照汇编代码和我们刚才所分析出来的C代码。
我们基本就能够看出来其间的对应关系。这里，将对应的C代码标记在反汇编代码右侧。

``` {.objdump linenos=""}
  400158:       sw      zero,16(s8)           #       sum = 0;
  40015c:       sw      zero,20(s8)           #       i = 0;
  400160:       j       400190 <main+0x48>    #       goto CHECK;
  400164:       nop                           # --------------------
  400168:       lw      a0,20(s8)             #  FOR:
  40016c:       jal     4000b0 <fib>          #
  400170:       nop                           #
  400174:       move    v1,v0                 #        sum += fib(i);
  400178:       lw      v0,16(s8)             #
  40017c:       addu    v0,v0,v1              #
  400180:       sw      v0,16(s8)             #
  400184:       lw      v0,20(s8)             # --------------------
  400188:       addiu   v0,v0,1               #        ++i;
  40018c:       sw      v0,20(s8)             # --------------------
  400190:       lw      v0,20(s8)             # CHECK:
  400194:       slti    v0,v0,10              #        if (i < 10)
  400198:       bnez    v0,400168 <main+0x20> #            goto FOR;
  40019c:       nop
```

再将右边的C代码对应会原来的C代码，就能够大致知道每一条汇编语句所对应的原始的C代码是什么了。
可以看出，判断和循环主要采用slt、slti判断两数间的大小关系，再结合b类型指令根据对应条件跳转。
以这些指令为突破口，我们就能大致识别出循环结构、分支结构了。

### 函数调用

这里需要分别分析函数的调用方和被调用方。我们选用样例程序中的fib这个函数来观察函数调用相关的内容。
这个函数是一个递归函数，因此，它函数调用过程的调用者，同时也是被调用者。可以从中观察到如何调用一个函数，
以及一个被调用的函数应当做些什么工作。

我们还是先将整个函数调用过程用高级语言来表示一下。

``` {.c linenos=""}
int fib(int n)
{
        if (n == 0) goto BRANCH;
        if (n != 1) goto BRANCH2;
BRANCH: v0 = 1;
        goto RETURN;
BRANCH2:v0 = fib(n-1) + fib(n-2);
RETURN: return v0;
}
```

然而，之后在分析汇编代码的时候，我们会发现有很多C语言中没有表示出来的东西。例如，在函数开头，有一大串的sw，
结尾处又有一大串的lw。这些东西究竟是在做些什么呢？

``` {.objdump linenos=""}
004000b0 <fib>:
  4000b0:       27bdffd8        addiu   sp,sp,-40
  4000b4:       afbf0020        sw      ra,32(sp)
  4000b8:       afbe001c        sw      s8,28(sp)
  4000bc:       afb00018        sw      s0,24(sp)
  # 中间暂且掠过，只关注一系列sw和lw操作。
  400130:       8fbf0020        lw      ra,32(sp)
  400134:       8fbe001c        lw      s8,28(sp)
  400138:       8fb00018        lw      s0,24(sp)
  40013c:       27bd0028        addiu   sp,sp,40
  400140:       03e00008        jr      ra
  400144:       00000000        nop
```

回忆一下C语言的递归。C语言递归的过程和栈这种数据结构有着惊人的相似性，函数递归到最底以及返回过程，就好像栈的后入先出。
而且每一次递归操作就仿佛将当前函数的所有变量和状态压入了一个栈中，待到返回时再从栈中弹出来，"一切"都保持原样。

回忆起了这个细节，我们再来看看汇编代码。在函数的开头，编译器添加了一组sw操作，
将所有当前函数所需要用到的寄存器原有的值全部保存到了内存中
[^5]。而在函数返回之前，编译器又加入了一组lw操作，将值被改变的寄存器全部恢复为原有的值。

我们惊奇地发现：编译器在函数调用的前后为我们添加了一组压栈(push)和弹栈(pop)的操作，保存了函数的当前状态。
函数的开始，编译器首先**减小sp指针的值，为栈分配空间**。并将需要保存的值放置在栈中。
当函数将要返回时，编译器再**增加sp指针的值，释放栈空间**。同时，恢复之前被保存的寄存器原有的值。
这就是为何C语言的函数调用和栈有着很大的相似性的原因：在函数调用过程中，编译器真的为我们维护了一个栈。
这下同学们应该也不难理解，为什么复杂函数在递归层数过多时会导致程序崩溃，也就是我们常说的"栈溢出"。

::: note
ra寄存器存放了函数的返回地址。使得被调用的函数结束时得以返回到调用者调用它的地方。我们其实可以将这个返回点设置为别的函数的入口，
使得该函数在返回时直接进入另一个函数中，而不是回到调用者哪里？一个函数调用了另一个函数，而返回时，返回到第三个函数中，
是不是也是一种很有价值的编程模型呢？如果你对此感兴趣，可以了解一下函数式编程中的Continuations的概念
(推荐[Functional Programming For The Rest Of
Us](https://github.com/justinyhuang/Functional-Programming-For-The-Rest-of-Us-Cn/blob/master/FunctionalProgrammingForTheRestOfUs.cn.md)这篇文章)，
在很多新近流行起来的语言中，都引入了类似的想法。
:::

在看到了一个函数作为被调用者做了哪些工作后，我们再来看看，作为函数的调用者需要做些什么？如何调用一个函数？如何传递参数？又如何获取返回值？
让我们来看一下，fib函数调用fib(n-1)和fib(n-2)时，编译器生成的汇编代码[^6]

``` {.gas linenos=""}
lw      $2,40($fp)      # v0 = n;
addiu   $2,$2,-1        # v0 = a0 - 1;
move    $4,$2           # a0 = v0;      // 即a0=n-1
jal     fib             # v0 = fib(a0);
nop                     #

move    $16,$2          # s0 = v0;
lw      $2,40($fp)      # v0 = n;
addiu   $2,$2,-2        # v0 = n - 2;
move    $4,$2           # a0 = v0;      // 即a0=n-2
jal     fib             # v0 = fib(a0);
nop                     #

addu    $16,$16,$2      # s0 += v0;
sw      $16,16($fp)     #
```

我们将汇编所对应的语义用C语言标明在右侧。可以看到，调用一个函数就是将参数存放在a0-a3寄存器中（暂且不关心参数非常多的函数会处理），
然后使用jal指令跳转到相应的函数中。函数的返回值会被保存在v0-v1寄存器中，我们通过这两个寄存器的值来获取返回值。

::: thinking
[]{#think-main label="think-main"}
内核入口在什么地方？main函数在什么地方？我们是怎么让内核进入到想要的main函数的呢？又是怎么进行跨文件调用函数的呢？
:::

如果仔细观察了上一个实验中的tools/scse0_3.lds这个文件，会发现在其中有ENTRY(\_start)这一行命令
旁边的注释写着，这是为了把入口定为_start这个函数。那么，这个函数是什么呢？

可以在实验代码的根目录运行一下命令"grep -r \_start
\*"进行文件内容的查找（这个命令在以后会很常用的，特别是想查找函数相
互调用关系的时候）。然后发现boot/start.S中似乎定义了我们的内核入口函数。在start.S中，前半部分前半段都是在初始化CPU。
后半段则是需要填补的部分。

::: note
在阅读boot/start.S汇编代码的时候会遇到LEAF、NESTED和END这三个宏。

LEAF用于定义那些不包含对其他函数调用的函数；NESTED用于定义那些需要调用其它函数的函数。
两者的区别在于对栈帧(frame)的处理，LEAF不需要过多处理堆栈指针。简言之，现阶段只需要知道这两个宏是在汇编代码中用来定义函数的。

END宏则仅仅是用来结束函数定义的。

阅读LEAF等宏定义的时候，我们会发现这些宏的定义是一系列以"."开头的指令。
这些指令不是MIPS汇编指令，而是 Assembler
directives(参考<https://sourceware.org/binutils/docs-2.34/as/Pseudo-Ops.html>)，
主要是在编译时用来指定目标代码的生成。
:::

::: exercise
[]{#exercise1-start.S label="exercise1-start.S"}
完成boot/start.S中空缺的部分。设置栈指针，跳转到main函数。 使用gxemul -E
testmips -C R3000 -M 64
elf-file运行(其中elf-file是你编译生成的vmlinux文件的路径)。
:::

在调用main函数之前，我们需要将sp寄存器设置到内核栈空间的位置上。具体的地址可以从mmu.h中看到。
这里做一个提醒，请注意栈的增长方向。设置完栈指针后，我们就具备了执行C语言代码的条件，因此，
接下来的工作就可以交给C代码来完成了。所以，在start.S的最后，调用C代码的主函数，
正式进入内核的C语言部分。

::: note
main函数虽然为c语言所书写，但是在被编译成汇编之后，其入口点会被翻译为一个标签，类似于：

main:

XXXXXX

想想看汇编程序中，如何调用函数？
:::

### 通用寄存器使用约定

为了和编译器等程序相互配合，需要遵循一些使用约定。这些规定是为了让不同的软件之间得以协同工作而制定的。MIPS中一共有32个通用寄存器(General
Purpose Registers)，
其用途如表[1.1](#tab:registers){reference-type="ref"
reference="tab:registers"}所示。

::: {#tab:registers}
   寄存器编号   助记符   用途
  ------------ --------- -------------------------------------------------------------------------------------------------------------------------------------------------------------
       0         zero    值总是为0
       1          at     （汇编暂存寄存器）一般由汇编器作为临时寄存器使用。
      2-3        v0-v1   用于存放表达式的值或函数的整形、指针类型返回值
      4-7        a0-a3   用于函数传参。其值在函数调用的过程中不会被保存。若函数参数较多，多出来的参数会采用栈进行传递
      8-15       t0-t7   用于存放表达式的值的临时寄存器;其值在函数调用的过程中不会被保存。
     16-23       s0-s7   保存寄存器;这些寄存器中的值在经过函数调用后不会被改变。
     24-25       t8-t9   用于存放表达式的值的临时寄存器;其值在函数调用的过程中不会被保存。当调用位置无关函数(position independent function)时， 25号寄存器必须存放被调用函数的地址。
     26-27      kt0-kt1  仅被操作系统使用。
       28         gp     全局指针和内容指针。
       29         sp     栈指针。
       30       fp或s8   保存寄存器（同s0-s7）。也可用作帧指针。
       31         ra     函数返回地址。

  : MIPS通用寄存器
:::

其中，只有16-23号寄存器和28-30号寄存器的值在函数调用的前后是不变的[^7]。
对于28号寄存器有一个特例：当调用位置无关代码(position independent
code)时，28号寄存器的值是不被保存的。

除了这些通用寄存器之外，还有一个特殊的寄存器：PC寄存器。这个寄存器中储存了当前要执行的指令的地址。
在Gxemul仿真器上调试内核时，可以留意一下这个寄存器。通过PC的值，就能够知道当前内核在执行的代码是哪一条，
或者触发中断的代码是哪一条等等。

## 实战printf

了解了这么多的内容后，我们来进行一番实战，在内核中实现一个printf函数。printf全都是由C语言的标准库提供的，而C语言的标准库是建立在操作系统基础之上的。所以，当开发操作系统的时候，就会发现，我们失去了C语言标准库的支持。
我们需要用到的几乎所有东西，都需要自己来实现。

要弄懂系统如何将一个字符输出到终端当中，需要阅读以下三个文件：lib/printf.c，lib/print.c和drivers/gxconsole/console.c。

首先大家先对这些代码的大致内容有个了解：

1.  drivers/gxconsole/console.c:这个文件负责往gxemul的控制台输出字符，其原理为读写某一个特殊的内存地址

2.  lib/printf.c:此文件中，实现了printf,但其所做的，实际上是把输出字符的函数，接受的输出参数给传递给了lp_Print这个函数

3.  lib/print.c:此文件中，实现了lp_Print函数，这个函数是printf函数的真正内核。

接着为了便于理解，我们一起来梳理一下这几个文件之间的关系。

lib/printf.c中定义了printf函数。但是，仔细观察就可以发现，这个printf函数实际上基本什么事情都没有做，只是把接受到的参数，以及myoutput()函数指针给传入lp_Print这个函数中。

``` {.c linenos=""}

void printf(char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    lp_Print(myoutput, 0, fmt, ap);
    va_end(ap);
}
```

同学们可能对va_list,va_start,va_end这三个语句以及函数参数中三个点比较陌生。想一想，我们使用printf时为什么可以有时只输出一个字符串，有时又可以一次输出好多变量呢？实际上，这是c语言函数可变长参数的功劳，va_list,va_start,va_end以及lib/print.c中的va_arg就是用于获取不定个数参数的宏。同学们可以参考这篇博客：https://www.cnblogs.com/qiwu1314/p/9844039.html来了解可变长参数的使用方法。

然后来看看myoutput这个函数，这个函数实际上是用来输出一个字符串的：

``` {.c linenos=""}

static void myoutput(void *arg, char *s, int l)
{
    int i;

    // special termination call
    if ((l==1) && (s[0] == '\0')) return;
    
    for (i=0; i< l; i++) {
    printcharc(s[i]);
    if (s[i] == '\n') printcharc('\n');
    }
}
```

可以发现，他实际上的核心是调用了一个叫做printcharc的函数。这个函数在哪儿呢？

我们可以在./drivers/gxconsole/gxconsole.c下面找到：

``` {.c linenos=""}

void printcharc(char ch)
{
    *((volatile unsigned char *) PUTCHAR_ADDRESS) = ch;
}
```

原来，想让控制台输出一个字符，实际上是对某一个内存地址写了一串数据。

看起来输出字符的函数一切正常，那为什么我们还不能使用printf呢？还记得printf函数实际上只是把相关信息传入到了lp_Print函数里面吗？而这个函数现在有一部分代码缺失了，需要你来帮忙补全。

为了方便大家理解这个比较复杂的函数，我们来给大家简单介绍一下。

首先，来看这个宏函数，这个函数先用宏定义来简化了myoutput这个函数指针的使用

``` {.c linenos=""}

#define     OUTPUT(arg, s, l)  \
  { if (((l) < 0) || ((l) > LP_MAX_BUF)) { \
       (*output)(arg, (char*)theFatalMsg, sizeof(theFatalMsg)-1); for(;;); \
    } else { \
      (*output)(arg, s, l); \
    } \
  }
```

观察lp_Print函数的参数，想想调用它时传入的参数，可以发现这个宏函数中的函数指针，就是之前提到过的myoutput函数，而这个宏定义的函数OUTPUT，其实就是简化了myoutput的调用过程。

然后，lp_Print中定义了一些需要使用到的变量。有几个变量需要重点了解其含义：

``` {.c linenos=""}
    int longFlag;//标记是否为long型
    int negFlag;//标记是否为负数
    int width;//标记输出宽度
    int prec;//标记小数精度
    int ladjust;//标记是否左对齐
    char padc;//填充多余位置所用的字符
```

接下来，我们发现整个的函数主体是一个没有终止条件的无限循环，这肯定是不对的，看来需要填补的地方就是这里了。这个循环中，主要有两个逻辑部分，第一部分：找到格式符%，并分析输出格式;第二部分，根据格式符分析结果进行输出。

什么叫找到格式并分析输出格式呢？想一想，我们在使用printf输出信息时，%ld，%-b，%c等等会被替换为相应输出格式的变量的值，他们就叫做格式符。在第一部分中，我们要干的就是解析fmt格式字符串，如果是不需要转换成变量的字符，那么就直接输出;如果碰到了格式字符串的结尾，就意味着本次输出结束，停止循环;但是如果碰到我们熟悉的%，那么就要按照printf的规格要求，开始解析格式符，分析输出格式，用上述变量记录下来这次对变量的输出要求，例如是要左对齐还是右对齐，是不是long型，是否有输出宽度要求等等，然后进入第二部分。

记录完输出格式，在第二部分中，需要做的就是按照格式输出相应的变量的值。这部分逻辑就比较简单了，先根据格式符进入相应的输出分支，然后取出变长参数中下一个参数，按照输出格式输出这个变量，输出完成后，又继续回到循环开头，重复第一部分，直到整个格式字符串被解析和输出完成。

到这里我们printf整个流程就梳理的差不多了，细节部分就靠同学们自己思考啦!

::: exercise
[]{#exercise1-print label="exercise1-print"}
阅读相关代码和下面对于函数规格的说明，补全lib/print.c中lp_Print()函数中两处缺失的部分来实现字符输出。
第一处缺失部分：找到%并分析输出格式;
第二处缺失部分：取出参数，输出格式串为\"%\[flags\]\[width\]\[.precision\]\[length\]d\"的情况。
:::

::: note
这个函数非常重要，希望大家即使评测得到满分，也要多加测试。否则可能出现一些诡异的情况下函数出错，造
成后续lab输出出错而导致评测结果错误，无法过关。
:::

下面是需要完成的printf函数的具体说明,同学们可以参考cppreference中有关C语言printf函数的文档[^8]或者标准
C++ 文档[^9]，对printf函数进行更加详细的了解。

函数原型：

``` {.c linenos=""}
void printf(const char* fmt, ...)
```

参数fmt是和C中printf类似的格式字符串，除了可以包含一般字符，还可以包含格式符(format
specifiers)，但略去并新添加了一些功能，格式符的原型为：

$$\%[flags][width][.precision][length]specifier$$

其中specifier指定了输出变量的类型，参见下表[1.2](#tab:specifiers){reference-type="ref"
reference="tab:specifiers"}：

::: {#tab:specifiers}
   Specifier              输出               例子   
  ----------- ---------------------------- -------- --
       b             无符号二进制数          110    
      d D               十进制数             920    
      o O            无符号八进制数          777    
      u U            无符号十进制数          920    
       x       无符号十六进制数，字母小写    1ab    
       X       无符号十六进制数，字母大写    1AB    
       c                  字符                a     
       s                 字符串             sample  

  : Specifiers说明
:::

除了specifier之外，格式符也可以包含一些其它可选的副格式符(sub-specifier),有flag(表[1.3](#tab:flag){reference-type="ref"
reference="tab:flag"})：

::: {#tab:flag}
   flag                        描述                        
  ------ ------------------------------------------------- --
    \-     在给定的宽度(width)上左对齐输出，默认为右对齐   
    0     当输出宽度和指定宽度不同的时候，在空白位置填充0  

  : flag说明
:::

和width(表[1.4](#tab:width){reference-type="ref"
reference="tab:width"})：

::: {#tab:width}
   width  描述
  ------- ----------------------------------------------------------------------------------------------------------------------------------------------
   数字   指定了要打印数字的最小宽度，当这个值大于要输出数字的宽度，则对多出的部分填充空格，但当这个值小于要输出数字的宽度的时候则不会对数字进行截断。

  : width说明
:::

以及precision(表[1.5](#tab:precision){reference-type="ref"
reference="tab:precision"})：

::: {#tab:precision}
   .precision  描述
  ------------ --------------------------------------------------------------------------------------------------------
     .数字     指定了精度，不同标识符下有不同的意义，但在我们实验的版本中这个值只进行计算而没有具体意义，所以不赘述。

  : precision说明
:::

另外，还可以使用length来修改数据类型的长度，在C中我们可以使用l、ll、h等，但这里我们只使用l，参看下表[1.6](#tab:length){reference-type="ref"
reference="tab:length"}

::: {#tab:length}
   length   Specifier  
  -------- ----------- -------------------
    2-3        d D        b o O u U x X
     l      long int    unsigned long int

  : length说明
:::

## 实验正确结果

如果你正确地实现了前面所要求的全部内容，你将在gxemul中观察到如下输出，这标志着你顺利完成了lab1实验。

``` {.text linenos=""}
GXemul 0.4.6    Copyright (C) 2003-2007  Anders Gavare
Read the source code and/or documentation for other Copyright messages.

Simple setup...
    net: simulating 10.0.0.0/8 (max outgoing: TCP=100, UDP=100)
        simulated gateway: 10.0.0.254 (60:50:40:30:20:10)
            using nameserver 202.112.128.51
    machine "default":
        memory: 64 MB
        cpu0: R3000 (I+D = 4+4 KB)
        machine: MIPS test machine
        loading gxemul/vmlinux
        starting cpu0 at 0x80010000
-------------------------------------------------------------------------------

main.c: main is start ...

init.c: mips_init() is called

panic at init.c:24: ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
```

## 如何退出gxemul

1\. 按Ctrl+C,以中断模拟。

2\. 输入quit以退出模拟器。

**一定要区分好退出模拟器，和把模拟器挂在后台。模拟器是相当占用系统资源的。**

可能有同学不小心按下ctrl+Z把模拟器挂到后台，或是不确定自己有没有把挂起的进程关掉，可以通过
jobs
命令观察后台有多少正在运行的命令。如果发现有多个模拟器正在运行的话，可以使用fg把命令调至前台然后使用ctrl+c停掉，也可以使用kill命令直接杀死。同学们可以自行了解一下linux后台进程管理的知识。

另外有一个小tips，从lab1起，git add --all之前一定要先make
clean，清空编译结果，这样可以最大限度降低评测机的存储压力。

## 任务列表

-   [**[Exercise-修改include.mk文件]{style="color: baseB"}**](#exercise1-include.mk)

-   [**[Exercise-完成readelf.c]{style="color: baseB"}**](#exercise1-readelf)

-   [**[Exercise-填写tools/scse0_3.lds中空缺的部分]{style="color: baseB"}**](#exercise1-scse0-3.lds)

-   [**[Exercise-完成boot/start.S]{style="color: baseB"}**](#exercise1-start.S)

-   [**[Exercise-完成lp_Print()函数]{style="color: baseB"}**](#exercise1-print)

## 实验思考

-   [**[思考-readelf程序的问题]{style="color: baseB"}**](#think-endian)

-   [**[思考-跨文件调用函数的问题]{style="color: baseB"}**](#think-main)

[^1]: printf位于标准库中，而不在我们的C代码中。将标准库和我们自己编写的C文件编译成一个可执行文件的过程，与将多个C文件编译成一个可执行文件的过程相仿。
    因此，通过探究printf如何和我们的C文件编译到一起，来展示整个过程。

[^2]: 为了便于你重现，这里没有选择MIPS，而选择了在流行的x86-64体系结构上进行反汇编。
    同时，由于x86-64的汇编是CISC汇编，看起来会更为清晰一些。

[^3]: 在不同机器上运行，结果也许会有一定的差异

[^4]: 为了生成更简单的汇编代码，我们采用了-nostdlib -static
    -mno-abicalls这三个编译参数

[^5]: 其实这样说并不准确，后面我们会看到，有些寄存器的值是由调用者负责保存的，有些是由被调用者保存的。但这里为了理解方便，
    我们姑且认为被调用的函数保存了调用者的所有状态吧

[^6]: 为了方便你了解自己手写汇编时应当怎样写，这一次采用汇编代码，
    而不是反汇编代码。这里注意，fp和上面反汇编出的s8其实是同一个寄存器，只是有两个不同的名字而已

[^7]: 请注意，这里的不变并不意味着它们的值在函数调用的过程中不能被改变。
    只是指它们的值在函数调用后和函数调用前是一致的。

[^8]: <https://en.cppreference.com/w/c/io/fprintf/>

[^9]: <http://www.cplusplus.com/reference/cstdio/printf/>
