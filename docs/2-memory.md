# 内存管理

## 实验目的

1.  了解内存访问原理

2.  了解MIPS内存映射布局

3.  掌握使用空闲链表的管理物理内存的方法

4.  建立页表，实现分页式虚存管理

5.  实现内存分配和释放的函数

本次实验中，需要掌握MIPS页式内存管理机制，需要使用一些数据结构来记录内存的使用情况，并实现内存分配
和释放的相关函数，完成物理内存管理和虚拟内存管理。

注意:
请不要随意修改官方代码中给定的输出语句，也不要随意添加输出语句(如有调试需要，请于调试后将其注释掉)

## MMU、TLB和内存访问

本章教程的题目是内存管理，而在进行内存管理之前，需要知道内存访问的整体原理。
我们首先介绍内存地址变换中最重要的两个部件：MMU和TLB，之后介绍内存访问的整体流程。

### MMU

根据在"计算机组成原理"和"操作系统"这两门课中学到的知识，我们知道当CPU发出一个访存的指令，需要把虚拟地址变换成物理地址才能对内存进行访问，而通过对页表相关知识的学习，同学们也对虚拟地址到物理地址的变换过程有了大致的了解，通常把这个过程称为地址变换，在计算机中负责完成这一任务的部件是MMU。

MMU的全称是Memory Management
Unit，中文为内存管理单元，MMU是硬件设备，它的功能是把逻辑地址映射为虚拟地址，并提供了一套硬件机制来实现内存访问的权限检查，它的位置和功能如下图[1.1](#lab2-mmu){reference-type="ref"
reference="lab2-mmu"}所示

