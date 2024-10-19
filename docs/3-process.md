# 进程与异常

## 实验目的

1.  创建一个进程并成功运行

2.  实现时钟中断，通过时钟中断内核可以再次获得执行权

3.  实现进程调度，创建两个进程，并且通过时钟中断切换进程执行

在本次实验中将运行一个用户模式的进程。需要使用数据结构进程控制块 Env
来跟踪用户进程。
通过建立一个简单的用户进程，加载一个程序镜像到进程控制块中，并让它运行起来。
同时，你的MIPS内核将拥有处理异常的能力。

## 进程

::: note
进程既是基本的分配单元，也是基本的执行单元。第一，进程是一个实体。每一个进程都有它自己的地址空间，
一般情况下，包括代码段、数据段和堆栈。第二，进程是一个"执行中的程序"。
程序是一个没有生命的实体，只有处理器赋予程序生命时，它才能成为一个活动的实体，我们称其为进程。
:::

### 进程控制块

这次实验是关于进程和异常的，那么我们首先来结合代码看看进程控制块是个什么。

进程控制块(PCB)是系统为了管理进程设置的一个专门的数据结构，用它来记录进程的外部特征，描述进程的运动变化过程。
系统利用PCB来控制和管理进程，所以**PCB是系统感知进程存在的唯一标志。进程与PCB是一一对应的。**
通常PCB应包含如下一些信息：

