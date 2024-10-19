# 系统调用与fork

## 实验目的

1.  掌握系统调用的概念及流程

2.  实现进程间通信机制

3.  实现fork函数

4.  掌握缺页中断的处理流程

一般情况下，用户进程不能够存取系统内核的地址空间，也就是说它不能存取内核使用的内存数据，也不能直接调用内核函数，
这一点是由CPU的硬件结构保证的。然而，用户进程在特定的场景下是需要进行一些只能在内核中执行的操作，如
对硬件的操作。这种时候允许内核执行用户提供的代码显然是不安全的，所以操作系统也就设计了一系列内核空间
的函数。当用户进程以特定的方式陷入异常后，能够由内核调用对应的函数，我们把这些函数称为**系统调用**。
在这一节的实验中，我们需要实现系统调用机制，并在此基础上实现进程间通信（IPC）机制和一个重要的系统调用fork。
在fork的实验中，我们会介绍一种被称为写时复制的特性，而与这种特性相关的正是内核的缺页中断处理机制。

## 系统调用(System Call)

本节中，我们着重讨论系统调用的作用，并完成实现相关的内容。

### 一探到底，系统调用的来龙去脉

说起系统调用，同学们第一个问题一定是：系统调用到底是什么样子？为了一探究竟，我们选择一个极为简单的程序作为实验对象。
在这个程序中，通过puts来输出一个字符串到终端。

``` {.c linenos=""}
#include <stdio.h>

int main() {
        puts("Hello World!\n");
        return 0;
}
```

::: note
如果还记得C语言课上关于标准输出的相关知识的话，大家一定知道在C语言中，终端被抽象为了标准输出文件stdout。
通过向标准输出文件写东西，就可以输出内容到屏幕。而向文件写入内容是通过write系统调用完成的。
因此，我们选择通过观察puts函数，来探究系统调用的奥秘。
:::

通过GDB进行单步调试，逐步深入到函数之中，观察puts具体的调用过程[^1]。运行GDB，将断点设置在puts这条语句上，并通过stepi指令[^2]
单步进入到函数中。当程序到达write函数时停下，因为write正是Linux的一条系统调用。我们打印出此时的函数调用栈，
可以看出，C标准库中的puts函数实际上通过了很多层函数调用，最终调用到了底层的write函数进行真正的屏幕打印操作。

``` {.text linenos=""}
(gdb)
0x00007ffff7b1b4e0 in write () from /lib64/libc.so.6
(gdb) backtrace
#0  0x00007ffff7b1b4e0 in write () from /lib64/libc.so.6
#1  0x00007ffff7ab340f in _IO_file_write () from /lib64/libc.so.6
#2  0x00007ffff7ab2aa3 in ?? () from /lib64/libc.so.6
#3  0x00007ffff7ab4299 in _IO_do_write () from /lib64/libc.so.6
#4  0x00007ffff7ab462b in _IO_file_overflow () from /lib64/libc.so.6
#5  0x00007ffff7ab5361 in _IO_default_xsputn () from /lib64/libc.so.6
#6  0x00007ffff7ab3992 in _IO_file_xsputn () from /lib64/libc.so.6
#7  0x00007ffff7aaa4ef in puts () from /lib64/libc.so.6
#8  0x0000000000400564 in main () at test.c:4
```

通过gdb显示的信息，可以看到，这个write()函数是在libc.so这个动态链接库中的，也就是说，它仍然是C库中的函数，
而不是内核中的函数。因此，该write()函数依旧是个用户空间函数。为了彻底揭开分析这个函数，我们对其进行反汇编。

``` {.text linenos=""}
(gdb) disassemble 0x00007ffff7b1b4e0
Dump of assembler code for function write:
=> 0x00007ffff7b1b4e0 <+0>:     cmpl   $0x0,0x2bf759(%rip)        # 0x7ffff7ddac40
   0x00007ffff7b1b4e7 <+7>:     jne    0x7ffff7b1b4f9 <write+25>
   0x00007ffff7b1b4e9 <+9>:     mov    $0x1,%eax
   0x00007ffff7b1b4ee <+14>:    syscall
   0x00007ffff7b1b4f0 <+16>:    cmp    \$0xfffffffffffff001,%rax
   0x00007ffff7b1b4f6 <+22>:    jae    0x7ffff7b1b529 <write+73>
   0x00007ffff7b1b4f8 <+24>:    retq
End of assembler dump.
```

通过gdb的反汇编功能，可以看到，这个函数最终执行了syscall这个特殊的指令。从它的名字我们就能够猜出它的用途，
它使得进程陷入到内核态中，执行内核中的相应函数，完成相应的功能。在系统调用返回后，用户空间的相关函数会将系统调用的结果，
通过一系列的过程，最终返回给用户程序。

因此，系统调用实际上是操作系统和用户空间的一组接口。用户空间的程序通过系统调用来访问操作系统的一些服务，
谋求操作系统提供必要的帮助。

在进行了上面的一系列分析后，我们将发现列出来，整理一下思路：

-   存在一些只能由操作系统来完成的操作（如读写设备、创建进程等）。

-   用户程序要实现一些功能（比如执行另一个程序、读写文件），必须依赖操作系统的帮助。

-   C标准库中的一些函数的实现必须依赖于操作系统（如我们所探究的puts函数）。

-   通过执行syscall指令，我们可以陷入到内核态，请求操作系统的一些服务。

-   直接使用操作系统的功能是很繁复的（每次都需要设置必要的寄存器，并执行syscall指令）

之后，再来整理一下调用C标准库中的puts函数的过程：

1.  调用puts函数

2.  在一系列的函数调用后，最终调用了write函数。

3.  write函数为寄存器设置了相应的值，并执行了syscall指令。

4.  进入内核态，内核中相应的函数或服务被执行。

5.  回到用户态的write函数中，将系统调用的结果从相关的寄存器中取回，并返回。

6.  再次经过一系列的返回过程后，回到了puts函数中。

7.  puts函数返回。

综合上面这些内容，实际上操作系统将自己所能够提供的服务以系统调用的方式提供给用户进程。
用户进程通过操作系统来完成一些特殊的操作。同时，所有的特殊操作就全部在操作系统的掌控之中
（因为用户进程只能通过由操作系统提供的系统调用来完成这些操作，所以操作系统可以确保用户进程不破坏系统的安全）。
而直接使用这些系统调用较为麻烦，于是由产生了用户空间的一系列API，如POSIX、C标准库等，它们在系统调用的基础上，
实现更多更高级的常用功能，使得用户在编写程序时不用再处理这些繁琐而复杂的底层操作，
而是直接通过调用高层次的API就能实现各种功能。通过这样巧妙的层次划分，使得程序更为灵活，也具有了更好的可移植性。
对于用户程序来说，只要自己所依赖的API不变，无论底层的系统调用如何变化，都不会对自己造成影响，
使得程序更易于在不同的系统间移植。整个结构如表[1.1](#fig:api-and-syscall){reference-type="ref"
reference="fig:api-and-syscall"}所示。