![MMU的位置和功能](lab2-mmu){#lab2-mmu width="12cm"}

我们所熟悉的查询二级页表访存机制，即先查询页目录，再查询相应的二级页表的工作，都是由MMU来完成。

### TLB

访存效率在体系结构的设计中是一个很重要的问题，但我们应用的页表机制却会降低系统的性能，举例来说，如果没有页表，那么只需要一次访存，但如果有了二级页表，就需要三次访存。虽然页表机制能带来许多好处，但是这样降低效率还是无法让人接受的，为了解决这一问题，我们需要一个能让计算机能够不经过页表就把虚拟地址映射成物理地址的硬件设备，这就是TLB。

TLB的全称是Translation Lookside
Buffer，中文为翻译块表（或后备存储器），有的时候也叫关联存储器(Associative
Memory)。如下图[1.2](#lab2-tlb){reference-type="ref"
reference="lab2-tlb"}所示：

![TLB示意图](lab2-tlb){#lab2-tlb width="12cm"}

简单来说，TLB就是页表的高速缓存，每个TLB的条目中包含有一个页面的所有信息（有效位、虚页号、物理页号、修改位、保护位等等），这些条目中的内容和页表中相同页面的条目中的内容是完全一致的。

当一个虚拟地址被送到MMU中进行地址变换的时候，硬件首先在TLB中寻找包含这个地址的页面，如果它的虚页号在TLB中，并且没有违反保护位，那么就可以直接从TLB中得到相应的物理页号，而不去访问页表；如果发现虚页号在TLB中不存在，那么MMU将进行常规的页表查找，同时通过一定的策略来将这一页的页表项替换到TLB中，之后再次访问这一页的时候就可以直接在TLB中找到。

容易发现，如果TLB命中，那么只需要访问一次内存就可以得到需要的数据，从而从整体上降低了平均访存时间。

实际上，在不同进程中，其地址映射关系可能不一样，比如进程A中的0x00001000对应的物理地址和进程B中的0x00001000可能不一样。在变换地址的时候，送到MMU中进行变换的信息不仅仅包含了虚拟地址，还包含了当前进程的ASID(每个进程独一无二的标识)，用于指明要变换哪个进程的虚拟地址。由于这里涉及到进程的知识，暂不过度展开。

到此为止，我们已经基本了解了内存访问的机制。

### 内存访问

在这之前，我们一直忽略了访存的一个重要部件------------cache，计算机中的访存过程中一定存在着和cache相关的过程，而之前对cache的忽略就意味着我们对内存访问机制的了解是不全面的，这就产生了下面的三个问题：

1.  TLB、cache、MMU、页表这些东西之间有什么关系？

2.  一个完整的访存流程是怎样的？

3.  在这次实验中要完成的内存管理在这个流程中起了什么作用？

这一节中我们将分别回答这些问题。在你往下看之前，可以先思考一下这些问题，看看下面给出的解答是否符合你的预期。

接下来的讲解将会涉及到cache中的一些概念，我们假设同学们对这些概念都有一定的了解，如果感觉有些不确定或者忘了这些内容，可以回头翻翻计算机组成原理课本中的相关内容。

一般来说，cache中用来查询相应地址是否存在时所用的地址是物理地址的一部分，这就是cache中的tag。如果是这样，那么很自然的，访存时候所用的虚拟地址应该首先经过TLB和MMU变换成物理地址，然后在cache中查找是否命中，基于这样的考虑，那么完整的访存流程应该是这样：

1.  CPU给出虚拟地址来访问数据，TLB接收到这个地址之后查找是否有对应的页表项。

2.  假设页表项存在，则根据物理地址在cache中查询；如果不存在，则MMU执行正常的页表查询工作之后再根据物理地址在cache中查询，同时更新TLB中的内容。

3.  如果cache命中，则直接返回给CPU数据；如果没有命中则按照相应的算法进行cache的替换或者装填，之后返回给CPU数据。

看上去上面的访存过程没什么问题，但如果上述过程是按顺序执行的，那么自然就产生了问题，每次访存都需要先经过TLB，如果TLB命中还能接受，可如果TLB没有命中，同时这个地址又恰好在cache中存在，那么这种情况下先将地址经过MMU变换再在cache中查询岂不是会造成性能上的损失？

我们把这个问题留给你来思考。

::: thinking
[]{#think-cache label="think-cache"}
请思考cache用虚拟地址来查询的可能性，并且给出这种方式对访存带来的好处和坏处。另外，能否能根据前一个问题的解答来得出用物理地址来查询的优势?
:::

虽然提出了相应的疑问，但是可以认为上述过程是基本正确的。到此，我们已经了解了内存访问的过程。

回想前面的访存过程，我们可以发现有一个过程是没有提到的，即页表内容的填充。虽然MMU可以自动访问页表得到虚拟地址相应的物理地址，但如果只有一个空荡荡的页表，那么MMU是无法工作的；同时，操作系统需要在软件层面上对内存进行管理和控制，以便为上层的应用提供相应的接口。我们这次的实验就是围绕着这些内容来展开。

::: thinking
[]{#think-addr-type label="think-addr-type"}
在我们的实验中，有许多对虚拟地址或者物理地址操作的宏函数(详见include/mmu.h
),那么在调用这些宏的时候需要弄清楚需要操作的地址是物理地址还是虚拟地址。阅读下面的代码，指出x是一个物理地址还是虚拟地址。

``` {.c linenos=""}
    int x;
    char* value = return_a_pointer();
    *value = 10;
    x = (int) value;
```
:::

## MIPS虚存映射布局

32位的MIPS
CPU最大寻址空间为4GB(2\^32字节)，这4GB虚存空间被划分为四个部分：

1.  kuseg (TLB-mapped cacheable user space, 0x00000000
    $\scriptsize{\sim}$ 0x7fffffff)：
    这一段是用户模式下可用的地址，大小为2G，也就是MIPS约定的用户内存空间。需要通过MMU进行虚拟地址到物理
    地址的变换。

2.  kseg0 (direct-mapped cached kernel space, 0x80000000
    $\scriptsize{\sim}$ 0x9fffffff)：
    这一段是内核地址，其内存虚存地址到物理内存地址的映射变换不通过MMU，使用时只需要将地址的最高位清零
    (& 0x7fffffff)，
    这些地址就被变换为物理地址。也就是说，这段逻辑地址被连续地映射到物理内存的低端512M空间。对这段地址
    的存取都会通过高速缓存(cached)。通常在没有MMU的系统中，这段空间用于存放大多数程序和数据。对于有
    MMU 的系统，操作系统的内核会存放在这个区域。

3.  kseg1 (direct-mapped uncached kernel space, 0xa0000000
    $\scriptsize{\sim}$ 0xbfffffff)：
    与kseg0类似，这段地址也是内核地址，将虚拟地址的高 3 位清零(&
    0x1fffffff)，就可以变换到物理地址，
    这段逻辑地址也是被连续地映射到物理内存的低端512M空间。但是 kseg1
    不使用缓存(uncached)，访问速度比较慢，
    但对硬件I/O寄存器来说，也就不存在Cache一致性的问题了，这段内存通常被映射到I/O寄存器，用来实现对外设的访问。

4.  kseg2 (TLB-mapped cacheable kernel space, 0xc0000000
    $\scriptsize{\sim}$ 0xffffffff):
    这段地址只能在内核态下使用，并且需要 MMU 的变换。

## 内存管理与地址变换

内存管理的实质是为每个程序提供自己的内存空间。最为朴素的内存管理就是直接将物理内存分配给程序。
但从之前的叙述中，我们可以发现，MIPS系统中使用了虚拟内存的技术。因此在我们的系统中，同时存在着两套地址，
一套是真实的物理地址，另一套则是虚拟地址。内存管理需要完成许多地址变换的工作。这些工作大多就是由
前文所述的MMU来完成。 为什么要这么做呢？原因有很多：

1.  隐藏与保护：因为加入了虚拟内存这一中间层，真实的物理地址对用户级程序是不可见的，它只能访问
    操作系统允许其访问的内存区域。

2.  为程序分配连续的内存空间：利用虚拟内存，操作系统可以从物理分散的内存页中，构建连续的程序空间，
    这使得我们拥有更高的内存利用率。

3.  扩展地址空间范围：如前文所述，通过虚拟内存，MIPS32位机拥有了4GB的寻址能力，这真的很cool。:)

4.  使内存映射适合你的程序：在大型操作系统中，可能存在相同程序的多个副本同时运行，这时候通过地址变换
    这一中间层，你能使他们都使用相同的程序地址，这让很多工作都简单了很多。

5.  重定位：程序入口地址和预先声明的数据在程序编译的过程中就确定了。但通过MMU的地址变换，我们能够让程
    序运行在内存中的任何位置。

为了这些好处，需要付出地址变换工作的代价。接下来在lab的工作中，也将一直遇到这个问题。建议大家在lab的
过程中，不妨思考当前我们使用的这个地址究竟是物理地址还是虚拟地址，搞清楚这一点，对lab和操作系统的理解都
大有帮助。

## 物理内存管理

### 初始化流程说明

在lab1实验中，我们将内核加载到内存中的 kseg0
区域(0x80010000)，成功启动并跳转到 init/main.c 中的 main
函数开始运行。现在需要在 main 函数中调用定义在 init/init.c 中的
mips_init() 函数，并 进一步通过

1.  `mips_detect_memory()`{.c}; 初始化物理内存大小以及物理页面数量的值

2.  `mips_vm_init()`{.c}; 建立二级页表

3.  `page_init()`{.c}; 建立基于物理页面的物理内存管理机制

这三个函数来实现物理内存管理的相关数据结构的初始化。

### 内存控制块

在MIPS CPU 中，地址变换以4KB
大小为单位，称为页。整个物理内存按4KB大小分成了许多页，大多数时候的
内存分配，也是以页为单位来进行。为了记录分配情况，我们需要使用 Page
结构体来记录一页内存的相关信息：

``` {.c linenos=""}
typedef LIST_ENTRY(Page) Page_LIST_entry_t;

struct Page {
    Page_LIST_entry_t pp_link;  /* free list link */
    u_short pp_ref;
};
```

其中，`pp_ref`{.c} 用来记录这一物理页面的引用次数，`pp_link`{.c}
是当前节点 指向链表中下一个节点的指针，其类型为 `LIST_ENTRY(Page)`{.c}
。

::: thinking
[]{#think-linklist label="think-linklist"} 我们在
include/queue.h中定义了一系列的宏函数来简化对链表的操作。
实际上，我们在include/queue.h文件中定义的链表和glibc相关源码较为相似，这一链表设计也应用于Linux系统中(sys/queue.h文件)。
请阅读这些宏函数的代码，说说它们的原理和巧妙之处。
:::

::: thinking
[]{#think-do_while label="think-do_while"}
我们注意到我们把宏函数的函数体写成了 `do { /* ... */ } while(0)`{.c}
的形式，而不是 仅仅写成形如 `{ /* ... */ }`{.c}
的语句块，这样的写法好处是什么？
:::

::: exercise
[]{#exercise2-queue.h label="exercise2-queue.h"}
在阅读queue.h文件之后，相信你对宏函数的巧妙之处有了更深的体会。
请完成queue.h中的`LIST_INSERT_AFTER`{.c}函数和`LIST_INSERT_TAIL`{.c}函数，分别实现将元素插入到链表首部与链表尾部的功能。
同时，希望大家自行思考这些操作各自的复杂度。
:::

在 include/pmap.h 中，我们使用 LIST_HEAD 宏来定义了一个结构体类型
Page_list，这个结构体是有表头链表的表头结构， 参与到链表的维护操作中 。
然后 ， 我们提供的代码在 mm/pmap.c 中，创建了一个 Page_list 类型的变量
page_free_list 来以链表的形式表示所有的空闲物理内存：

``` {.c linenos=""}
LIST_HEAD(Page_list, Page);

static struct Page_list page_free_list; /* Free list of physical pages */
```

::: thinking
[]{#think-struct-page label="think-struct-page"} 注意，我们定义的 Page
结构体只是一个信息的载体，它只代表了相应物理内存页的信息，它本身并不是物理内存页。
那物理内存页究竟在哪呢？Page
结构体又是通过怎样的方式找到它代表的物理内存页的地址呢？ 请阅读
include/pmap.h 与 mm/pmap.c 中相关代码，并思考一下。
:::

::: thinking
[]{#think-page label="think-page"} 请阅读 include/queue.h 以及
include/pmap.h,
将Page_list的结构梳理清楚,选择正确的展开结构(请注意指针)。

``` {.c linenos=""}
A:
struct Page_list{
    struct {
         struct {
            struct Page *le_next;
            struct Page **le_prev;
        }* pp_link;
        u_short pp_ref;
    }* lh_first;
}
```

``` {.c linenos=""}
B:
struct Page_list{
    struct {
         struct {
            struct Page *le_next;
            struct Page **le_prev;
        } pp_link;
        u_short pp_ref;
    } lh_first;
}
```

``` {.c linenos=""}
C:
struct Page_list{
    struct {
         struct {
            struct Page *le_next;
            struct Page **le_prev;
        } pp_link;
        u_short pp_ref;
    }* lh_first;
}
```
:::

### 内存分配和释放

首先，需要注意在 mm/pmap.c 中定义的与内存相关的全局变量：

``` {.c linenos=""}
u_long maxpa;            /* Maximum physical address */
u_long npage;            /* Amount of memory(in pages) */
u_long basemem;          /* Amount of base memory(in bytes) */
u_long extmem;           /* Amount of extended memory(in bytes) */
```

::: exercise
[]{#exercise2-mips-detect-memory label="exercise2-mips-detect-memory"}
我们需要在 mips_detect_memory()
函数中初始化这几个全局变量，以确定内核可用的物理内存的大小和范围。
根据代码注释中的提示，完成 mips_detect_memory() 函数。

提示：

1.  物理内存的大小为64MB，页的大小为4KB。

2.  maxpa 指的是 总共有多少物理内存(以字节为单位)， npage
    指的是这些物理内存被划分为了多少个物理页。

3.  值得注意的是， 物理内存可能分为 basemem 与
    extmem，在本实验中，仅考虑basemem，即
    basemem的总量与maxpa一致，extmem为0 。
:::

在操作系统刚刚启动时，我们还没有建立起有效的数据结构来管理所有的物理内存。因此，出于最基本的
内存管理的需求，需要实现一个函数来分配指定字节的物理内存。这一功能由
mm/pmap.c 中的 alloc函数来实现。

``` {.c linenos=""}
static void *alloc(u_int n, u_int align, int clear);
```

alloc 函数能够按照参数 align 进行对齐，然后分配 n
字节大小的物理内存，并根据参数 clear 的设定决
定是否将新分配的内存全部清零，并最终返回新分配的内存的首地址。

::: thinking
[]{#think-bzero label="think-bzero"} 在 mmu.h
中定义了`bzero(void *b, size_t)`{.c}这样一个函数。请思考一下，此处的b指针是一个物理地址，
还是一个虚拟地址呢？
:::

有了分配物理内存的功能后，接下来需要给操作系统内核必须的数据结构 --
页目录（pgdir）、内存控制块
数组（pages）和进程控制块数组（envs）分配所需的物理内存。mips_vm_init()
函数实现了这一功能， 并且完成了相关的虚拟内存与物理内存之间的映射。

完成上述工作后，便可以通过在 mips_init() 函数中调用 page_init()
函数将余下的物理内存块加 入到空闲链表中。

::: exercise
[]{#exercise2-page-init label="exercise2-page-init"} 完成 page_init
函数，使用 include/queue.h 中定义的宏函数将未分配的物理页加入到空闲链表
page_free_list
中去。思考如何区分已分配的内存块和未分配的内存块，并注意内核可用的物理内存上限。
提示：

1.  首先，需要使用定义在include/queue.h中的宏函数 LIST_INIT
    去初始化空闲页面链表 page_free_list
    （这个链表的意义在前文已经提及，如有遗忘可以往回查阅）。

2.  在 mm/pmap.c 中，我们定义了一个u_long（非负长整数）变量 freemem，
    这个变量是在没有建立起基于物理页面的物理内存管理机制前用于物理内存的分配，
    用于记录已经分配了多少物理内存出去、下一次分配应该从哪里开始继续分配。
    在我们完整的MOS
    进行内存管理初始化的时候，在page_init函数被调用前，freemem会被别的函数修改，比如定义在mm/pmap.c
    中的函数
    `static void *alloc`{.c}（这个函数我们已经为大家实现了，但仍然希望大家阅读代码并看看哪些函数调用了alloc），这也就意味着在page_init中我们所遇到的freemem不一定是按照BY2PG（宏常量，代表一个页面有多少个字节，定义于include/mmu.h）对齐的。但是，在page_init中我们需要将剩余的空闲物理内存按照页面为单位管理起来，这也就要求我们将freemem按照BY2PG进行对齐。考虑到我们先前分配物理内存是从低地址到高地址分配，所以我们需要将freemem以BY2PG为单位，向上取整，我们为大家提供了实现该功能的宏函数ROUND（定义于include/types.h）。

3.  通过向上取整，我们此时可以认为freemem以下的物理页面是已经被使用了的，需要对其对应的Page结构体进行修改。值得注意的是这里通过freemem计算已经有多少个物理页面被使用了不能直接拿freemem除以BY2PG，需要写成PADDR(freemem)除以BY2PG，其原理此处暂不解释，待学完虚拟内存管理，想必就可以理解此操作的含义了。

4.  除去freemem以下的页面，剩下的物理页面都是空闲的了，只需要将其各自对应的结构体进行标记，并使用LIST_INSERT_HEAD将其插入page_free_list中。
    为了不影响课程评测，需要按照结构体对应物理页面起始地址从低到高的顺序进行插入。
:::

有了记录物理内存使用情况的链表之后，我们就可以不再像之前的 alloc
函数那样按字节为单位进行内存 的分配，而是可以以页为单位
进行物理内存的分配与释放。page_alloc 函数用来从空闲链表中分配一页
物理内存，而 page_free
函数则用于将一页之前分配的内存重新加入到空闲链表中。

::: exercise
[]{#exercise2-page-alloc label="exercise2-page-alloc"} 完成 mm/pmap.c
中的 page_alloc 和 page_free 函数，基于空闲内存链表 page_free_list
，以页
为单位进行物理内存的管理。并在init/init.c的函数mips_init中注释掉page_check()。

page_alloc的提示：

1.  首先，需要判断当前是否有空闲页面（用LIST_FIRST判断page_free_list
    是否为空），如果没有则返回错误编码 E_NO_MEM
    （定义于include/error.h）,如果有，则记录其该结构体的地址，并用LIST_REMOVE将其从空闲页面链表中删除

2.  取出的物理页面需要使用 bzero
    函数（定义于init/init.c）将这一段内存清零。然后返回0，表示函数执行成功。

page_free的提示：

1.  首先，如果该页面的引用次数（pp_ref）
    大于0，则说明该物理页还被某些虚拟页面所使用（可以看完虚拟内存部分再加深理解），则不应该释放该物理内存，函数执行结束。

2.  如果该页面的引用次数等于0，则说明没有虚拟页面使用该物理页面，可以将其回收到空闲页面链表中，使用LIST_INSERT_HEAD将其插入即可。

3.  如果该页面的引用次数小于0，这是不应该出现的情况，说明系统出现严重错误。

如果到此为止的各个exercise都实现正确，那么此时运行结果应该如下。
:::

``` {.c linenos=""}
main.c: main is start ...
init.c: mips_init() is called
Physical memory: 65536K available, base = 65536K, extended = 0K
to memory 80401000 for struct page directory.
to memory 80431000 for struct Pages.
pmap.c:  mips vm init success
*temp is 1000
phycial_memory_manage_check() succeeded
panic at init.c:17: ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
```

注意：最后一行的数字 17
是不固定的，因个人代码而异，究其原因，可以研读定义在include/printf.h
中的宏函数panic。

至此，我们的内核已经能够按照分页的方式对物理内存进行管理,

## 虚拟内存管理

我们通过建立两级页表来进行虚拟内存的管理。在此基础上，将实现根据虚拟地址在页表中查找
对应的物理地址，以及将一段虚存地址映射到一段的物理地址的功能，然后实现虚存的管理与释放，最后为内核建立起一
套虚存管理系统。

### 两级页表机制

我们的操作系统内核采取二级页表结构，如图[1.3](#lab2-pic-1){reference-type="ref"
reference="lab2-pic-1"}所示：

![二级页表结构示意图](lab2-pic-1){#lab2-pic-1 width="12cm"}

第一级表称为页目录(page
directory)，一共1024个页目录项，每个页目录项32位(4 Bytes)，页目录项存储
的值为其对应的二级页表入口的物理地址。整个页目录存放在一个页面(4KB)中，也就是我们在
mips_vm_init 函数中为其分配了相应的物理内存。第二级表称为页表(page
table)，每一张页表有1024个页表项，每个页表 项32位(4
Bytes)，页表项存储的是对应页面的页框号(20位)以及标志位(12位)。每张页表占用一个页面大小
(4KB)的内存空间。

对于一个32位的虚存地址，其31-22位表示的是页目录项的索引，21-12位表示的是页表项的索引，11-0位表示的
是该地址在该页面内的偏移。

### 地址变换

对于操作系统来说，虚拟地址与物理地址之间的变换是内存管理中非常重要的内容。在这一部分，我们将详细探讨
咱们的内核是如何进行地址变换的。

首先从较为简单的形式开始。在前面的实验中，通过设置 lds
文件让操作系统内核加载到内存的 0x80010000 位置。通过前面的MIPS
存储器映射布局的介绍中，我们知道这一地址对应的是 kseg0
区域，这一部分的地址 变换不通过 MMU
进行。我们也称这一部分虚拟地址为内核虚拟地址。从虚拟地址到物理地址的变换只需要清掉
最高位的1即可，反过来，将对应范围内的物理地址变换到内核虚拟地址，也只需要将最高位设置为1即可。我们在
include/mmu.h 中定义了 PADDR 和 KADDR 两个宏来实现这一功能：

``` {.c linenos=""}
// translates from kernel virtual address to physical address.
#define PADDR(kva)            \
  ({                \
    u_long a = (u_long) (kva);        \
    if (a < ULIM)         \
      panic("PADDR called with invalid kva %08lx", a);\
    a - ULIM;           \
  })

// translates from physical address to kernel virtual address.
#define KADDR(pa)           \
  ({                \
    u_long ppn = PPN(pa);         \
    if (ppn >= npage)         \
      panic("KADDR called with invalid pa %08lx", (u_long)pa);\
    (pa) + ULIM;          \
  })
```

在 PADDR 中，使用了一个宏 ULIM ，这个宏定义在 include/mmu.h 中，其值为
0x80000000。对于小于 0x80000000
的虚拟地址值，显然不可能是内核区域的虚拟地址。在 KADDR
中，一个合理的物理地址的物理 页框号显然不能大于在 mm/pmap.c
中所定义的物理内存总页数 npage 的值。

::: note
形如 `({ /* ... */ })`{.c}
的表达式是GCC的扩展语法，含义为执行其中的语句，并返回最后一个表达式的值。
:::

接下来，讨论如何通过二级页表进行虚拟地址到物理地址的变换。

首先，可以通过 PDX(va)
来获得一个虚拟地址对应的页目录索引，然后直接凭借索引在页目录中得到对应的
二级页表的基址(物理地址)，然后把这个物理地址转化为内核虚拟地址(KADDR)，之后，通过
PTX(va) 获得这个
虚存地址对应的页表索引，然后就可以从页表中得到对应的页面的物理地址。整个变换的过程如图[1.4](#lab2-pic-2){reference-type="ref"
reference="lab2-pic-2"} 所示。

![地址变换过程](lab2-pic-2){#lab2-pic-2 width="12cm"}

### 页目录自映射

3.6.1节中我们介绍二级页表结构的时侯提到，要映射整个4G地址空间，一共需要1024个页表和1个页目录的。一个页表占用
4KB 空间，页目录也占用 4KB 空间，也就是说，整个二级页表结构将占用
4MB+4KB 的存储空间，1024\*4KB+1\*4KB=4MB+4KB。

在 include/mmu.h 中的内存布局图里有这样的内容：

``` {.c linenos=""}
/**
 *      ULIM     -----> +----------------------------+------------0x8000 0000-------
 *                      |         User VPT           |     PDMAP                /|\
 *      UVPT     -----> +----------------------------+------------0x7fc0 0000    |
 */
```

不难计算出 UVPT(0x7fc00000) 到 ULIM(0x80000000) 之间的空间只有 4MB
，这一区域就是进程的 页表的位置，于是我们不禁想问：页目录所占用的 4KB
内存空间在哪儿？

**答案就在于页目录的自映射机制！**

如果页表和页目录没有被映射到进程的地址空间中，而一个进程的4GB地址空间又都映射了物理内存的话，那么就确实需要
1024个物理页(4MB)来存放页表，和另外1个物理页(4KB)来存放页目录，也就是需要(4M+4K)的物理内存。但是页表也被映射
到了进程的地址空间中，也就意味着 1024 个页表中，有一个页表所对应的 4M
空间正是这 1024 个页表所占用的 4M 内存， 这个页表的 1024 的页表项存储了
1024 个物理地址，分别是 1024
个页表的物理地址。而在二级页表结构中，页目录 对应着二级页表，1024
个页目录项存储的也是全部 1024
个页表的物理地址。也就是说，一个页表的内容和页目录的
内容是完全一样的，**正是这种完全相同**，使得将 1024 个页表加 1
个页目录映射到地址空间只需要 4M 的地址空间。

接下来我们会想到这样一个问题：那么，这个与页目录重合的页表，也就是页目录究竟在哪儿呢？

这 4M
空间的起始位置也就是第一个二级页表对应着页目录的第一个页目录项，同时，由于
1M 个页表项和 4G 地址空间是线性映射，不难算出 0x7fc00000
这一地址对应的应该是第 (0x7fc00000 `>>` 12) 个页表项，这个页表项
也就是第一个页目录项。一个页表项 32 位，占用 4
个字节的内存，因此，其相对于页表起始地址 0x7fc000000 的 偏移为
(0x7fc00000 `>>` 12) \* 4 = 0x1ff000 ，于是得到地址为 0x7fc00000 +
0x1ff000 = 0x7fdff000 。也就是 说，页目录的虚存地址为 0x7fdff000。

::: thinking
[]{#think-windows_pde_addr label="think-windows_pde_addr"}
了解了二级页表页目录自映射的原理之后，我们知道，Win2k内核的虚存管理也是采用了二级页表的形式，其页表所占的
4M 空间对应的虚存起始地址为
0xC0000000，那么，它的页目录的起始地址是多少呢？
:::

::: thinking
[]{#think-skip_page_dir label="think-skip_page_dir"}
注意到页表在进程地址空间中连续存放，并线性映射到整个地址空间，思考：
是否可以由虚拟地址直接得到对应页表项的虚拟地址？3.6.2节末尾所述变换过程中，第一步查页目录有必要吗，为什么？
:::

### 创建页表

将虚拟地址变换为物理地址的过程中，如果这个虚拟地址所对应的二级页表不存在，可能需要为这个
虚拟地址创建一个新的页表。我们需要申请一页物理内存来存放这个页表，然后将他的物理地址赋值给对应的页
目录项，最后设置好页目录项的权限位即可。

内核在 mm/pmap.c 中分定义了 boot_pgdir_walk 和 pgdir_walk
两个函数来实现地址变换和
页表的创建，这两个函数的区别仅仅在于当虚存地址所对应的页表的页面不存在的时候，分配策略的不同和使用的
内存分配函数的不同。前者用于内核刚刚启动的时候，这一部分内存通常用于存放内存控制块和进程控制块等数据
结构，相关的页表和内存映射必须建立，否则操作系统内核无法正常执行后续的工作。然而用户程序的内存
申请却不是必须满足的，当可用的物理内存不足时，内存分配允许出现错误。boot_pgdir_walk
被调用时， 还没有建立起空闲链表来管理物理内存，因此，直接使用 alloc
函数以字节为单位进行物理内存的分配，而 pgdir_walk
则在空闲链表初始化之后发挥功能，因此，直接使用 page_alloc
函数从空闲链表中 以页为单位进行内存的申请。

上文中介绍了页目录自映射的相关知识，我们了解到页目录其实也是一个页表。在咱们实验使用的内核中，
一个页表指向的物理页面存在的标志是页表项存储的值的 PTE_V 标志位被置为 1
。因此，在将页表的物理地址赋值
给页目录项时，我们还为页目录项设置权限位。

::: exercise
[]{#exercise2-pgdir-walk label="exercise2-pgdir-walk"} 完成 mm/pmap.c
中的 boot_pgdir_walk 和 pgdir_walk
函数，实现虚拟地址到物理地址的变换以及创建页表的功能。
提示：仔细阅读mmu.h和相关代码，寻找并使用其中有用的宏函数。

提示：

1.  这两个函数十分相似，其作用都是根据给出的二级页表起始地址pgdir、虚拟地址va以及参数create进行虚拟地址到页目录表项的映射。如果该虚拟地址va没有对应的物理页面，那么将根据create参数决定是否为其分配物理页面。

2.  两者的不同之处在于 boot_pgdir_walk
    在尚未通过page_init建立起基于页面的物理内存管理时就已经被使用，而pgdir_walk在调用pgdir_walk之后才可以使用。

3.  由于第2点，在需要分配物理页面（内存）时，boot_pgdir_walk需调用alloc函数，而pgdir_walk调用page_alloc、pgdir_walk需要将分配的物理页面的引用次数加一，而boot_pgdir_walk不用（也不能）。

4.  使用 page2pa 函数获取 Page \* 代表的相应物理内存页的物理地址。
:::

### 地址映射

一个空荡荡的页表自然不会对内存地址变换有帮助，我们需要在具体的物理内存与虚拟地址间建立映射，即将相应的物理页面地址填入对应虚拟地址的页表项中。

::: exercise
[]{#exercise2-boot-map-segment label="exercise2-boot-map-segment"} 实现
mm/pmap.c 中的 boot_map_segment
函数，实现将指定的物理内存与虚拟内存建立起映射的功能。其作用是在pgdir对应的页表上，把
\[va,va+size) 这一部分的虚拟地址空间映射到 \[pa,pa+size) 这一部分空间。
:::

实验提示：

1.  boot_map_segment 函数是在boot阶段执行的，使用 boot_pgdir_walk
    寻找页表项

2.  映射的内存区间可能大于一页，所以应当逐页地进行映射

3.  注意要设置对应的标志位，即，PTE_V与PTE_R。

### page insert and page remove

``` {.c linenos=""}
// Overview:
//   Map the physical page 'pp' at virtual address 'va'.
//   The permissions (the low 12 bits) of the page table entry
//   should be set to 'perm|PTE_V'.
//
// Post-Condition:
//   Return 0 on success
//   Return -E_NO_MEM, if page table couldn't be allocated
//
// Hint:
//   If there is already a page mapped at `va`, call page_remove()
//   to release this mapping.The `pp_ref` should be incremented
//   if the insertion succeeds.
int
page_insert(Pde *pgdir, struct Page *pp, u_long va, u_int perm)
{
    u_int PERM;
    Pte *pgtable_entry;
    PERM = perm | PTE_V;

    /* Step 1: Get corresponding page table entry. */
    pgdir_walk(pgdir, va, 0, &pgtable_entry);

    if (pgtable_entry != 0 && (*pgtable_entry & PTE_V) != 0) {
        if (pa2page(*pgtable_entry) != pp) {
            page_remove(pgdir, va);
        }
        else{
            tlb_invalidate(pgdir, va);
            *pgtable_entry = (page2pa(pp) | PERM);
            return 0;
        }
    }

    /* Step 2: Update TLB. */

    /* hint: use tlb_invalidate function */

    /* Step 3: Do check, re-get page table entry to validate the insertion. */
    
    /* Step 3.1 Check if the page can be insert, if can’t return -E_NO_MEM */

    /* Step 3.2 Insert page and increment the pp_ref */

    return 0;
}
```

这个函数**非常重要**！它将在后续实验**lab3和lab4**中被反复用到，这个函数
将va虚拟地址和其要对应的物理页pp的映射关系以perm的权限设置加入页目录。我们大概讲一下函数的执行流程与执行要点。

**流程大致如下：**

1.  先判断va是否有效，如果va
    已经有了映射的物理地址的话，则去判断这个物理地址是不是我们要插入的那个物理地址。如果不是，那么就把该物理地址移除掉；如果是，则修改权限，更新
    TLB，并直接返回。

2.  更新TLB，只要对页表的内容有修改，都必须用tlb_invalidate来让TLB更新。

3.  查找页表项，根据需要创建页表。如果成功，则填入页表项，并增加va映射的物理页的引用计数。否则返回错误代码。

**有一个值得指出的要点**：我们能看到，只要对页表的内容修改，都必须tlb_invalidate
来让TLB更新，否则后面紧接着对内存的访问很有可能出错。

可以说tlb_invalidate
函数是它的一个核心子函数，这个函数实际上又是由tlb_out 汇编函数组成的。

::: thinking
[]{#think-pter label="think-pter"}
观察给出的代码可以发现，page_insert会默认为页面设置PTE_V的权限。请问，你认为是否应该将PTE_R也作为默认权限？并说明理由。
:::

::: exercise
[]{#exercise2-page-insert label="exercise2-page-insert"} 完成 mm/pmap.c
中的 page_insert 函数。可能需要用到的函数为： pgdir_walk, pa2page,
page_remove, tlb_invalidate, page2pa
:::

::: codeBoxWithCaption
TLB汇编函数[]{#code:tlb_out.S label="code:tlb_out.S"}

``` {.gas linenos=""}
#include <asm/regdef.h>
#include <asm/cp0regdef.h>
#include <asm/asm.h>

LEAF(tlb_out)
	nop
	mfc0	k1,CP0_ENTRYHI
	mtc0	a0,CP0_ENTRYHI
	nop
	// insert tlbp or tlbwi
	nop
	nop
	nop
	nop
	mfc0	k0,CP0_INDEX
	bltz	k0,NOFOUND
	nop
	mtc0	zero,CP0_ENTRYHI
	mtc0	zero,CP0_ENTRYLO0
	nop
	// insert tlbp or tlbwi
NOFOUND:

	mtc0	k1,CP0_ENTRYHI
	
	j	ra
	nop
END(tlb_out)

```
:::

这个汇编函数相对其他汇编函数来说相对简单，它的作用是使得某一虚拟地址对应的tlb表项失效。从而下次访问这个地址的时候诱发tlb重填，以保证数据更新。

在空出的两行位置，你需要填写tlbp或tlbwi指令，那么请思考几个问题。

::: thinking
[]{#think-tlb label="think-tlb"} 思考一下tlb_out
汇编函数，结合代码阐述一下跳转到**NOFOUND**的流程？
从MIPS手册中查找tlbp和tlbwi指令，明确其用途，并解释为何第10行处指令后有4条nop指令。
:::

::: exercise
[]{#exercise2-tlb-out label="exercise2-tlb-out"} 完成 mm/tlb_asm.S 中
tlb_out 函数。
:::

### 访存与TLB重填

通过之前的实验，我们可以知道，虚拟地址通过MMU变换成物理地址，然后通过物理地址我们能够在主存中获得相应的数据。而实际上，在MIPS架构中，关于这一块
地址变换的内容，很大程度上与TLB有关。TLB可以看做是一块页表的高速缓存，里面存储了一些物理页面与虚拟页面的对应关系。而当CPU访问相应内存地址时，会先去
TLB中查询。当TLB中没有相应对应关系时会触发一个**TLB缺失异常**。而MIPS将这个异常的处理，全权交给了软件。因此若发生缺失异常，则会跳转到相应异常处理程序中，
再由我们的二级页表进行相应的地址变换，对TLB进行重填。换句话说，MIPS中并没有一个执行内存地址变换的MMU处理器，CPU完成了相应工作。

如果大家仔细地理解了上面这段话，就会发现MMU的正常工作需要**异常处理**的支持。由于我们暂时还不了解异常的相关内容，因此在本次实验中实际上并没有真正地启用MMU，而只是先创建了页表。
在lab3中学习完中断和异常后，我们就能够见到一个真正启用的MMU了。

### 再深入探究一下访存

让我们来一起探索一个假设情境：在page_check最后一句printf之前添加如下代码段。
提醒一下，由于MMU暂时没有启用，这段代码目前不能在我们的系统中实际运行：

``` {.c linenos=""}
    u_long* va = 0x12450;
    u_long* pa;

    page_insert(boot_pgdir, pp, va, PTE_R);
    pa = va2pa(boot_pgdir, va);
    printf("va: %x -> pa: %x\n", va, pa);
    *va = 0x88888;
    printf("va value: %x\n", *va);
    printf("pa value: %x\n", *((u_long *)((u_long)pa + (u_long)ULIM)));
```

这段代码旨在计算出相应va与pa的对应关系，设置权限位为PTE_R是为了能够将数据写入内存。

如果MMU能够正常工作，实际输出将会是：

``` {.c linenos=""}
    va: 12450 -> pa: 3ffd000
    va value: 88888
    pa value: 0
    page_check() succeeded!
```

::: thinking
[]{#think-memory-access label="think-memory-access"}
显然，运行后结果与我们预期的不符，va值为0x88888，相应的pa中的值为0。这说明代码中存在问题，请仔细思考我们的访存模型，指出问题所在。
:::

另外，还可以提醒大家的是，在gxemul中，有tlbdump这个命令，可以随时查看tlb中的内容。

::: thinking
[]{#think-PSE label="think-PSE"}
在X86体系结构下的操作系统，有一个特殊的寄存器CR4，在其中有一个PSE位，当该位设为1时将开启4MB大物理页面模式，请查阅相关资料，说明当PSE开启时的页表组织形式与我们当前的页表组织形式的区别。
:::

最后，做一个说明，我们的MOS操作系统在lab2部分实现的内存管理部分没有页面交换的部分。

## 正确结果展示

lab2做完之后，在init/init.c的函数mips_init中，将page_check还原，并注释掉physical_memory_manage_check();。运行的正确结果应该是这样的：

``` {.c linenos=""}
main.c: main is start ...
init.c: mips_init() is called
Physical memory: 65536K available, base = 65536K, extended = 0K
to memory 80401000 for struct page directory.
to memory 80431000 for struct Pages.
mips_vm_init:boot_pgdir is 80400000
pmap.c:  mips vm init success
start page_insert
va2pa(boot_pgdir, 0x0) is 3ffe000
page2pa(pp1) is 3ffe000
pp2->pp_ref 0
end page_insert
page_check() succeeded!
panic at init.c:55: ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
```

地址**3ffe000**和最后一行的数字 **55** 是不固定的。

## 任务列表

-   [**[Exercise-完成queue.h]{style="color: baseB"}**](#exercise2-queue.h)

-   [**[Exercise-完成 mips_detect_memory
    函数]{style="color: baseB"}**](#exercise2-mips-detect-memory)

-   [**[Exercise-完成 page_init
    函数]{style="color: baseB"}**](#exercise2-page-init)

-   [**[Exercise-完成 page_alloc 和 page_free
    函数]{style="color: baseB"}**](#exercise2-page-alloc)

-   [**[Exercise-完成 boot_pgdir_walk 和 pgdir_walk
    函数]{style="color: baseB"}**](#exercise2-pgdir-walk)

-   [**[Exercise-实现 boot_map_segment
    函数]{style="color: baseB"}**](#exercise2-boot-map-segment)

-   [**[Exercise-完成 page_insert
    函数]{style="color: baseB"}**](#exercise2-page-insert)

-   [**[Exercise-完成 tlb_out
    函数]{style="color: baseB"}**](#exercise2-tlb-out)

## 实验思考

-   [**[思考-不同地址类型查询cache的比较]{style="color: baseB"}**](#think-cache)

-   [**[思考-地址类型判断]{style="color: baseB"}**](#think-addr-type)

-   [**[思考-链表宏函数]{style="color: baseB"}**](#think-linklist)

-   [**[思考-使用do-while(0)语句的好处]{style="color: baseB"}**](#think-do_while)

-   [**[思考-Page结构体与物理内存页]{style="color: baseB"}**](#think-struct-page)

-   [**[思考-Page_list的展开结构]{style="color: baseB"}**](#think-page)

-   [**[思考-bzero参数探究]{style="color: baseB"}**](#think-bzero)

-   [**[思考-自映射机制页目录地址的计算]{style="color: baseB"}**](#think-windows_pde_addr)

-   [**[思考-软件变换中跳过页目录的可行性]{style="color: baseB"}**](#think-skip_page_dir)

-   [**[思考-PTE_R的设置]{style="color: baseB"}**](#think-pter)

-   [**[思考-NOFOUND的奥妙及两条MIPS指令]{style="color: baseB"}**](#think-tlb)

-   [**[思考-访存的问题]{style="color: baseB"}**](#think-memory-access)

-   [**[思考-大物理页面]{style="color: baseB"}**](#think-PSE)