::: codeBoxWithCaption
进程控制块[]{#code:process_of_env.h label="code:process_of_env.h"}

``` {.c linenos=""}
struct Env {
    struct Trapframe env_tf;        // Saved registers
    LIST_ENTRY(Env) env_link;       // Free LIST_ENTRY
    u_int env_id;                   // Unique environment identifier
    u_int env_parent_id;            // env_id of this env's parent
    u_int env_status;               // Status of the environment
    Pde  *env_pgdir;                // Kernel virtual address of page dir
    u_int env_cr3;
    LIST_ENTRY(Env) env_sched_link;
    u_int env_pri;
};
```
:::

为了集中注意力在关键的地方，我们暂时不介绍其他实验所需介绍的内容。
下面是**lab3**中需要用到的这些域的一些简单说明：

-   env_tf : Trapframe 结构体的定义在**include/trap.h** 中，env_tf
    的作用就是在进程因为时间片用光不再运行时，将其当时的进程上下文环境保存在env_tf
    变量中。当从用户模式切换到内核模式时，内核也会保存进程上下文，因此进程返回时上下文可以从中恢复。

-   env_link : env_link 的机制类似于lab2中的 pp_link,使用它和
    env_free_list 来构造空闲进程链表。

-   env_id : 每个进程的 env_id 都不一样，env_id 是进程独一无二的标识符。

-   env_parent_id : 该变量存储了创建本进程的进程 id。
    这样进程之间通过父子进程之间的关联可以形成一棵进程树。

-   env_status : 该变量只能在以下三个值中进行取值：

    -   ENV_FREE :
        表明该进程是不活动的，即该进程控制块处于进程空闲链表中。

    -   ENV_NOT_RUNNABLE : 表明该进程处于阻塞状态，
        处于该状态的进程往往在等待一定的条件才可以变为就绪状态从而被CPU调度。

    -   ENV_RUNNABLE : 表明该进程处于就绪状态，正在等待被调度，
        但处于RUNNABLE状态的进程可以是正在运行的，也可能不在运行中。

-   env_pgdir : 这个变量保存了该进程页目录的内核虚拟地址。

-   env_cr3 : 这个变量保存了该进程页目录的物理地址。

-   env_sched_link : 这个变量来构造就绪状态进程链表。

-   env_pri : 这个变量保存了该进程的优先级。

这里值得强调的一点就是**ENV_RUNNABLE
状态并不代表进程一定在运行中，它也可能正处于调度队列中**。
而我们的进程调度也只会调度处于RUNNABLE状态的进程。

既然知道了进程控制块，我们就来认识一下进程控制块数组**envs**。在我们的实验中，
存放进程控制块的物理内存在系统启动后就要被分配好，并且这块内存不可被换出。
所以需要在系统启动后就要为envs数组分配内存，下面就要完成这个重任。

::: exercise
[]{#exercise-mips-vm-init label="exercise-mips-vm-init"}

-   修改pmap.c/mips_vm_init函数来为envs数组分配空间。

-   envs数组包含 NENV
    个Env结构体成员，可以参考pmap.c中已经写过的**pages**数组空间的分配方式。

-   除了要为数组 envs 分配空间外，还需要使用 pmap.c
    中以前实验曾填写过的一个内核态函数为其进行段映射， envs
    数组应该被**UENVS**区域映射，你可以参考**./include/mmu.h**。

注：本次实验**不需要**填写这个部分，但是请仔细阅读相关代码。
:::

当然光有了存储进程控制块信息的envs还不够，我们还需要像lab2一样将空闲的env控制块按照链表形式"串"起来，便于后续分配ENV结构体对象，形成env_free_list。一开始所有进程控制块都是空闲的，所以要把它们都"串"到env_free_list上去。

::: exercise
[]{#exercise-env-init label="exercise-env-init"}
仔细阅读注释，填写**env_init**函数，注意链表插入的顺序(函数位于lib/env.c中)。
:::

在填写完env_init函数后，对于envs的操作暂时就告一段落了，不过还有一个小问题没解决，为什么严格规定链表的插入顺序？需要仔细思考，注释中已经给出了提示。

::: thinking
[]{#think-env_init label="think-env_init"}
为什么我们在构造空闲进程链表时必须使用特定的插入的顺序？(顺序或者逆序)
:::

### 进程的标识

在计算机系统中经常有很多进程同时存在，每个进程执行不同的任务，它们之间也经常需要相互协作、通信，操作系统是如何识别每个进程呢？

在上一部分中，我们了解到每个进程的信息存储在该进程对应的进程控制块中，同学们可能已经发现struct
Env中的env_id这个域，它就是每个进程独一无二的标识符。我们在创建每个新的进程的时候必须为它赋予一个与众不同的id来作为它的标识符。

可以在env.c文件中找到一个叫做mkenvid的函数，它的作用就是生成一个新的进程id。

如果我们知道一个进程的id，那么如何才能找到该id对应的进程控制块呢？

::: exercise
[]{#exercise-envid2env label="exercise-envid2env"}
仔细阅读注释，完成env.c/envid2env函数，实现通过一个env的id获取该id对应的进程控制块的功能。
:::

::: thinking
[]{#think-mkenvid label="think-mkenvid"}
思考env.c/mkenvid函数和envid2env函数:

-   请谈谈对mkenvid函数中生成id的运算的理解，为什么这么做？

-   为什么envid2env中需要判断e-\>env_id != envid
    的情况？如果没有这步判断会发生什么情况？
:::

### 设置进程控制块

做完上面练习后，那么我们就可以开始利用**空闲进程链表env_free_list**创建进程。下面我们就来具体讲讲应该如何创建一个进程[^1]。

进程创建的流程如下：

第一步

:   申请一个空闲的PCB，从env_free_list中索取一个空闲PCB块，这时候的PCB就像张白纸一样。

第二步

:   "纯手工打造"打造一个进程。在这种创建方式下，由于没有模板进程,所以进程拥有的所有信息都是手工设置的。
    而进程的信息又都存放于进程控制块中，所以我们需要手工初始化进程控制块。

第三步

:   进程光有PCB的信息还没法跑起来，每个进程都有独立的地址空间。[]{#process-3
    label="process-3"}
    所以，要为新进程分配资源，为新进程的程序和数据以及用户栈分配必要的内存空间。

第四步

:   此时PCB已经被涂涂画画了很多东西，不再是一张白纸，把它从空闲链表里除名,就可以投入使用了。

当然，第二步的信息设置是本次实验的关键，那么下面让我们来结合注释看看这段代码

::: codeBoxWithCaption
进程创建[]{#code:env_alloc.c label="code:env_alloc.c"}

``` {.c linenos=""}
/* Overview:
 *  Allocates and Initializes a new environment.
 *  On success, the new environment is stored in *new.
 *
 * Pre-Condition:
 *  If the new Env doesn't have parent, parent_id should be zero.
 *  env_init has been called before this function.
 *
 * Post-Condition:
 *  return 0 on success, and set appropriate values for Env new.
 *  return -E_NO_FREE_ENV on error, if no free env.
 *
 * Hints:
 *  You may use these functions and defines:
 *      LIST_FIRST,LIST_REMOVE,mkenvid (Not All)
 *  You should set some states of Env:
 *      id , status , the sp register, CPU status , parent_id
 *      (the value of PC should NOT be set in env_alloc)
 */

int
env_alloc(struct Env **new, u_int parent_id)
{
	int r;
	struct Env *e;
    
    /*Step 1: Get a new Env from env_free_list*/

    
    /*Step 2: Call certain function(has been implemented) to init kernel memory
     * layout for this new Env.
     *The function mainly maps the kernel address to this new Env address. */


    /*Step 3: Initialize every field of new Env with appropriate values*/


    /*Step 4: focus on initializing env_tf structure, located at this new Env. 
     * especially the sp register,CPU status. */
    e->env_tf.cp0_status = 0x10001004;


    /*Step 5: Remove the new Env from Env free list*/


}
```
:::

相信你一眼看到第三条注释的时候一定会问："什么叫合适的值啊？"。先别着急，请花半分钟的时间看一下第二条注释。

env.c中的env_setup_vm函数就是你在第二部中要使用的函数，该函数的作用是初始化新进程的地址空间。在使用该函数之前，必须先完成这个函数，这部分任务是这次实验的难点之一。

::: codeBoxWithCaption
地址空间初始化[]{#code:env_setup_vm.c label="code:env_setup_vm.c"}

``` {.c linenos=""}
/* Overview:
 *  Initialize the kernel virtual memory layout for 'e'.
 *  Allocate a page directory, set e->env_pgdir and e->env_cr3 accordingly,
 *  and initialize the kernel portion of the new env's address space.
 *  DO NOT map anything into the user portion of the env's virtual address space.
 */
/*** exercise 3.4 ***/
static int
env_setup_vm(struct Env *e)
{

    int i, r;
    struct Page *p = NULL;
    Pde *pgdir;

    /* Step 1: Allocate a page for the page directory 
     * using a function you completed in the lab2 and add its pp_ref.
     * pgdir is the page directory of Env e, assign value for it. */
    if () {
        panic("env_setup_vm - page alloc error\n");
        return r;
    }

    /*Step 2: Zero pgdir's field before UTOP. */

    /*Step 3: Copy kernel's boot_pgdir to pgdir. */

    /* Hint:
     *  The VA space of all envs is identical above UTOP
     *  (except at UVPT, which we've set below).
     *  See ./include/mmu.h for layout.
     *  Can you use boot_pgdir as a template?
     */

    /*Step 4: Set e->env_pgdir and e->env_cr3 accordingly. */

    /* UVPT map the env's own page table, with read-only permission. */
    e->env_pgdir[PDX(UVPT)]  = e->env_cr3 | PTE_V;
    return 0;
}
```
:::

在动手开始完成env_setup_vm之前，为了便于理解你需要在这个函数中所做的事情，请先阅读以下提示:

在我们的实验中，对于不同的进程而言，虚拟地址ULIM以上的地方，虚拟地址到物理地址的映射关系都是一样的。因为这2G虚拟空间，不是由哪个进程管理的，而是由内核管理！你仔细思索这句话,可能会产生疑惑："那为什么不能该内核管的时候让内核进程去管，该普通进程管的时候给普通进程去管,非要混在一个地址空间里呢？"这种想法是很好的，不同的操作系统会有不同的进程地址空间的设计方式，在我们的MOS中采用了这种混合的设计方式。

这里我们介绍几个概念，大家可能会更清楚地理解这种方式。

首先来介绍下虚拟空间的分配模式。我们知道，每一个进程都有4G的逻辑地址可以访问，
我们所熟知的系统不管是Linux还是Windows系统，都可以支持3G/1G模式或者2G/2G模式。
3G/1G模式即满32位的进程地址空间中，用户态占3G，内核态占1G。这些情况在进入内核态的时候叫做陷入内核，
因为**即使进入了内核态，还处在同一个地址空间中，并不切换CR3寄存器。**
但是！还有一种模式是4G/4G模式，内核单独占有一个4G的地址空间，所有的用户进程独享自己的4G地址空间，
这种模式下，在进入内核态的时候，叫做切换到内核，**因为需要切换CR3寄存器**，
所以进入了**不同的地址空间**！

::: note
用户态和内核态的概念大家已经了解了，内核态即计算机系统的特权态，用户态就是非特权态。
mips汇编中使用一些特权指令如`mtc0`{.gas}、`mfc0`{.gas}、`syscall`{.gas}等都会陷入特权态（内核态)。
:::

而我们这次实验，根据`./include/mmu.h`{.c}里面的布局来说，我们是2G/2G模式，
用户态占用2G，内核态占用2G。接下来，也是最容易产生混淆的地方，进程从用户态提升到内核态的过程，
操作系统中发生了什么？

是从当前进程切换成一个所谓的"内核进程"来管理一切了吗？不！还是一样的配方，还是一样的进程！
改变的其实只是进程的权限！我们打一个比方，你所在的班级里没有班长，
任何人都可以以合理的理由到老师那申请当临时班长。你是班里的成员吗？当然是的。
某天你申请当临时班长，申请成功了，那么现在你还是班里的成员吗？当然还是的。
那么你前后有什么区别呢？是权限的变化。可能你之前和其他成员是完全平等，互不干涉的。
那么现在你可以根据花名册点名，你可以安排班里的成员做些事情，你可以开班长会议等等。
但你并不是永久的班长，只能说你拥有了班长的资格。这种**成为临时班长的资格**，可以粗略地认为它就是内核态的精髓所在。

而在操作系统中，每个完整的进程都拥有这种成为临时内核进程的资格，即所有的进程都可以发出申请变成内核态下运行的进程。
内核态下进程可访问的资源更多。在之后我们会提到一种"申请"的方式，就叫做"系统调用"。

那么大家现在应该能够理解为什么我们要将内核才能使用的虚页表为每个进程都拷贝一份了，在2G/2G这种布局模式下，
每个进程都会有**2G内核态**的虚拟地址，以便临时变身为"内核"行使大权。但是，在变身之前，
只能访问自己的那2G用户态的虚拟地址。

那么这种微妙的关系应该类似于下面这种：（图[1.1](#fig:Process){reference-type="ref"
reference="fig:Process"}灰色代表不可用，白色代表可用）

![进程页表与内核页表的关系](Process){#fig:Process width="14cm"}

现在结合注释已经可以开始动手完成env_setup_vm函数了。

::: exercise
[]{#exercise-env-setup-vm label="exercise-env-setup-vm"}
仔细阅读注释，填写env_setup_vm函数。
:::

::: thinking
[]{#think-env_setup_vm label="think-env_setup_vm"}
结合include/mmu.h中的地址空间布局，思考env_setup_vm函数：

-   我们在初始化新进程的地址空间时为什么不把整个地址空间的pgdir都清零，而是复制内核的`boot_pgdir`{.c}作为一部分模板？(提示:mips虚拟空间布局)

-   UTOP和ULIM的含义分别是什么，在UTOP到ULIM的区域与其他用户区相比有什么最大的区别？

-   在step4中我们为什么要让`pgdir[PDX(UVPT)]=env_cr3`{.c}?(提示:结合系统自映射机制)

-   谈谈自己对进程中物理地址和虚拟地址的理解。
:::

在上述的思考完成后，那么就可以直接在**env_alloc** 第二步使用该函数了。
现在来解决一下刚才的问题，第三点所说的合适的值是什么？我们要设定哪些变量的值呢？

我们要设定的变量其实在**env_alloc**
函数的提示中已经说的很清楚了，至于其合适的值，
相信大家可以从函数的前面长长的注释里获得足够的信息。当然要讲的重点不在这里，
重点在我们已经给出的这个设置`e->env_tf.cp0_status = 0x10001004;`{.c}

这个设置很重要！重要到我们直接在代码中给出。为什么说它重要，我们来仔细分析一下。

![R3000的SR寄存器示意图](3-R3000_SR){#fig:3-R3000_SR width="15cm"}

图[1.2](#fig:3-R3000_SR){reference-type="ref"
reference="fig:3-R3000_SR"}是我们MIPSR3000里的SR(status
register)寄存器示意图，就是我们在env_tf里的cp0\_ status。

第28bit设置为1，表示处于用户模式下。

第12bit设置为1，表示4号中断可以被响应。

这些都是比较容易，下面讲的是重点！

R3000的SR寄存器的低六位是一个二重栈的结构。
KUo和IEo是一组，每当中断发生的时候，硬件自动会将KUp和IEp的数值拷贝到这里；
KUp和IEp是一组，当中断发生的时候，硬件会把KUc和IEc的数值拷贝到这里。

其中KU表示是否位于内核模式下，为1表示位于内核模式下；
IE表示中断是否开启，为1表示开启，否则不开启[^2]。

而每当rfe指令调用的时候，就会进行上面操作的**逆操作**。
我们现在先不管为何,但是已经知道,下面这一段代码在**运行第一个进程前是一定要执行的**，
[]{#env_pop_tf label="env_pop_tf"}所以就一定会执行`rfe`{.gas}这条指令。

``` {.gas linenos=""}
lw   k0,TF_STATUS(k0)        #恢复CP0_STATUS寄存器
mtc0 k0,CP0_STATUS
j    k1
rfe
```

现在大家可能就懂了为何我们status后六位是设置为`000100b`{.c}了。
当运行第一个进程前，运行上述代码到`rfe`{.gas}的时候，就会将KUp和IEp拷贝回KUc和IEc，令status为
`000001b`{.c}，最后两位KUc,IEc为**\[0,1\]**，表示开启了中断。之后第一个进程成功运行，
这时操作系统也可以正常响应中断！

::: note
关于MIPSR3000版本SR寄存器功能的英文原文描述： The status register is one
of the more important registers. The register has several fields. The
current Kernel/User (KUc) flag states whether the CPU is in kernel mode.
The current Interrupt Enable (IEc) flag states whether external
interrupts are turned on. If cleared then external interrupts are
ignored until the flag is set again. In an exception these flags are
copied to previous Kernel/User and Interrupt Enable (KUp and IEp) and
then cleared so the system moves to a kernel mode with external
interrupts disabled. The Return From Exception instruction writes the
previous flags to the current flags.
:::

当然从注释也能看出，第四步除了需要设置`cp0status`{.c}以外，还需要设置栈指针。
在MIPS中，栈寄存器是第29号寄存器，注意这里的栈是用户栈，不是内核栈。

::: exercise
[]{#exercise-env-alloc label="exercise-env-alloc"}
根据上面的提示与代码注释，填写 **env\_ alloc** 函数。
:::

### 加载二进制镜像

继续来完成我们的实验。下面这个函数还是比较困难的，我们慢慢来分析一下这个函数。

我们在[进程创建](#process-3)第三点曾提到过,我们需要为**新进程的程序**分配空间来容纳程序代码。
那么下面需要有两个函数来一起完成这个任务。

::: codeBoxWithCaption
加载镜像映射[]{#code:load_icode_mapper.c
label="code:load_icode_mapper.c"}

``` {.c linenos=""}
/* Overview:
 *   This is a call back function for kernel's elf loader.
 * Elf loader extracts each segment of the given binary image.
 * Then the loader calls this function to map each segment
 * at correct virtual address.
 *
 *   `bin_size` is the size of `bin`. `sgsize` is the
 * segment size in memory.
 *
 * Pre-Condition:
 *   bin can't be NULL.
 *   Hint: va may NOT aligned 4KB.
 *
 * Post-Condition:
 *   return 0 on success, otherwise < 0.
 */
static int load_icode_mapper(u_long va, u_int32_t sgsize,
         u_char *bin, u_int32_t bin_size, void *user_data)
{
	struct Env *env = (struct Env *)user_data;
	struct Page *p = NULL;
	u_long i;
	int r;

	/*Step 1: load all content of bin into memory. */
	for (i = 0; i < bin_size; i += BY2PG) {
	/* Hint: You should alloc a page and increase the reference count of it. */


	}
	/*Step 2: alloc pages to reach `sgsize` when `bin_size` < `sgsize`. */
	while (i < sgsize) {


	}
	return 0;
}
```
:::

为了完成这个函数，我们接下来再补充一点关于ELF的知识。

前面在讲解内核加载的时候，我们曾简要说明过ELF的加载过程。这里，再做一些补充说明。要想正确加载一个ELF文件到内存，
只需将ELF文件中所有需要加载的segment加载到对应的虚地址上即可。我们已经写好了用于解析ELF文件的代码
(lib/kernel_elfloader.c)中的大部分内容，可以直接调用相应函数获取ELF文件的各项信息，并完成加载过程。该函数的原型如下：

``` {.c linenos=""}
// binary为整个待加载的ELF文件。size为该ELF文件的大小。
// entry_point是一个u_long变量的地址(相当于引用)，解析出的入口地址会被存入到该位置
int load_elf(u_char *binary, int size, u_long *entry_point, void *user_data,
             int (*map)(u_long va, u_int32_t sgsize,
                        u_char *bin, u_int32_t bin_size, void *user_data))
```

我们来着重解释一下load_elf()函数的设计，以及最后两个参数的作用。为了让同学们有机会完成加载可执行文件到内存的过程，
load_elf()函数只完成了解析ELF文件的部分，而把将ELF文件的各个segment加载到内存的工作留给了大家。
为了达到这一目标，load_elf()的最后两个参数用于接受一个自定义函数以及大家想传递给自定义函数的额外参数。
每当load_elf()函数解析到一个需要加载的segment，会将ELF文件里与加载有关的信息作为参数传递给自定义函数。
自定义函数完成加载单个segment的过程。

为了进一步简化理解难度，我们已经定义好了这个"自定义函数"的框架。如代码[\[code:load_icode_mapper.c\]](#code:load_icode_mapper.c){reference-type="ref"
reference="code:load_icode_mapper.c"}所示。
load_elf()函数会从ELF文件文件中解析出每个segment的四个信息：va(该段需要被加载到的虚地址)、sgsize(该段在内存中的大小)、
bin(该段在ELF文件中的内容)、bin_size(该段在文件中的大小)，并将这些信息传给我们的"自定义函数"。

接下来，只需要完成以下两个步骤：

第一步

:   加载该段在ELF文件中的所有内容到内存。

第二步

:   如果该段在文件中的内容的大小达不到该段在内存中所应有的大小，那么余下的部分用0来填充。

大家会发现一个问题：我们并没有真正解释清楚user_data这个参数的作用。最后一个参数是一个函数指针，
用于将我们的自定义函数传入进去。但这个user_data到底是做什么用的呢？这样设计又是为了什么呢？
这个问题我们决定留给同学们来思考。

::: thinking
[]{#think-user-data label="think-user-data"}
思考user_data这个参数的作用。没有这个参数可不可以？为什么？（如果你能说明哪些应用场景中可能会应用这种设计就更好了。
可以举一个实际的库中的例子）
:::

思考完这一点，就可以进入这一小节的练习部分了。

::: exercise
[]{#exercise-load-icode-mapper label="exercise-load-icode-mapper"}
通过上面补充的知识与注释，填充 **load_icode_mapper** 函数。
:::

::: thinking
[]{#think-load-icode label="think-load-icode"}
结合load_icode_mapper的参数以及二进制镜像的大小，考虑该函数可能会面临哪几种复制的情况？你是否都考虑到了？
:::

现在我们已经完成了补充部分最难的一个函数，那么下面完成这个函数后，
就能真正实现把二进制镜像加载进内存的任务了。

::: codeBoxWithCaption
完整加载镜像[]{#code:load_icode.c label="code:load_icode.c"}

``` {.c linenos=""}
/* Overview:
 *  Sets up the the initial stack and program binary for a user process.
 *  This function loads the complete binary image by using elf loader,
 *  into the environment's user memory. The entry point of the binary image
 *  is given by the elf loader. And this function maps one page for the
 *  program's initial stack at virtual address USTACKTOP - BY2PG.
 *
 * Hints: 
 *  All mappings are read/write including those of the text segment.
 *  You may use these :
 *      page_alloc, page_insert, page2kva , e->env_pgdir and load_elf.
 */
static void
load_icode(struct Env *e, u_char *binary, u_int size)
{
    /* Hint:
     *  You must figure out which permissions you'll need
     *  for the different mappings you create.
     *  Remember that the binary image is an a.out format image,
     *  which contains both text and data.
     */
    struct Page *p = NULL;
    u_long entry_point;
    u_long r;
    u_long perm;
    
    /*Step 1: alloc a page. */

    /*Step 2: Use appropriate perm to set initial stack for new Env. */
    /*Hint: The user-stack should be writable? */

    /*Step 3:load the binary by using elf loader. */

    /***Your Question Here***/
    /*Step 4:Set CPU's PC register as appropriate value. */
    e->env_tf.pc = entry_point;
}
```
:::

现在来根据注释一步一步完成这个函数。
在第二步要用第一步申请的页面来初始化一个进程的栈，根据注释应当可以轻松完成。
这里我们只讲第三步的注释所代表的内容，其余可以根据注释中的提示来完成。

第三步通过调用load_elf()函数来将ELF文件真正加载到内存中。
这里仅做一点提醒：请将load_icode_mapper()这个函数作为参数传入到load_elf()中。
其余的参数在前面已经解释过了，就不再赘述了。

::: exercise
[]{#exercise-load-elf label="exercise-load-elf"}
通过补充的ELF知识与注释，填充 **load_elf** 函数和**load_icode** 函数。
:::

这里的`e->env_tf.pc`{.c}是什么呢？就是在计算机组成原理中反复强调的甚为重要的`PC`{.c}。
它指示着进程当前指令所处的位置。冯诺依曼体系结构的一大特点就是
：程序预存储，计算机自动执行。我们要运行的进程的代码段预先被载入到了**entry\_
point**为起点的
内存中，当运行进程时，CPU将自动从pc所指的位置开始执行二进制码。

::: thinking
[]{#think-位置 label="think-位置"}
思考上面这一段话，并根据自己在**lab2**中的理解，回答：

-   我们这里出现的\"指令位置\"的概念，你认为该概念是针对虚拟空间，还是物理内存所定义的呢？

-   你觉得`entry_point`{.c}其值对于每个进程是否一样？该如何理解这种统一或不同？
:::

思考完这一点后，下面我们可以真正创建进程了。

### 创建进程

创建进程的过程很简单，就是实现对上述个别函数的封装，**分配一个新的Env结构体，设置进程控制块，并将二进制代码载入到对应地址空间**即可完成。好好思考上面的函数，我们需要用到哪些函数来做这几件事？

::: exercise
[]{#exercise-env-create label="exercise-env-create"} 根据提示，完成
**env_create** 函数与 **env_create_priority** 的填写。
:::

当然提到创建进程，还需要提到一个封装好的宏命令

``` {.c linenos=""}
#define ENV_CREATE_PRIORITY(x, y) \
{ \
    extern u_char binary_##x##_start[];\
    extern u_int binary_##x##_size; \
    env_create_priority(binary_##x##_start, \
        (u_int)binary_##x##_size, y); \
}
```

``` {.c linenos=""}
#define ENV_CREATE(x) \
{ \
    extern u_char binary_##x##_start[];\
    extern u_int binary_##x##_size; \
    env_create(binary_##x##_start, \
        (u_int)binary_##x##_size); \
}
```

这个宏里的语法大家可能以前没有见过，这里解释一下`##`{.c}代表拼接，例如[^3]

``` {.c linenos=""}
#define CONS(a,b) int(a##e##b)
int main()
{
    printf("%d\n", CONS(2,3));  // 2e3 输出:2000
    return 0;
}
```

好，那么现在我们就得手工在`init/init.c`{.c}里面加两个语句来初始化创建两个进程

``` {.c linenos=""}
ENV_CREATE_PRIORITY(user_A, 2);
ENV_CREATE_PRIORITY(user_B, 1);
```

这两个语句加在哪里呢？那就需要大家阅读代码来寻找。

::: exercise
[]{#exercise-init.c label="exercise-init.c"}
根据注释与理解，将上述两条进程创建命令加入 **init/init.c** 中。
:::

做完这些，是不是迫不及待地想要跑个进程看看能否成功？等我们完成下面这个函数，就可以开始第一部分的自我测试了！

### 进程运行与切换

::: codeBoxWithCaption
进程的运行[]{#code:env_run.c label="code:env_run.c"}

``` {.c linenos=""}
extern void env_pop_tf(struct Trapframe *tf, int id);
extern void lcontext(u_int contxt);

/* Overview:
 *  Restores the register values in the Trapframe with the
 *  env_pop_tf, and context switch from curenv to env e.
 *
 * Post-Condition:
 *  Set 'e' as the curenv running environment.
 *
 * Hints:
 *  You may use these functions:
 *      env_pop_tf and lcontext.
 */
void
env_run(struct Env *e)
{
    /*Step 1: save register state of curenv. */
    /* Hint: if there is a environment running,you should do
    *  context switch.You can imitate env_destroy() 's behaviors.*/


    /*Step 2: Set 'curenv' to the new environment. */


    /*Step 3: Use lcontext() to switch to its address space. */


    /*Step 4: Use env_pop_tf() to restore the environment's
     * environment registers and drop into user mode in the
     * the environment.
     */
    /* Hint: You should use GET_ENV_ASID there.Think why? */

}

```
:::

前面说到的load_icode 是为数不多的比较难的函数之一，env_run
也是，而且其实按程度来讲可能更甚一筹。

env_run，是进程运行使用的基本函数，它包括两部分：

-   一部分是保存当前进程上下文(**如果当前没有运行的进程就跳过这一步**)

-   另一部分就是恢复要启动的进程的上下文，然后运行该进程。

::: note
进程上下文说来就是一个环境，相对于进程而言，就是进程执行时的环境。具体来说就是各个变量和数据，包括所有的寄存器变量、内存信息等。
:::

其实这里运行一个新进程往往意味着是进程切换，而不是单纯的进程运行。进程切换，
就是当前进程停下工作，让出CPU处理器来运行另外的进程。
那么要理解进程切换，我们就要知道进程切换时系统需要做些什么。
实际上进程切换的时候，为了保证下一次进入这个进程的时候我们不会再"从头来过"，
而是有记忆地从离开的地方继续往后走，我们要保存一些信息。那么，
需要保存什么信息呢？大家可能会想到下面两种需要保存的信息：

进程本身的信息

:   

进程运行环境的信息（上下文）

:   

那么我们先解决一个问题，进程本身的信息需要记录吗？
进程本身的信息是进程控制块里面那几个内容，包括

**env_id,env_status,env_parent_id,env_pgdir,env_cr3\...**

这些内容，除了env_status之外，基本不会改变。

而变化最多的就是进程运行环境的信息（上下文），包括各种寄存器。而**env_tf**保存的是进程的上下文。

这样或许大家就能明白run代码中的第一句注释的含义了：/\*Step 1: save
register state of curenv. \*/

那么进程运行到某个时刻，它的上下文------所谓的CPU的寄存器在哪呢？我们又该如何保存？
在lab3中，寄存器状态保存的地方是TIMESTACK区域。 \|struct Trapframe
\*old;\| \|old = (struct Trapframe \*)(TIMESTACK - sizeof(struct
Trapframe));\|
这个old就是当前进程的上下文所存放的区域。那么第一步注释还说到，让我们参考`env_destroy`{.c}
，其实就是让我们把old区域的东西**拷贝到当前进程的env\_
tf中**，以达到保存进程上下文的效果。
那么还有一点很关键，就是当我们返回到该进程执行的时候，应该从哪条指令开始执行？
即当前进程上下文中的pc应该设为什么值？这将留给大家去思考。

::: thinking
[]{#think-pc label="think-pc"}
思考一下，要保存的进程上下文中的`env_tf.pc`{.c}的值应该设置为多少？为什么要这样设置？
:::

思考完上面的，我们沿着注释再一路向下，后面好像没有什么很难的地方了。根据提示也完全能够做出来。

总结以上说明，不难看出 env_run 的执行流程：

1.  保存当前进程的上下文信息，设置当前进程上下文中的 pc 为合适的值。

2.  把当前进程 curenv 切换为需要运行的进程。

3.  调用 lcontext 函数，设置进程的地址空间。

4.  调用 env_pop_tf 函数，恢复现场、异常返回。

但是还有一点没完成，我们忽略了 **env_pop_tf**函数。

env_pop_tf 是定义在 lib/env_asm.S
中的一个汇编函数。这个函数也可以用来解释:为什么启动第一个进程前一定会执行
`rfe`{.gas}汇编指令。但是我们本次思考的重点不在这里，重点在于TIMESTACK。
请仔细地看看这个函数，你或许能看出什么关于TIMESTACK的端倪。
TIMESTACK问题可能将是你在本实验中需要思考时间最久的问题，希望大家积极交流，努力寻找实验
代码来支撑你们的看法与观点，鼓励提出新奇的想法！

::: thinking
[]{#think-TIMESTACK label="think-TIMESTACK"}
思考TIMESTACK的含义，并找出相关语句与证明来回答以下关于TIMESTACK的问题：

-   请给出一个你认为合适的TIMESTACK的定义

-   请为你的定义在实验中找出合适的代码段作为证据(请对代码段进行分析)

-   思考TIMESTACK和第18行的KERNEL_SP 的含义有何不同
:::

::: exercise
[]{#exercise-env-run label="exercise-env-run"} 根据补充说明，填充完成
**env_run** 函数。
:::

至此，我们第一部分的工作已经完成了！第二部分的代码量很少，但是不可或缺！

## 中断与异常

之前在学习计算机组成原理的时候已经学习了异常和中断的概念，所以这里不再将概念作为主要介绍内容。

::: note
实验里认为凡是引起控制流突变的都叫做异常，而中断仅仅是异常的一种，并且是仅有的一种异步异常。
:::

我们可以通过一个简单的图来认识一下异常的产生与返回（见图[1.3](#fig:3-exception){reference-type="ref"
reference="fig:3-exception"})。

![异常处理图示](3-exception){#fig:3-exception width="15cm"}

### 异常的分发

每当发生异常的时候，一般来说，处理器会进入一个用于分发异常的程序，
这个程序的作用就是检测发生了哪种异常，并调用相应的异常处理程序。
一般来说，这个程序会被要求放在固定的某个物理地址上（根据处理器的区别有所不同），
以保证处理器能在检测到异常时正确地跳转到那里。这个分发程序可以认为是操作系统的一部分。

代码[\[code:exec_vec3\]](#code:exec_vec3){reference-type="ref"
reference="code:exec_vec3"}就是异常分发代码，
我们先将下面代码填充到我们的`start.S`{.c}的开头，然后来分析一下。

[]{#code:exec_vec3 label="code:exec_vec3"}

``` {.gas linenos=""}
.section .text.exc_vec3
NESTED(except_vec3, 0, sp)
     .set noat
     .set noreorder
  1:
     mfc0 k1,CP0_CAUSE
     la   k0,exception_handlers
     andi k1,0x7c
     addu k0,k1
     lw   k0,(k0)
     NOP
     jr   k0
     nop
END(except_vec3)
     .set at
```

::: exercise
[]{#exercise-start.S label="exercise-start.S"} 将异常分发代码填入
**boot/start.S** 合适的部分。
:::

这段异常分发代码的作用流程简述如下：

1.  将CP0_CAUSE寄存器的内容拷贝到k1寄存器中。

2.  将execption_handlers基地址拷贝到k0。

3.  取得CP0_CAUSE中的2`~`6位，也就是对应的异常码，这是区别不同异常的重要标志。

4.  以得到的异常码作为索引去exception_handlers数组中找到对应的中断处理函数，后文中会有涉及。

5.  跳转到对应的中断处理函数中，从而响应了异常，并将异常交给了对应的异常处理函数去处理。

![CR寄存器](3-CauseRegister){#fig:3-CauseRegister width="15cm"}

图[1.4](#fig:3-CauseRegister){reference-type="ref"
reference="fig:3-CauseRegister"}是MIPS R3000中Cause Register寄存器。
其中保存着CPU中哪一些中断或者异常已经发生。bit2`~`bit6保存着异常码，也就是根据异常码来识别具体哪一个异常发生了。
bit8`~`bit15保存着哪一些中断发生了。其他的一些位含义在此不涉及，可参看MIPS开发手册。

这个.text.exec_vec3
段将通过链接器放到特定的位置，在R3000中要求是放到0x80000080地址处，
这个地址处存放的是异常处理程序的入口地址。一旦CPU发生异常，就会自动跳转到0x80000080地址处，
开始执行，下面我们将.text.exec_vec3
放到该位置，一旦异常发生，就会引起该段代码的执行，
而该段代码就是一个分发处理异常的过程。

所以要在我们的lds中增加这么一段，从而可以让R3000出现异常时自动跳转到异常分发代码处。

``` {.c linenos=""}
. = 0x80000080;
.except_vec3 : {
    *(.text.exc_vec3)
}
```

::: exercise
[]{#exercise-scse0-3.lds label="exercise-scse0-3.lds"}
将tools/scse0_3.lds代码补全使得异常后可以跳到异常分发代码。
:::

### 异常向量组

我们刚才提到了异常的分发，要寻找到exception_handlers
数组中的中断处理函数， 而exception_handlers 就是所谓的异常向量组。

下面跟随`trap_init(lib/traps.c)`{.c}函数来看一下，异常向量组里存放的是些什么？

``` {.c linenos=""}
extern void handle_int();
extern void handle_reserved();
extern void handle_tlb();
extern void handle_sys();
extern void handle_mod();
unsigned long exception_handlers[32];

void trap_init()
{
    int i;

    for (i = 0; i < 32; i++) {
        set_except_vector(i, handle_reserved);
    }

    set_except_vector(0, handle_int);
    set_except_vector(1, handle_mod);
    set_except_vector(2, handle_tlb);
    set_except_vector(3, handle_tlb);
    set_except_vector(8, handle_sys);
}
void *set_except_vector(int n, void *addr)
{
    unsigned long handler = (unsigned long)addr;
    unsigned long old_handler = exception_handlers[n];
    exception_handlers[n] = handler;
    return (void *)old_handler;
}
```

实际上呢，这个函数实现了对全局变量exception_handlers\[32\]数组初始化的工作，
其实就是把相应的处理函数的地址填到对应数组项中。 主要初始化

0号异常

:   的处理函数为handle_int，

1号异常

:   的处理函数为handle_mod，

2号异常

:   的处理函数为handle_tlb，

3号异常

:   的处理函数为handle_tlb，

8号异常

:   的处理函数为handle_sys，

一旦初始化结束，有异常产生，那么其对应的处理函数就会得到执行。而在我们的实验中，主要是
要使用0号异常，即中断异常的处理函数。因为我们接下来要做的，就是要产生时钟中断。

### 时钟中断

大家回忆一下计算机组成原理中**定时器**这个概念，我们下面来简单介绍一下时钟中断的概念。

时钟中断和操作系统的时间片轮转算法是紧密相关的。时间片轮转调度是一种很公平的算法。
每个进程被分配一个时间段，称作它的时间片，即该进程允许运行的时间。如果在时间片结束时进程还在运行，
则该进程将挂起，切换到另一个进程运行。那么CPU是如何知晓一个进程的时间片结束的呢？就是通过定时器产生的时钟中断。
当时钟中断产生时，当前运行的进程被挂起，我们需要在调度队列中选取一个合适的进程运行。
如何"选取"，就要涉及到进程的调度了。

要产生时钟中断，我们不仅要了解中断的产生与处理，还要了解gxemul是如何模拟时钟中断。
初始化时钟主要是在 kclock_init 函数中，该函数主要调用set_timer
函数，完成如下操作：

-   首先向0xb5000100位置写入1，其中0xb5000000是模拟器(gxemul)映射实时钟的位置。偏移量为0x100表示来设置实时钟中断的频率，1则表示1秒钟中断1次，如果写入0，表示关闭实时钟。实时钟对于R3000来说绑定到了4号中断上，故这段代码其实主要用来触发了4号中断。注意这里的中断号和异常号是不一样的概念，我们实验的异常包括中断。

-   一旦实时钟中断产生，就会触发MIPS中断，从而MIPS将PC指向`0x80000080`{.c}，从而跳转到`.text.exc_vec3`{.c}代码段执行。对于实时钟引起的中断，通过text.exc_vec3代码段的分发，最终会调用handle\_
    int函数来处理实时钟中断。

-   在handle\_
    int判断`CP0_CAUSE`{.c}寄存器是不是对应的4号中断位引发的中断，如果是，则执行中断服务函数timer\_
    irq。

-   在timer\_ irq里直接跳转到sched\_
    yield中执行。而这个函数就是同学们将要补充的调度函数。

以上就是时钟中断的产生与中断处理流程，在这里要完成下面的任务以顺利产生时钟中断。

::: exercise
[]{#exercise-kclock-init label="exercise-kclock-init"}
通过上面的描述，补充 **kclock_init** 函数。
:::

::: thinking
[]{#think-时钟设置 label="think-时钟设置"} 阅读 kclock_asm.S
文件并说出每行汇编代码的作用
:::

### 进程调度

通过上面的描述，我们知道了，其实在时钟中断产生时，最终是调用了sched\_
yield函数来进行进程的调度，
这个函数在`lib/sched.c`{.c}中所定义。这个函数就是本次最后要写的调度函数。
调度的算法很简单，就是时间片轮转的算法。这里优先级就有用了，我们在这里将优先级设置为时间片大小:
1 表示 1 个时间片长度, 2 表示 2 个时间片长度，
以此类推。不过寻找就绪状态进程不是简单遍历进程链表,
而是用两个链表存储所有就绪状态进程。每当一个进程状态变为 ENV_RUNNABLE
,我们要将其插入第一个就绪状态进程链表的头部。 调用 sched_yield 函数时,
先判断当前时间片是否用完。如果用完,
将其插入另一个就绪状态进程链表的头部。
之后判断当前就绪状态进程链表是否为空。如果为空,
切换到另一个就绪状态进程链表。

根据调度方法，需要在设置 env 的 status 为 ENV_RUNNABLE 的时候将该 env
插入到第一个队列中。相应的，需要在 destroy 的时候，将其从队列中移除。

::: exercise
[]{#exercise-sched-yield label="exercise-sched-yield"} 根据注释，完成
**sched_yield** 函数的补充，并根据调度方法对 env.c
中的部分函数进行修改，使得进程能够被正确调度。

提示：使用静态变量来存储当前进程剩余执行次数、当前进程、当前正在遍历的队列等。
:::

::: thinking
[]{#think-进程调度 label="think-进程调度"}
阅读相关代码，思考操作系统是怎么根据时钟周期切换进程的。
:::

至此，我们的lab3就算是圆满完成了。

## 代码导读

为了帮助同学们理清lab3代码逻辑和执行流程，我们增加了代码导读部分。首先给出lab3代码的整体运行框图，下面对照图[1.5](#fig:3-code-struct){reference-type="ref"
reference="fig:3-code-struct"}，进行分析。

![lab3代码整体运行框图](3-code-struct){#fig:3-code-struct height="22cm"}

系统入口点仍然是_start，在框图中亦可以找到，不过lab3在lab2基础上增加了一个代码段，名字叫做.text.exec_vec3，该段代码实际上是一个分发处理异常的过程，对于该段的代码在**异常的分发**小节有所详述，这里不再赘述。

进入入口后很快，CPU会执行到main函数入口处，紧接着进行一系列的初始化操作。对于本实验来说，重点引入了trap_init，env_create以及kclock_init三者函数。下面就对详细介绍这三个函数。

1.  trap_init:

    对[exception_handler]{style="color: red"}\[32\]数组进行初始化，这个数组在前面的中断处理程序中有所涉及，可参看前面.text.exec_vec3代码部分。主要初始化0号异常的处理函数为handle_int，1号异常处理函数为handle_mod，2号异常处理函数为handle_tlb，3号异常处理函数为handle_tlb，8号异常处理函数为handle_sys。（在本实验中，8号异常没有被使用过），一旦初始化结束，有异常产生，那么其对应的处理函数就会得到执行。

2.  env_create:功能很简单，创建一个进程，主要分两大部分

    (1).

    :   分配进程控制块

        env_alloc函数从空闲链表中分配一个空闲控制块，并进行相应的初始化工作，具体应该进行哪些操作请参看lab3内核代码注释！重点就是PC和SP的正确初始化。
        env_setup_vm函数，这个函数虽然没有让大家自己实现，但是该函数完成的功能却[异常的重要]{style="color: red"}。这个函数主要工作是:

        i\.

        :   为进程创建一个页表，在该系统中每一个进程都有自己独立的页表，所建立的这个页表主要在缺页中断或者tlb中断中被服务程序用来查询，或者对于具有MMU单元的系统，供MMU来进行查询

        ii\.

        :   将内核2G-4G空间映射到新创建的进程的地址空间中，这是至关重要的！为什么这是至关重要的？

            大家知道，一个应用程序不可能不使用操作提高给应用程序的API接口，或者干脆就叫做系统调用。而这些系统实现好的服务往往是位于系统空间中的。现在，假设我们新创建的进程并没有把内核2G空间映射到自己的地址空间中，那么一旦进程调用了系统提供给我们的接口，这个接口的虚拟地址位于大于2G的空间上，tlb就会拿着这个虚拟地址去查找tlb表，但是遗憾的是，表中没有，可能就会触发缺页中断，page_fault也会去查表，但是遗憾的是也没有，而且操作系统也不知道该如何却做映射，因为这个地址空间在内核中，他将会认为这个来自应用程序的一个非法访问，采取的措施很果断也很自然，直接杀死进程，当然这并不是我们想要的。我们的目的是让进程能够使用系统提供给的服务，并正常的运行起来！

    (2).

    :   载入执行程序
        将代码拷贝到新创建的进程的地址空间，这个过程实际上就是一个内存的拷贝，通过bcopy完成，具体参看学生版代码注释。

3.  kclock_init:

    该函数主要调用set_timer函数，完成如下操作:
    首先向0xb5000100位置写入1，其中0xb5000000是模拟器(gxemul)映射实时钟的位置，而偏移量为0x100表示来设置实时钟中断的频率，1则表示1秒钟中断1次，如果写入0，表示关闭实时钟。而实时钟对于R3000来说绑定到了4号中断上，故这段代码其实主要用来触发了4号中断。一旦实时钟中断产生，就会触发MIPS中断，从而MIPS将PC指向0x80000080，
    从而跳转到.text.exc_vec3代码段进行执行。关于这个代码段前面有介绍，请参看前面具体介绍。

    而对于实时钟引起的中断，通过text.exc_vec3代码段的分发，最终会调用handle_int函数来处理实时钟中断。

4.  handle_int:

    判断CP0_CAUSE寄存器是不是对应的4号中断位引发的中断，如果是，则执行中断服务函数timer_irq

5.  timer_irq:

    这个函数实现比较简单，直接跳转到sched_yield中执行。而sched_yield函数会调用引起进程切换的函数来完成进程的切换。注意:[这里是第一次进行进程切换，请务必保证：kclock_init函数在env_create函数之后调用]{style="color: red"}

6.  sched_yield:

    引发进程切换，主要分为两大块:

    (1).

    :   将正在执行的进程（如果有）的线程保存到对应的进程控制块中,具体操作参考学生版代码注释

    (2).

    :   选择一个可以运行的进程，将该进程的控制块中的线程释放进CPU各个寄存器中，恢复该进程上次被挂起时候的现场（对于新创建的进程，创建的时候仅仅初始化了PC和SP也可以看作一个现场）
        这个过程主要通过env_pop_tf来完成：该函数其他部分代码比较容易看懂，下面主要看一下关键的数条代码：

        ``` {.gas linenos=""}
                lw   k0,TF_STATUS(k0)        #恢复CP0_STATUS寄存器
                mtc0 k0,CP0_STATUS
                j    k1
                rfe
        ```

        我们知道在env.c中对进程中的env_tf.CP0_STATUS初始化为0x10001004，前面提到，这样设置的理由主要是为了让操作系统可以正常对中断（lab3中主要是时钟中断）进行响应，从而可以正常调用handle_int函数，进而进行timer_irq，进而进行sched_yield的函数执行，最终进程发生了切换。

7.  tlb中断何时被调用？

    从上面的分析看，系统在实时钟的驱动下，每隔一段时间会切换进程来执行，从而多任务可以并行的在系统中正确的运行起来，不过如果没有tlb中断，真的可以正确的运行吗，当然不行。

    因为每当系统在取数据或者取指令的时候，都会发出一个所需数据所在的虚拟地址，tlb就是将这个虚拟地址转换为对应的物理地址，进而才能够驱动内存取得正确的所期望的数据。但是如果一旦tlb在转换的时候发现没有对应于该虚拟地址的项，那么此时就会产生一个tlb中断。

    tlb对应的中断处理函数是handle_tlb，通过宏映射到了do_refill函数上：这个函数完成tlb的填充，首先介绍一下tlb的结构：

    ![Contents of a TLB Entry](3-pic-TLB){#fig:3-pic-TLB height="6cm"}

    从上往下，分别可以看到四个部分，PageMask，VPN2，PFN0，PFN1。下面就这几个简单的介绍一下。
    一切可以用例子很直观的介绍出来，这里也采用一个具体的例子，比如现在有一个虚拟地址0x82531000映射到了物理地址0x12345000，用二进制表示分别是，0b10000010010100110001000000000000映射到了0b0001
    0010001101000101000000000000。

    VPN2存放着一个虚拟页面号，就这个例子例子来说存放的是0x82531。

    G表示是否是系统tlb页表项，如果是系统页表项的话，就不需要匹配ASID区域，为什么呢？介绍ASID的作用之后大家应该就能够明白

    ASID(Address Space
    Identifiers)：地址空间标示，为什么需要这个标示呢？举个例子假设现在系统中两个进程P1，P2，其中操作系统给P1和P2呈现的地址空间都是0`~`4G的大小，如果P1和P2都访问了各自的0x62531000(包含在0`~`2G空间)这个地址，大家都知道各两个虚拟地址对应的物理地址肯定不一样，但是现在问题是这两个虚拟地址却是一样的，这就会给tlb造成疑惑，假设现在P1重新访问这个地址（或者与这个地址相距\<4KB的范围），tlb就会拿着0x62531去查表，结果发现有两项，但是此时tlb就不知道该取那一项了。而ASID正是用于解决这个问题而设计的，区分不同的进程。但是对于有些地址（位于系统空间），映射的物理地址都是一样的，tlb无需区分是哪一个进程，因为这对于所有的进程来说都是一样的。

    PFN，很显然就是保存物理页面号，C表示cache一致性，D表示被修改过，V表示该项有效。
    但是此处为什么有两项呢？因为在设计tlb的时候，为了提高tlb表项的利用率，采用了一个虚拟页面号映射为两个不同的物理页面号上，这样就必须让虚拟页面号的最低位跳过匹配检查。
    PageMask是干什么的？PageMask主要用于解决大页表映射，目前常用的是4KB页面，如果某个操作系统采用了8KB或者16KB或者更大的页表的话，MIPS体系必须支持，就采用的是PageMask，PageMask让VPN2中的低X位跳过检查，从而为页内偏移留出更多的bit位。从而实现了大页表映射。

    上面仅仅简单介绍了一下tlb中的基本内容，具体的可参看MIPS开发手册。

    下面重点介绍[tlb]{style="color: red"}缺失处理过程:

    系统为我们做了什么？在发生tlb缺失的时候，系统会把引发tlb缺失的虚拟地址填入到BadVAddr
    Register中，这个寄存器具体的含义请参看mips手册。接着触发一个tlb缺失中断。

    我们需要做什么？作为系统开发者，我们必须从BadVAddr寄存器中获取使tlb缺失的虚拟地址，接着拿着这个虚拟地址通过手动查询页表（mContext所指向的），找到这个页面所对应的物理页面号，并将这个页面号填入到PFN中，也就是EntryHi寄存器，填写好之后，tlbwr指令就会将填入的内容填入到具体的某一项tlb表项中。

8.  handle_sys:

    lab3中虽然没有用到系统调用，在这里简单介绍一下系统调用的过程，在lab4中会用到。
    系统调用异常是通过在用户态执行syscall指令来触发的，一旦触发异常，根据上面介绍的就会调用.text_exc_vec3代码段的代码，进而进行异常分发，最终调用handle_sys函数。

    这个函数的功能比较简单，实质上也是一个分发函数，首先这个函数需要从用户态拷贝参数到内核中，然后根据第一个参数（是一个系统调用号）来作为一个数组的索引，取得该索引项所对应的那一项的值，其中这个数组就是[sys_call_table(系统调用表)]{style="color: red"}，数组里面存放的每一项都是位于内核中的系统调用服务函数的入口地址。一旦找到对应的入口地址，则跳转到该入口处进行代码的执行。

## 实验正确结果

如果按流程做下来并且做的结果正确的话，运行之后应该会出现这样的结果

``` {.text linenos=""}
init.c: mips_init() is called

Physical memory: 65536K available, base = 65536K, extended = 0K

to memory 80401000 for struct page directory.

to memory 80431000 for struct Pages.

mips_vm_init:boot_pgdir is 80400000

pmap.c:  mips vm init success

panic at init.c:27: ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2
1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1
1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1
2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2
1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1
1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1
2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2
1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1
1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1
```

当然不会这么整齐，且没有换行，只是交替输出2和1而已～ 不过 1 的个数几乎是
2 的 2 倍。

## 任务列表

-   [**[Exercise-mips_vm_init]{style="color: baseB"}**](#exercise-mips-vm-init)

-   [**[Exercise-env_init]{style="color: baseB"}**](#exercise-env-init)

-   [**[Exercise-envid2env]{style="color: baseB"}**](#exercise-envid2env)

-   [**[Exercise-env_setup_vm]{style="color: baseB"}**](#exercise-env-setup-vm)

-   [**[Exercise-env_alloc]{style="color: baseB"}**](#exercise-env-alloc)

-   [**[Exercise-load_icode_mapper]{style="color: baseB"}**](#exercise-load-icode-mapper)

-   [**[Exercise-load_elf and
    load_icode]{style="color: baseB"}**](#exercise-load-elf)

-   [**[Exercise-env_create and
    env_create_priority]{style="color: baseB"}**](#exercise-env-create)

-   [**[Exercise-init.c]{style="color: baseB"}**](#exercise-init.c)

-   [**[Exercise-env_run]{style="color: baseB"}**](#exercise-env-run)

-   [**[Exercise-start.S]{style="color: baseB"}**](#exercise-start.S)

-   [**[Exercise-scse0_3.lds]{style="color: baseB"}**](#exercise-scse0-3.lds)

-   [**[Exercise-kclock_init]{style="color: baseB"}**](#exercise-kclock-init)

-   [**[Exercise-sched_yield]{style="color: baseB"}**](#exercise-sched-yield)

## 实验思考

-   [**[思考-init的逆序插入]{style="color: baseB"}**](#think-env_init)

-   [**[思考-mkenvid的作用]{style="color: baseB"}**](#think-mkenvid)

-   [**[思考-地址空间初始化]{style="color: baseB"}**](#think-env_setup_vm)

-   [**[思考-user_data的作用]{style="color: baseB"}**](#think-user-data)

-   [**[思考-load-icode的不同情况]{style="color: baseB"}**](#think-load-icode)

-   [**[思考-位置的含义]{style="color: baseB"}**](#think-位置)

-   [**[思考-进程上下文的PC值]{style="color: baseB"}**](#think-pc)

-   [**[思考-TIMESTACK的含义]{style="color: baseB"}**](#think-TIMESTACK)

-   [**[思考-时钟的设置]{style="color: baseB"}**](#think-时钟设置)

-   [**[思考-进程的调度]{style="color: baseB"}**](#think-进程调度)

[^1]: 这里创建进程是指系统创建进程，不是指fork等系统调用创建进程。我们将在lab4中介绍fork这种进程创建的方式。

[^2]: 我们的实验是不支持中断嵌套的， 所以内核态时是不可以开启中断的。

[^3]: 这个例子是转载的，出处为<http://www.cnblogs.com/hnrainll/archive/2012/08/15/2640558.html>，想深入了解的同学可以参考这篇博客