::: {#fig:api-and-syscall}
  用户程序 User Program   
  ----------------------- ---------------------------------
  应用程序编程接口 API      POSIX, C Standard Library, etc.
  系统调用                          read, write, fork, etc.

  : API、系统调用层次结构
:::

MOS操作系统整个系统调用的流程，大致可以如下图所示：

![syscall过程流程图](4_syscall_process){#fig:4_syscall_process
height="15cm"}

### 系统调用机制的实现

在发现了系统调用的本质之后，就要着手在MOS操作系统中实现一套系统调用机制了。为了使得后面的思路更清晰，
我们先来整理一下系统调用的流程：

1.  调用一个封装好的用户空间的库函数（如writef）

2.  调用用户空间的syscall\_\*函数

3.  调用msyscall，用于陷入内核态

4.  陷入内核，内核取得信息，执行对应的内核空间的系统调用函数（sys\_\*）

5.  执行系统调用，并返回用户态，同时将返回值"传递"回用户态

6.  从库函数返回，回到用户程序调用处

在用户空间的程序中，我们定义了许多的函数，以writef函数为例，这一函数实际上并不是最接近内核的函数，它最后会调用一个名为syscall_putchar
的函数，这个函数在user/syscall_lib.c中。在MOS操作系统实验中，这些syscall开头的函数与内核中的系统调用函数（sys开头的函数）是一一对应的，
syscall开头的函数是我们在用户空间中最接近的内核的也是最原子的函数，而sys开头的函数是内核中系统调用具体内容。
syscall开头的函数的实现中，它们毫无例外都调用了msyscall函数，而且函数的第一个参数都是一个与调用名相似的宏（如SYS_putchar），
在MOS操作系统实验中把这个参数称为**系统调用号**（请找到这个宏的定义，了解系统调用号的排布规则），系统调用号是
操作系统内核区分系统调用的唯一依据。除此之外msyscall函数还有5个参数，这些参数是系统调用实际需要使用的参数，而为了方便我们使用了
取了最多参数的系统调用所需要的参数数量（syscall_mem_map函数具有5个参数）。

在syscall\_\*系列函数中，我们将参数传递给了msyscall函数，而这些参数究竟是如何放置的呢？这里就需要了解MIPS的调用规范。
我们把函数体中没有函数调用语句的函数称为**叶函数**，自然如果有函数调用语句的函数称为非叶函数。在MIPS的调用规范中，进入函数体时会通过对栈指针做减法的方式
为自身的局部变量、返回地址、调用函数的参数分配存储空间（叶函数没有后两者）。在函数调用结束之后会对栈指针做加法来释放这部分空间，我们把
这部分空间称为**栈帧（Stack
Frame）**。非叶函数是在调用方的栈帧的底部预留被调用函数的参数存储空间（被调用方从调用方函数的栈帧中取得参数）。
以MOS操作系统为例，msyscall函数一共有6个参数，前4个参数会被syscall开头的函数分别存入\$a0-\$a3寄存器（寄存器传参的部分）同时栈帧底部保留16字节的空间（不要求存入参数的值），
后2个参数只会被存入在前4的参数的预留空间之上的8字节空间内（没有寄存器传参）。这些过程虽然不需要显式地编写汇编来完成，但是需要在内核中是以汇编的方式显式地把函数的参数值"转移"到内核空间中。

既然参数的位置已经被合理安置，那么接下来我们需要编写msyscall函数，这个叶函数没有局部变量，也就是说这个函数不需要分配栈帧，**只**需要执行
特权指令（syscall）来陷入内核态以及函数调用返回即可。

::: exercise
[]{#exercise-msyscall label="exercise-msyscall"}
填写user/syscall_wrap.S中的msyscall函数，使得用户部分的系统调用机制可以正常工作。
:::

在通过特权指令syscall陷入内核态后，处理器将PC寄存器指向一个相同的内核异常入口。在
trap_init函数中将系统调用类型的异常的入口设置为了handle_sys函数，这一函数在lib/syscall.S中。需要注意的是，此处的栈指针是内核空间的栈指针，内核将运行现场
保存到内核空间后（其保存的结构与结构体`struct Trapframe`{.c}等同），栈指针指向这个结构体的起始位置，你可以借助include/trap.h的宏使用lw指令取得保存现场的一些寄存器的值。

::: codeBoxWithCaption
内核的系统调用处理程序[]{#code:handlesys.S label="code:handlesys.S"}

``` {.gas linenos=""}
NESTED(handle_sys,TF_SIZE, sp)
    SAVE_ALL                            /* 用于保存所有寄存器的汇编宏 */
    CLI                                 /* 用于屏蔽中断位的设置的汇编宏 */
    nop
    .set at                             /* 恢复$at寄存器的使用 */

    /* TODO: 将Trapframe的EPC寄存器取出，计算一个合理的值存回Trapframe中 */

    /* TODO: 将系统调用号“复制”入寄存器$a0 */
    
    addiu   a0, a0, -__SYSCALL_BASE     /* a0 <- “相对”系统调用号 */
    sll     t0, a0, 2                   /* t0 <- 相对系统调用号 * 4 */
    la      t1, sys_call_table          /* t1 <- 系统调用函数的入口表基地址 */
    addu    t1, t1, t0                  /* t1 <- 特定系统调用函数入口表项地址 */
    lw      t2, 0(t1)                   /* t2 <- 特定系统调用函数入口函数地址 */

    lw      t0, TF_REG29(sp)            /* t0 <- 用户态的栈指针 */
    lw      t3, 16(t0)                  /* t3 <- msyscall的第5个参数 */
    lw      t4, 20(t0)                  /* t4 <- msyscall的第6个参数 */

    /* TODO: 在当前栈指针分配6个参数的存储空间，并将6个参数安置到期望的位置 */
    
    
    jalr    t2                          /* 调用sys_*函数 */
    nop
    
    /* TODO: 恢复栈指针到分配前的状态 */
    
    sw      v0, TF_REG2(sp)             /* 将$v0中的sys_*函数返回值存入Trapframe */

    j       ret_from_exception          /* 从异常中返回（恢复现场） */
    nop
END(handle_sys)

sys_call_table:                         /* 系统调用函数的入口表 */
.align 2
    .word sys_putchar
    .word sys_getenvid
    .word sys_yield
    .word sys_env_destroy
    .word sys_set_pgfault_handler
    .word sys_mem_alloc
    .word sys_mem_map
    .word sys_mem_unmap
    .word sys_env_alloc
    .word sys_set_env_status
    .word sys_set_trapframe
    .word sys_panic
    .word sys_ipc_can_send
    .word sys_ipc_recv
    .word sys_cgetc
    /* 每一个整字都将初值设定为对应sys_*函数的地址 */
    /* 在此处增加内核系统调用的入口地址 */
```
:::

::: thinking
[]{#think-syscall label="think-syscall"} 思考并回答下面的问题：

-   内核在保存现场的时候是如何避免破坏通用寄存器的？

-   系统陷入内核调用后可以直接从当时的\$a0-\$a3参数寄存器中得到用户调用msyscall留下的信息吗？

-   我们是怎么做到让sys开头的函数"认为"我们提供了和用户调用msyscall时同样的参数的？

-   内核处理系统调用的过程对Trapframe做了哪些更改？这种修改对应的用户态的变化是？
:::

::: exercise
[]{#exercise-handle-sys label="exercise-handle-sys"}
按照lib/syscall.S中的提示，完成handle_sys函数，使得内核部分的系统调用机制可以正常工作。
:::

做完这一步，整个系统调用的机制已经可以正常工作，接下来我们要来实现几个具体的系统调用。

### 基础系统调用函数

在系统调用机制完成之后，我们可以实现几个系统调用。实现哪些系统调用呢？打开
lib/syscall_all.c，可以看到有不少的系统调用函数等着大家去填写。这些系统调用都是比较基础的系统调用，不论是之后的IPC还是fork，都需要这些基础的系统调用作为支撑。
首先我们看看sys_mem_alloc。这个函数的主要功能是分配内存，简单的说，用户程序可以通过这个系统调用给该程序所允许的虚拟内存空间内存**显式地**分配实际的物理内存。可能用到的函数：page_alloc，page_insert。

::: exercise
[]{#exercise-sys-mem-alloc label="exercise-sys-mem-alloc"}
实现lib/syscall_all.c中的int sys_mem_alloc(int sysno,u_int envid, u_int
va, u_int perm)函数
:::

再来看sys_mem_map，这个函数的参数很多，但是意义也很直接：将源进程地址空间中的相应内存映射到目标进程的相应地址空间的相应虚拟内存中去。换句话说，此时两者共享着一页物理内存。可能用到的函数：page_alloc，page_insert，page_lookup。

::: exercise
[]{#exercise-sys-mem-map label="exercise-sys-mem-map"}
实现lib/syscall_all.c中的int sys_mem_map(int sysno,u_int srcid, u_int
srcva, u_int dstid, u_dstva, u_int perm)函数
:::

关于内存，还有一个函数：sys_mem_unmap,
正如字面意义所显示的，该系统调用的功能是解除某个进程地址空间虚拟内存和物理内存之间的映射关系。可能用到的函数：page_remove。

::: exercise
[]{#exercise-sys-mem-unmap label="exercise-sys-mem-unmap"}
实现lib/syscall_all.c中的int sys_mem_unmap(int sysno,u_int envid, u_int
va)函数
:::

除了与内存相关的函数外，另外一个常用的系统调用函数是sys_yield，这个函数的功能主要就在于实现用户进程对CPU的放弃，可以利用我们之前已经编写好的函数。
另外为了利用之前编写的进程切换机制保存现场，这里需要在KERNEL_SP和TIMESTACK上做一点准备工作，可以回顾lab3的部分内容。

::: exercise
[]{#exercise-sys-yield label="exercise-sys-yield"}
实现lib/syscall_all.c中的void sys_yield(void)函数
:::

可能大家也注意到了，在此我们的系统调用函数并没使用到它的第一个参数sysno。在这里，sysno作为系统调用号被传入，现在起的更多是一个"占位"的作用，和之前用户层面的系统调用函数参数顺序相匹配。

## 进程间通信机制(IPC)

进程间通信机制(IPC)是微内核最重要的机制之一。

::: note
上世纪末，微内核设计逐渐成为了一个热点。微内核设计主张将传统操作系统中的设备驱动、文件系统等可在用户空间实现的功能，
移出内核，作为普通的用户进程来实现。这样，即使它们崩溃，也不会影响到整个系统的稳定。其他应用程序通过进程间通信来请求
文件系统等相关服务。因此，在微内核中IPC是一个十分重要的机制。
:::

接下来进入正题，IPC机制远远没有我们想象得那样复杂，特别是在这个被极度简化了的MOS操作系统中。
根据之前的讨论，需要了解这样几个细节:

-   IPC的目的是使两个进程之间可以通信

-   IPC需要通过系统调用来实现

IPC的大致流程如图 [1.2](#fig:ipc-syscall){reference-type="ref"
reference="fig:ipc-syscall"} 所示。

![IPC流程图](lab4-ipc-syscall-upscale){#fig:ipc-syscall height="14cm"}

通信，最直观的一种理解就是交换数据。假如我们能够将让一个进程有能力将数据传递给另一个进程，
那么进程之间自然具有了相互通信的能力。那么，要实现交换数据，面临的最大的问题是什么呢？
就在于**各个进程的地址空间是相互独立的**。在实现内存管理的时候大家已经深刻体会到了这一点，
每个进程都有各自的地址空间，这些地址空间之间是相互独立的。因此，要想传递数据，
就需要想办法**把一个地址空间中的东西传给另一个地址空间**。

想要让两个完全独立的地址空间之间发生联系，最好的方式是什么呢？我们要去找一找它们是否存在共享的部分。
虽然地址空间本身独立，但是有些地址也许被映射到了同一物理内存上。通过分析进程的页表建立的部分，
我们可以找到线索，就在env_setup_vm()这个函数里面。

``` {.c linenos=""}
static int
env_setup_vm(struct Env *e)
{
    //略去的无关代码

    for (i = PDX(UTOP); i <= PDX(~0); i++) {
        pgdir[i] = boot_pgdir[i];
    }
    e->env_pgdir = pgdir;
    e->env_cr3   = PADDR(pgdir);

    //略去的无关代码
}
```

通过分析这个函数，可以发现，所有的进程都共享了内核所在的2G空间。对于任意的进程，这2G都是一样的。
因此，想要在不同空间之间交换数据，就可以借助于内核的空间来实现。那么，我们把要传递的消息放在哪里比较好呢？
发送和接受消息和进程有关，消息都是由一个进程发送给另一个进程。内核里什么地方和进程最相关呢？------进程控制块！

``` {.c linenos=""}
struct Env {
    // Lab 4 IPC
    u_int env_ipc_value;            // data value sent to us
    u_int env_ipc_from;             // envid of the sender
    u_int env_ipc_recving;          // env is blocked receiving
    u_int env_ipc_dstva;        // va at which to map received page
    u_int env_ipc_perm;     // perm of page mapping received
};
```

在进程控制块中我们看到了需要的数据结构，env_ipc_value用于存放需要发给当前进程的数据。
env_ipc_dstva则说明了接收到的页需要被映射到哪个虚地址上。知道了这些，我们就不难实现IPC机制了。请结合下方讲解完成练习。

::: exercise
[]{#exercise-ipc label="exercise-ipc"} 实现lib/syscall_all.c中的void
sys_ipc_recv(int sysno,u_int dstva)函数和 int sys_ipc_can_send(int
sysno,u_int envid, u_int value, u_int srcva, u_int perm)函数。
:::

void sys_ipc_recv(int sysno,u_int
dstva)函数首先要将env_ipc_recving设置为1，表明该进程准备接受其它进程的消息了。
之后修改env_ipc_dstva，接着阻塞当前进程，即把当前进程的状态置为不可运行（ENV_NOT_RUNNABLE），之后放弃CPU（调用相关函数重新进行调度）。

int sys_ipc_can_send(int sysno,u_int envid, u_int value, u_int srcva,
u_int perm)函数用于发送消息。
根据envid找到相应进程，如果指定进程为可接收状态(考虑env_ipc
\_recving)，则发送成功，之后清除接收进程的接收状态，
修改进程控制块中相应域的值，使其可运行(ENV_RUNNABLE)，函数返回0。否则，函数返回_E_IPC_NOT_RECV。
值得一提的是，由于在我们的用户进程中，会大量使用srcva为0的调用来表示不需要传递物理页面，因此在编写相关函数时也需要注意此种情况。

实现IPC后大家可以参照编写用户进程，利用实现好的IPC系统调用来实现一些有意思的小程序。

## FORK

在lab3我们曾提到过,env_alloc是内核产生一个进程。但如果想让一个进程创建一个进程，
就像是父亲与儿子那样，就需要使用到fork了。那么fork究竟是什么呢？

### 初窥fork

fork，直观意象是叉子的意思。在这里更像是分叉的意思，就好像一条河流动着，遇到一个分叉口，分成两条河一样，
fork就是那个分叉口。在操作系统中，在某个进程中调用fork()之后，将会以此为分叉分成两个进程运行。
新的进程在开始运行时有着和旧进程**绝大部分相同的信息**，而且在新的进程中fork依旧有
一个返回值，只是该返回值为0。在旧进程，也就是所谓的父进程中，fork的返回值是子进程的env_id，是大于0的。
在父子进程中有不同的返回值的特性，可以让我们在使用fork后很好地区分父子进程，从而安排不同的工作。

你可能会想，fork执行完为什么不直接生成一个空白的进程块，生成一个几乎和父进程一模一样的子进程有什么用呢？
换成创建一个空白的进程多简单！这是因为：

-   与不相干的两个进程相比，父子进程间的通信要方便的多。因为fork虽然没法造成进程间的统治关系[^3]，
    但是因为在子进程中记录了父进程的一些信息，父进程也可以很方便地对子进程进行一些管理等。

-   当然还有一个可能的原因在于安全与稳定，尤其是关于操作权限方面。对这方面有兴趣的同学可以查看链接[^4]
    探索一下。

fork之后父子进程就分道扬镳，互相独立了。如果子进程想执行一个新的程序，则需要调用名为exec系列的系统调用。在子进程中执行exec系统调用后，子进程从父进程那拷贝来的东西就全部被覆盖了。取而代之的是一个全新的进程，有着全新的代码段、数据段等内容。exec系列系统调用我们将会作为一个挑战性任务放在后面来实现，暂时不做过多介绍。

下面我们来做一个小实验，认识一下实际的fork。把下面代码复制到Linux环境下运行一下。

::: codeBoxWithCaption
理解fork[]{#code:fork_test.c label="code:fork_test.c"}

``` {.c linenos=""}
#include<stdio.h>
#include<sys/types.h>
#include<unistd.h>

int main(void){
	int var;
	pid_t pid;
	printf("Before fork.\n");
	pid = fork();
	printf("After fork.\n");
	if(pid==0){
		printf("son.");
	}else{
		sleep(2);
		printf("father.");
	}
	printf("pid:%d\n",getpid());
	return 0;
}
```
:::

使用`gcc fork_test.c`{.bash}，然后` ./a.out`{.bash}
运行一下，你得到的正常的结果应该如下所示：

``` {.console linenos=""}
Before fork.
After fork.
After fork.
son.pid:16903 (数字不一定一样)
father.pid:16902
```

我们从这段简短的代码里可以获取到很多的信息，比如以下几点：

-   在fork之前的代码段只有父进程会执行。

-   在fork之后的代码段父子进程都会执行[]{#fork与子进程
    label="fork与子进程"}。

-   fork在不同的进程中返回值不一样，在父进程中返回值不为0，在子进程中返回值为0。

-   父进程和子进程虽然很多信息相同，但他们的env_id是不同的。

从上面的小实验也能看出来------子进程实际上就是按父进程的绝大多数信息和状态作为模板而复制出来的。
即使是以父进程为模板，父子进程也还是有很多不同的地方，不知大家从刚才的小实验中能否看出父子进程有哪些地方是明显不一样的吗？

::: thinking
[]{#think-father-son label="think-father-son"}
思考下面的问题，并对这两个问题谈谈你的理解：

-   子进程完全按照fork()之后父进程的代码执行，说明了什么？

-   但是子进程却没有执行fork()之前父进程的代码，又说明了什么？
:::

::: thinking
[]{#think-fork的调用 label="think-fork的调用"}
关于fork函数的两个返回值，下面说法正确的是：

A、fork在父进程中被调用两次，产生两个返回值

B、fork在两个进程中分别被调用一次，产生两个不同的返回值

C、fork只在父进程中被调用了一次，在两个进程中各产生一个返回值

D、fork只在子进程中被调用了一次，在两个进程中各产生一个返回值
:::

首先，我们简要概括一下整个fork实现过程可能需要阅读、实现的代码内容可能有：

-   **lib/syscall_all.c**：sys_env_alloc函数，sys_set_env_status函数，sys_set_pgfault_handler函数是我们这次需要完成的函数。

-   **lib/traps.c**：page_fault_handler函数负责完成写时复制处理前的相关设置，也是我们这次需要完成的函数。

-   **user/fork.c**：fork函数是我们这次作业的重点函数，我们将分多个步骤来完成这个函数。

-   **user/fork.c**：pgfault函数是写时复制处理的函数，也是page_fault_handler后续会调用到的函数，负责对PTE_COW标志的页面进行处理，是我们这次需要完成的主要函数之一。

-   **user/fork.c**：duppage函数是父进程对子进程页面空间进行映射以及相关标志设置的函数，是我们这次需要完成的主要函数之一。

-   **user/pgfault.c**：set_pgfault_handler函数是父进程为子进程设置缺页处理函数的函数，是我们这次需要了解的函数之一。

-   **user/entry.S**：用户进程的入口，里面实现了\_\_asm_pgfault_handler函数，也是page_fault_handler后续会调用到的函数，是我们这次需要了解的函数之一。

-   **lib/genex.S**：该文件实现了定义了我们MOS中断处理的流程，虽然不是我们本次实验的重点，但是建议课下多多阅读，理解中断处理的流程。

对于MOS操作系统的fork函数流程，大致为图[1.3](#fig:4_fork_process){reference-type="ref"
reference="fig:4_fork_process"}所展示的流程。对于流程图中的各个函数，也是大家这次要完成的大部分函数，我们后续会慢慢做介绍。

![fork过程流程图](4_fork_process){#fig:4_fork_process height="22cm"}

### 写时复制机制

通过使用初步了解fork后，先不着急实现它。我们先来了解一下关于fork的底层细节。
根据下面Wiki的定义，在fork时，父进程会为子进程分配独立的地址空间。当然我们给父子进程分配不同的物理内存，但是子进程创建后通常会调用exec系统调用，覆盖从父进程复制过来的内容。这样分配物理内存的操作就是在浪费时间和空间。因此，可以采用一种优化方法：分配独立的虚拟空间并不一定会分配额外的物理内存，父子进程可以使用相同的物理内存。子进程的代码段、数据段、堆栈
都是指向父进程的物理内存，也就是说，虽然两者的虚拟空间不同，但是他们所对应的物理内存是同一个。

::: note
Wiki Fork: In Unix systems equipped with virtual memory support
(practically all modern variants), the fork operation creates a separate
address space for the child. The child process has an exact copy of all
the memory segments of the parent process, though if copy-on-write
semantics are implemented,the physical memory need not be actually
copied. Instead, virtual memory pages in both processes may refer to the
same pages of physical memory until one of them writes to such a page:
then it is copied. This optimization is important in the common case
where fork is used in conjunction with exec to execute a new program:
typically, the child process performs only a small set of actions before
it ceases execution of its program in favour of the program to be
started, and it requires very few, if any, of its parent's data
structures.
:::

这可能有个问题：既然上文提到了父子进程之间是独立的，而现在又说共享物理内存，这不是矛盾吗？

这两种说法实际上不矛盾，因为父子进程共享物理内存是有前提条件的：共享的物理内存不会被任一进程修改。那么，对于那些父进程或子进程修改的内存，我们又该如何处理呢？
这里需要引入一个新的概念------写时复制（Copy On
Write，简称COW)。通俗来讲就是当父子进程中有**修改**内存（一般是数据段）的行为发生时，内核捕获这种缺页中断后，再为**发生内存修改的进程**相应的地址分配物理页面，而一般来说子进程的代码段继续共享父进程的物理空间（两者的代码完全相同）。

::: note
如果在fork之后在子进程中执行了exec，由于这时和父进程要执行的代码完全不同，子进程的代码段也会分配单独的物理空间。
:::

在MOS操作系统实验中，对于所有的可被写入的内存页面，都需要**通过设置页表项标识位PTE_COW的方式**被保护起来。
无论父进程还是子进程何时试图写一个被保护的物理页，就会产生一个异常（一般指缺页中断
Page Fault），这一异常的处理会在后文详细介绍。

::: note
早期的Unix系统对于fork采取的策略是：直接把父进程所有的资源复制给新创建的进程。
这种实现过于简单，并且效率非常低。因为它拷贝的内存也许是可以父子进程共享的，
当然更糟的情况是，如果新进程打算通过exec执行一个新的映像，那么所有的拷贝都将前功尽弃。
:::

### 返回值的秘密

在MOS操作系统实验中，需要强调的一点是我们实现的fork是一个用户态函数，fork函数中需要若干个"原子的"系统调用来完成所期望的功能。其中最核心的一个系统调用就是一个新的进程的创建syscall_env_alloc。

在fork的实现中，我们是通过判断syscall_env_alloc的返回值来决定fork的返回值以及后续动作，所以会有类似这样结构的代码片段：

``` {.c linenos=""}
 envid = syscall_env_alloc();
 if (envid == 0) {
     // 子进程
     ...
 }
 else {
     // 父进程
     ...
 }
```

既然fork的目的是使得父子进程处于几乎相同的运行状态，我们可以认为它们都应该经历了同样的"恢复运行现场"的过程。在现场恢复后，进程会从同样的地方返回到fork函数中。而它们携带的函数的返回值是不同的，这也就能够在fork函数中区分两者。

为了实现这一特性，需要先实现sys_env_alloc的几个任务，它除了创建一个新的进程外，还需要用一些当前进程的信息作为模版来填充这个进程：

运行现场:

:   要复制一份当前进程的运行现场Trapframe到子进程的进程控制块中。

程序计数器:

:   子进程的程序计数器应该被设置为syscall_env_alloc返回后的地址，也就是它陷入异常地址的下一行指令的地址，**这个值已经存在于Trapframe中**。

返回值有关:

:   这个系统调用本身是需要一个返回值的（这个返回过程只会影响到父进程），对于子进程则需要对它的运行现场Trapframe进行一个修改。

进程状态:

:   我们当然不能让子进程在父进程syscall_env_alloc返回后就直接进入调度，因为这时候它还没有做好充分的准备，所以我们需要设定不能
    让它被加入调度队列。

其他信息:

:   观察Env结构体的结构，思考下还有哪些信息需要进行初始化，这些信息初始化的值应该是依赖于父类还是固定的，如果这些信息没有初始化会有什么后果（提示：env_pri）。

::: exercise
[]{#exercise-sys-env-alloc label="exercise-sys-env-alloc"}
请根据上述步骤以及代码中的注释提示，填写 lib/syscall_all.c 中的
sys_env_alloc 函数。
:::

在解决完返回值的问题之后，父与子就能够分别走上各自的旅途了。

### 父子各自的旅途

进程在很多时候（如进程通信）都是需要访问自身的进程控制块的，用户程序初次运行时会将一个` struct Env *env`{.c}
指针指向自身的进程控制块。作为子进程，它很明显具有了一个与父亲不同的进程控制块，因此在用户程序里的相关变量设置需要做些修改。具体步骤如下：

1.  在子进程第一次被调度的时候（当然这时还是在fork函数中）它需要将用户的env指针指向自身的进程控制块。

2.  通过一个系统调用来取得自己的envid，因为对于子进程而言syscall_env_alloc返回的是一个0值。

3.  根据获得的envid，获取对应的进程控制块，将env指针设置为对应的进程控制块。

做完上面步骤，当子进程醒来时，就可以从fork函数中正常返回，开始自己的旅途了。

::: exercise
[]{#exercise-fork-env-alloc label="exercise-fork-env-alloc"}
按照上述提示，填写 user/fork.c 中的fork
函数中关于sys_env_alloc的部分和"子进程"执行的部分
:::

当然只完成子进程部分，子进程还不能正常运行起来，父亲在儿子醒来之前则需要做更多的准备，而这些准备中最重要的一步是遍历进程的**大部分用户空间页**，
对于所有可以写入的页面的页表项，**在父进程和子进程都**加以PTE_COW标志位保护起来。这里需要
实现duppage函数来完成这个过程。

::: thinking
[]{#think:遍历页 label="think:遍历页"}
如果仔细阅读上述这一段话,应该可以发现,我们并不是对所有的用户空间页都使用duppage进行了保护。那么究竟哪些用户空间页可以保护，哪些不可以呢，
请结合 include/mmu.h 里的内存布局图谈谈你的看法。
:::

::: thinking
[]{#think:vpt的使用 label="think:vpt的使用"}
在遍历地址空间存取页表项时需要使用到vpd和vpt这两个"指针的指针"，请思考并回答这几个问题：

-   vpt和vpd的作用是什么？怎样使用它们？

-   从实现的角度谈一下为什么能够通过这种方式来存取进程自身页表？

-   它们是如何体现自映射设计的？

-   进程能够通过这种存取的方式来修改自己的页表项吗？
:::

在duppage函数中，唯一需要强调的一点是要对不同权限的页有着不同的处理方式，你可能会遇到这几种情况：

只读页面:

:   按照相同权限（只读）映射给子进程即可

共享页面:

:   即具有PTE_LIBRARY标记的页面，这类页面需要保持共享的可写的状态

写时复制页面:

:   即具有PTE_COW标记的页面，这类页面是上一次的fork的duppage的结果

可写页面:

:   需要给父进程和子进程的页表项都加上PTE_COW标记

::: exercise
[]{#exercise-duppage label="exercise-duppage"}
结合代码注释以及上述提示，填写 user/fork.c 中的 duppage 函数
:::

::: note
在MOS操作系统实验中实现的fork并不是一整个原子的过程，所以会出现一段时间（也就是在duppage之前的时间）
我们没有来得及为堆栈所在的页面
设置写时复制的保护机制，在这一段时间内对堆栈的修改（比如发生了其他的函数调用），则会将非叶函数syscall_env_alloc
函数调用的栈帧中的返回地址覆盖。这一问题对于父进程来说是理所当然的，然而对于子进程来说，这个覆盖导致
的后果则是在从syscall_env_alloc返回时跳转到一个不可预知的位置造成panic。当然大家现在看到的代码已经通过一个
优雅的办法来修补这个问题：与其他系统调用函数不同，syscall_env_alloc是一个内联（inline）的函数，也就是说
这个函数并不会被编译为一个函数，而是直接内联展开在fork函数内。所以syscall_env_alloc的栈帧就不存在了，而
msyscall函数的返回指令也直接返回到了fork函数内。
:::

在完成写时复制的保护机制后，还不能让子进程处于能被调度的状态，因为作为父亲进程还有其他的责任------为写时复制特性
的**缺页中断**处理做好准备。

### 缺页中断

内核在捕获到 一个常规的**缺页中断（Page
Fault）**时（在MIPS中这个情况特指TLB缺失，因为MIPS不存在MMU只存在TLB，TLB缺失查找填入都是内核以软件编
程的方式完成的），会进入到一个在trap_init中"注册"的handle_tlb的内核处理函数中，这一汇编函数的实现在lib/genex.S
中，化名为一个叫do_refill的函数。如果物理页面在页表中存在，则会将TLB填入并
返回异常地址再次执行内存存取的指令。如果物理页面不存在，则会触发一个一般意义的缺页错误，并跳转到mm/pmap.c中的
pageout函数中，在存取地址合法的情况下，内核会在用户空间的对应地址分配映射一个物理页面（被动的分配页面）来解
决缺页的问题。

前文中我们提到了写时复制特性，而写时复制特性也是依赖于缺页中断的。我们在trap_init中注册了另外一个处理函数------handle_mod，这一函数会跳转到lib/traps.c的page_fault_handler函数中，这个函数正是
处理写时复制特性的缺页中断的内核处理函数。

但在这个函数中似乎并没有做任何的页面复制操作，这是为什么呢？答案是这样的，在MOS操作系统实验中按照
微内核的设计理念，会将大部分的功能都从内核移到用户程序，其中也包括了写时复制的缺页中断处理。真正的处理过程
是用户进程自身去完成的。

如果需要用户进程去完成页面复制等处理过程，是不能直接使用原先的堆栈的（因为发生缺页错误
的也可能是正常堆栈的页面），所以这个时候用户进程就需要一个另外的堆栈来执行处理程序，我们把这个堆栈称作
**异常处理栈**，它的栈顶对应的是宏UXSTACKTOP。异常处理栈需要父进程为自身以及子进程分配映射物理页面。
此外内核还需要知晓进程自身的处理函数所在的地址，这个地址存在于进程控制块的env_pgfault_handler域中，这个地址
也需要事先由父进程通过系统调用设置。

因此，概括一下上述内容，在MOS操作系统中，完成写时复制的缺页中断处理大致流程可以概括为：

1.  用户进程触发缺页中断，识别为写时复制处理，跳转到handle_mod函数，再跳转到page_fault_handler函数。

2.  page_fault_handler函数负责将当前现场保存在异常处理栈中，并设置epc寄存器的值，使得退出中断后能够跳转到env_pgfault_handler域定义的异常处理函数。

3.  退出中断，跳转到异常处理函数中，这个函数首先跳转到pgfault函数（定义在fork.c中）进行缺页处理，之后恢复事先保存好的现场，并恢复sp寄存器的值，使得子进程恢复执行。

关于上文总是提到的env_pgfault_handler域定义的异常处理函数，在下文中会做具体介绍。

::: exercise
[]{#exercise-page-fault-handler label="exercise-page-fault-handler"}
根据上述提示以及代码注释，完成 lib/traps.c 中的 page_fault_handler
函数，设置好异常处理栈以及epc寄存器的值。
:::

::: thinking
[]{#think:pgfault-kernel label="think:pgfault-kernel"}
page_fault_handler
函数中，大家可能注意到了一个向异常处理栈复制Trapframe运行现场的过程，请思考并回答这几个问题：

-   这里实现了一个支持类似于"中断重入"的机制，而在什么时候会出现这种"中断重入"？

-   内核为什么需要将异常的现场Trapframe复制到用户空间？
:::

让我们回到fork函数，在提示使用syscall_env_alloc之前，有另一个提示------使用set_pgfault_handler函数来"安装"
处理函数。也就是上文总提到的env_pgfault_handler域定义的异常处理函数。

``` {.c linenos=""}
void set_pgfault_handler(void (*fn)(u_int va))
{
  if (__pgfault_handler == 0) {
    if (syscall_mem_alloc(0, UXSTACKTOP - BY2PG, PTE_V | PTE_R) < 0 ||
      syscall_set_pgfault_handler(0, __asm_pgfault_handler, UXSTACKTOP) < 0) {
      writef("cannot set pgfault handler\n");
      return;
    }
  }
  __pgfault_handler = fn;
}
```

上面的set_pgfault_handler函数中，进程**为自身**分配映射了异常处理栈，
同时也用系统调用告知内核自身的处理程序是\_\_asm_pgfault_handler（在entry.S定义），随后内核也需要就将进程控制块的env_pgfault_handler域设为它。在函数的最后，将在entry.S定义的字\_\_pgfault_handler赋值为fn，而这个fn究竟是什么我们稍后再说。这里需要完成内核设置进程控制块中的两个域的系统调用。

::: exercise
[]{#exercise-sys-set-pgfault-handler
label="exercise-sys-set-pgfault-handler"} 完成 lib/syscall_all.c 中的
sys_set_pgfault_handler 函数
:::

我们现在知道了缺页中断会返回到entry.S中的\_\_asm_pgfault_handler函数，再来看这个函数会
做些什么。

``` {.gas linenos=""}
__asm_pgfault_handler:
lw      a0, TF_BADVADDR(sp)
lw      t1, __pgfault_handler
jalr    t1
nop

lw      v1, TF_LO(sp)
mtlo    v1
lw      v0, TF_HI(sp)
lw      v1, TF_EPC(sp)
mthi    v0
mtc0    v1, CP0_EPC
lw      $31, TF_REG31(sp)

lw      $1, TF_REG1(sp)
lw      k0, TF_EPC(sp)
jr      k0
lw      sp, TF_REG29(sp)
```

从内核返回后，此时的栈指针是由内核设置的在异常处理栈的栈指针，而且指向一个由内核复制好的Trapframe结构体的底部。
通过宏TF_BADVADDR用lw指令取得了Trapframe中的cp0_badvaddr字段的值，这个值
也正是发生缺页中断的虚拟地址。将这个地址作为第一个参数去调用了\_\_pgfault_handler这个字内存储的函数，不难看出这个函数是真正进行处理的函数。
函数返回后就是一段类似于恢复现场的汇编，最后非常巧妙地利用了MIPS的延时槽特性跳转的同时恢复了栈指针。

::: thinking
[]{#think:pgfault-user-1 label="think:pgfault-user-1"}
到这里我们大概知道了这是一个由用户程序处理并由用户程序自身来恢复运行现场的过程，请思考并回答以下几个问题：

-   用户处理相比于在内核处理写时复制的缺页中断有什么优势？

-   从通用寄存器的用途角度讨论用户空间下进行现场的恢复是如何做到不破坏通用寄存器的？
:::

说到这里，就要来实现真正进行处理的函数：user/fork.c中的pgfault函数了，pgfault需要完成这些任务：

1.  判断页是否为写时复制的页面，是则进行下一步，否则报错

2.  分配一个新的内存页到临时位置，将要复制的内容拷贝到刚刚分配的页中（临时页面位置可以自定义，观察mmu.h的地址分配查看哪个地址没有被用到，思考这个临时位置可以定在哪）

3.  将临时位置上的内容映射到发生缺页中断的虚拟地址上，注意设定好对应的页面权限，然后解除临时位置对内存的映射

::: exercise
[]{#exercise-pgfault label="exercise-pgfault"} 填写 user/fork.c 中的
pgfault函数
:::

这里的pgfault也正是父进程在fork中使用set_pgfault_handler函数安装的处理函数。

::: thinking
[]{#think:pgfault-user-2 label="think:pgfault-user-2"}
请思考并回答以下几个问题：

-   为什么需要将set_pgfault_handler的调用放置在syscall_env_alloc之前？

-   如果放置在写时复制保护机制完成之后会有怎样的效果？

-   子进程需不需要对在entry.S定义的字\_\_pgfault_handler赋值？
:::

父进程还需要为子进程通过类似于set_pgfault_handler函数的方式，用若干系统调用分配子进程的异常处理栈以及设置
处理函数为\_\_asm_pgfault_handler。

最后父进程通过系统调用syscall_set_env_status设置子进程为可以运行的状态。在实现内核sys_set_env_status函
数时，不仅需要设置进程控制块的env_status域，还需要在status为RUNNABLE时将进程加入可以调度的链表。

::: exercise
[]{#exercise-sys-set-env-status label="exercise-sys-set-env-status"}
填写 lib/syscall_all.c 中的 sys_set_env_status 函数
:::

说到这里我们需要整理一下思路，fork中父进程在syscall_env_alloc后还需要做的事情有：

1.  遍历父进程地址空间，进行duppage。

2.  为子进程分配异常处理栈。

3.  设置子进程的处理函数，确保缺页中断可以正常执行。

4.  设置子进程的运行状态

最后再将子进程的envid返回，fork函数就大功告成了！

::: exercise
[]{#exercise-fork label="exercise-fork"} 填写 user/fork.c 中的fork
函数中关于"父进程"执行的部分
:::

至此,lab4实验已经基本完成了,接下来就一起来愉快地调试吧！

可以模仿**fktest.c**的方式构建用户进程，测试你的syscall部分

1.  首先在user目录下新建xxx.c

2.  在xxx.c中include在user目录中的lib.h

3.  在user/Makefile中的编译目标加上xxx.x和xxx.b

4.  在init/init.c中用ENV_CREATE或者ENV_CREATE_PRIORITY创建用户进程user_xxx

5.  在xxx.c文件中，你可以编写自己的测试逻辑

## 实验正确结果

本次测试分为两个文件,当基础系统调用与fork写完后,单独测试fork的文件是**user/fktest.c**,测试时将

ENV_CREATE(user_fktest)加入init.c即可测试。

正确结果如下：

``` {.text linenos=""}

    main.c: main is start ...

    init.c: mips_init() is called

    Physical memory: 65536K available, base = 65536K, extended = 0K

    to memory 80401000 for struct page directory.

    to memory 80431000 for struct Pages.

    mips_vm_init:boot_pgdir is 80400000

    pmap.c:  mips vm init success

    panic at init.c:31: ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

    pageout:    @@@___0x7f3fe000___@@@  ins a page

    this is father: a:1

    this is father: a:1

    this is father: a:1

    this is father: a:1

    this is father: a:1

    this is father: a:1

    this is father: a:1

    this is father: a:1

    this is father: a:1

      child :a:2

        this is child :a:2

        this is child :a:2

                this is child2 :a:3

                this is child2 :a:3

                this is child2 :a:3

                this is child2 :a:3

    this is father: a:1

    this is father: a:1

    this is father: a:1

    this is father: a:1

    this is father: a:1

        this is child :a:2

        this is child :a:2

        this is child :a:2
```

另一个测试文件主要测试进程间通信,文件为**user/pingpong.c**,测试方法同上。

正确结果如下：

``` {.text linenos=""}

main.c: main is start ...

init.c: mips_init() is called

Physical memory: 65536K available, base = 65536K, extended = 0K

to memory 80401000 for struct page directory.

to memory 80431000 for struct Pages.

mips_vm_init:boot_pgdir is 80400000

pmap.c:  mips vm init success

panic at init.c:31: ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

pageout:    @@@___0x7f3fe000___@@@  ins a page


@@@@@send 0 from 800 to 1001

1001 am waiting.....

800 am waiting.....

1001 got 0 from 800

@@@@@send 1 from 1001 to 800

1001 am waiting.....

800 got 1 from 1001



@@@@@send 2 from 800 to 1001

800 am waiting.....

1001 got 2 from 800



@@@@@send 3 from 1001 to 800

1001 am wa800 got 3 from 1001



@@@@@send 4 from 800 to 1001

iting.....

800 am waiting.....

1001 got 4 from 800



@@@@@send 5 from 1001 to 800

1001 am waiting.....

800 got 5 from 1001



@@@@@send 6 from 800 to 1001

800 am waiting.....

1001 got 6 from 800



@@@@@send 7 from 1001 to 800

1001 am waiting.....

800 got 7 from 1001



@@@@@send 8 from 800 to 1001

800 am waiting.....

1001 got 8 from 800



@@@@@send 9 from 1001 to 800

1001 am waiting.....

800 got 9 from 1001



@@@@@send 10 from 800 to 1001

[00000800] destroying 00000800

[00000800] free env 00000800

i am killed ...

1001 got 10 from 800

[00001001] destroying 00001001

[00001001] free env 00001001

i am killed ...
```

此外，大家还可以模仿fktest.c和pingpong.c，自己写几个fork测试进程，理解fork的执行过程。

## 任务列表

-   [**[Exercise-完成msyscall函数]{style="color: baseB"}**](#exercise-msyscall)

-   [**[Exercise-完成handle_sys函数]{style="color: baseB"}**](#exercise-handle-sys)

-   [**[Exercise-实现
    sys_mem_alloc函数]{style="color: baseB"}**](#exercise-sys-mem-alloc)

-   [**[Exercise-实现sys_mem_map函数]{style="color: baseB"}**](#exercise-sys-mem-map)

-   [**[Exercise-实现sys_mem_unmap函数]{style="color: baseB"}**](#exercise-sys-mem-unmap)

-   [**[Exercise-实现sys_yield函数]{style="color: baseB"}**](#exercise-sys-yield)

-   [**[Exercise-实现sys_ipc_recv函数和sys_ipc_can_send函数]{style="color: baseB"}**](#exercise-ipc)

-   [**[Exercise-填写 sys_env_alloc
    函数]{style="color: baseB"}**](#exercise-sys-env-alloc)

-   [**[Exercise-填写 fork
    函数中关于sys_env_alloc的部分和"子进程"执行的部分]{style="color: baseB"}**](#exercise-fork-env-alloc)

-   [**[Exercise-填写 duppage
    函数]{style="color: baseB"}**](#exercise-duppage)

-   [**[Exercise-完成 page_fault_handler
    函数]{style="color: baseB"}**](#exercise-page-fault-handler)

-   [**[Exercise-完成 sys_set_pgfault_handler
    函数]{style="color: baseB"}**](#exercise-sys-set-pgfault-handler)

-   [**[Exercise-填写 pgfault
    函数]{style="color: baseB"}**](#exercise-pgfault)

-   [**[Exercise-填写 sys_set_env_status
    函数]{style="color: baseB"}**](#exercise-sys-set-env-status)

-   [**[Exercise-填写 fork
    函数中关于"父进程"执行的部分]{style="color: baseB"}**](#exercise-fork)

## 实验思考

-   [**[思考-系统调用的实现]{style="color: baseB"}**](#think-syscall)

-   [**[思考-不同的进程代码执行]{style="color: baseB"}**](#think-father-son)

-   [**[思考-fork的返回结果]{style="color: baseB"}**](#think-fork的调用)

-   [**[思考-用户空间的保护]{style="color: baseB"}**](#think:遍历页)

-   [**[思考-vpt的使用]{style="color: baseB"}**](#think:vpt的使用)

-   [**[思考-缺页中断-内核处理]{style="color: baseB"}**](#think:pgfault-kernel)

-   [**[思考-缺页中断-用户处理-1]{style="color: baseB"}**](#think:pgfault-user-1)

-   [**[思考-缺页中断-用户处理-2]{style="color: baseB"}**](#think:pgfault-user-2)

## 挑战性任务------线程和信号量

线程（thread）和信号量（semaphore）是同步与互斥问题的重要概念。

在这个挑战任务中，我们希望你能够按照 POSIX
标准，在MOS操作系统中实现这两个机制。

### POSIX Threads

POSIX Threads 也就是 pthreads 库是由 IEEE Std 1003.1c
标准定义的一组函数接口。它至少包含以下的函数：

-   pthread_create 创建线程

-   pthread_exit 退出线程

-   pthread_cancel 撤销线程

-   pthread_join 等待线程结束

### POSIX Semaphore

POSIX Semaphore 是由 IEEE Std 1003.1b 标准定义的一组函数接口：

-   sem_init 初始化信号量

-   sem_destroy 销毁信号量

-   sem_wait 对信号量P操作（阻塞）

-   sem_trywait 对信号量P操作（非阻塞）

-   sem_post 对信号量V操作

-   sem_getvalue 取得信号量的值

### 题目要求

题目要求如下：

#### 线程

要求实现全用户地址空间共享，线程之间可以互相访问栈内数据。可以保留少量仅限于本线程可以访问的空间用以保存线程相关数据。（POSIX标准有规定相关接口也可以实现）

#### 信号量

POSIX标准的信号量分为有名和无名信号量。无名信号量可以用于进程内同步与通信，有名信号量既可以用于进程内同步与通信，也可以用于进程间同步与通信。要求至少实现无名信号量（上述函数）的全部功能。

#### 要求

学生要至少实现上述的两组函数，并实现要求的功能，请合理增加系统调用以及相应的数据结构。其他POSIX线程信号量相关接口会根据实现难度加分。

#### 提交方式

以 lab4 分支为基础，新建 lab4-challenge
分支，完成之后提交到同名的远程分支。

[^1]: 这里为了方便大家在自己的机器上重现， 我们选用了Linux
    X86_64平台作为实验平台

[^2]: 为了加快调试进程， 可以尝试stepi
    N指令，N的位置填写任意数字均可。这样每次会执行N条机器指令。

[^3]: 这是因为进程之间是并发的，在操作系统看来，父子进程之间更像是兄弟关系。

[^4]: <https://httpd.apache.org/docs/current/mod/prefork.html>
