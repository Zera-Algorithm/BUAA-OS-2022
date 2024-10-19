# 文件系统

## 实验目的

1.  了解文件系统的基本概念和作用。

2.  了解普通磁盘的基本结构和读写方式。

3.  了解实现设备驱动的方法。

4.  掌握并实现文件系统服务的基本操作。

5.  了解微内核的基本设计思想和结构。

在之前的实验中，所有的程序和数据都存放在内存中。然而内存空间的大小是有限的，且内存中的数据
具有易失性。因此有些数据必须保存在磁盘、光盘等外部存储设备上。这些存储设备能够长期地保存大
量的数据，且可以方便地将数据装载到不同进程的内存空间进行共享。
为了便于管理存放在外部存储设备上的数据，我们在操作系统中引入了文件系统。文件系统将文件作为
数据存储和访问的单位，可看作是对用户数据的逻辑抽象。对于用户而言，文件系统可以屏蔽访问外存
上数据的复杂性。

## 文件系统概述

计算机文件系统是一种存储和组织数据的方法，它使得对数据的访问和查找变得容易。
文件系统使用文件和树形目录的抽象逻辑概念代替了硬盘和光盘等物理设备使用数据块的概念，
用户不必关心数据实际保存在硬盘（或者光盘）的哪个数据块上，只需要记住这个文件的所属目录和文件名。在写入新数据之前，
用户不必关心硬盘上的哪个块没有被使用，硬盘上的存储空间管理（分配和释放）由文件系统自动完成，用户只需要记住数据被写入到了哪个文件中。

文件系统通常使用硬盘和光盘这样的存储设备，并维护文件在设备中的物理位置。但是，实际上文件系统也可能仅仅是一种访问数据的界面而已，实际的数据在内存中或者通过网络协议（如NFS、SMB、9P等）提供，甚至可能根本没有对应的文件（如
proc文件系统）。

简单地说，文件系统通过对外部存储设备的抽象，实现了数据的存储、组织、访问和获取等操作。

::: thinking
[]{#think-proc label="think-proc"} 查阅资料，了解Linux/Unix的 /proc
文件系统是什么？有什么作用？Windows操作系统又是如何实现这些功能的？proc
文件系统的设计有哪些好处和不足？
:::

### 磁盘文件系统

磁盘文件系统是一种利用存储设备来保存计算机文件的文件系统，最常用的数据存储设备是磁盘驱动器，可以直接或者间接地连接到计算机上。操作系统可以支持多种磁盘文件系统，如可以在Linux中挂载使用Ext4、FAT32等多种文件系统，但是Linux中运行的程序访问这些文件系统的界面都是Linux的VFS虚拟文件系统。

### 用户空间文件系统

在以Linux为代表的宏内核操作系统中，文件系统是内核的一部分。文件系统作为内核资源的索引发挥了重要的定位内核资源的作用，重要的mmap，ioctl,
read,
write操作都依赖文件系统实现。与此相对的是众多微内核操作系统中使用的用户空间文件系统，其特点是文件系统在用户空间中实现，通过特殊的系统调用接口或者通用机制为其他用户程序提供服务。
与此概念相关的还有用户态驱动程序。

### 文件系统的设计与实现

在本次实验中，我们将要实现一个简单但结构完整的文件系统。整个文件系统包括以下几个部分：

1.  **外部存储设备驱动** 通常，外部设备的操作需要通过按照一定操作序列读写特定的寄存器来实现。为了将这种操作转化为具有通用、明确语义的接口，必须实现相应的驱动程序。在本部分，我们将实现IDE磁盘的用户态驱动程序。

2.  **文件系统结构** 在本部分，我们实现磁盘上和操作系统中的文件系统结构，并通过驱动程序实现文件系统操作相关函数。

3.  **文件系统的用户接口** 在本部分，我们提供接口和机制使得用户程序能够使用文件系统，这主要通过一个用户态的文件系统服务来实现。同时，引入了文件描述符等结构使操作系统和用户程序可以抽象地操作文件而忽略其实际的物理表示。

::: note
IDE 的英文全称为"Integrated Drive
Electronics"，即"集成电子驱动器"，是目前最主流的硬盘接口，
也是光储类设备的主要接口。IDE接口，也称之为ATA接口。
:::

接下来我们一一详细解读这些部分的实现。

## IDE磁盘驱动

为了在磁盘等外部设备上实现文件系统，我们必须为这些外部设备编写驱动程序。实际上，MOS操作系统中已经实现了一个简单的驱动程序，那就是位于driver目录下的串口通信驱动程序。在这个驱动程序中使用了内存映射I/O(MMIO)技术编写驱动。

本次要实现的硬盘驱动程序与已经实现的串口驱动相比，同样使用MMIO技术编写磁盘的驱动；而不同之处在于，我们需要驱动的物理设备------IDE磁盘功能更加复杂，并且本次要编写的驱动程序**完全运行在用户空间**。

### 内存映射I/O

在第二个实验中，我们已经了解了MIPS存储器地址映射的基本内容。几乎每一种外设都是通过读写设备上的寄存器来进行数据通信，
外设寄存器也称为**I/O端口**，我们使用I/O端口来访问I/O设备。外设寄存器通常包括控制寄存器、状态寄存器和
数据寄存器。这些硬件I/O寄存器被映射到指定的内存空间。例如，在 Gxemul
中，console 设备被映射到 `0x10000000`{.c}，simulated IDE disk被映射到
`0x13000000`{.c}，等等。更详细的关于 Gxemul 的仿真
设备的说明，可以参考[Gxemul Experimental
Devices](http://gavare.se/gxemul/gxemul-stable/doc/experiments.html#expdevices)。

驱动程序访问的是I/O空间，与一般我们说的内存空间不同。外设的I/O地址空间是系统启动后才确定（实际上，这个工作是由BIOS完成后告知操作系统）。
通常的体系结构（如x86）没有为这些外设I/O空间的物理地址预定义虚拟地址范围，所以驱动程序并不能直接访问I/O虚拟地址空间，
因此**必须首先将它们映射到内核虚地址空间**，驱动程序才能基于虚拟地址及访存指令来实现对IO设备的访问。

幸运的是，实验中使用的MIPS体系结构并没有I/O端口的概念，而是统一使用内存映射I/O的模型。MIPS的地址空间中，其在内核地址空间中（kseg0和kseg1段）实现了硬件级别的物理地址和内核虚拟地址的转换机制，其中，对kseg1段地址的读写是未缓存（uncached）的，即所有的操作都不会写入高速缓存，这种可见性正是设备驱动所需要的。由于我们在模拟器上运行操作系统，I/O设备的物理地址是完全固定的。这样一来就可以简单地通过读写某些固定的内核虚拟地址来实现驱动程序的功能。

在之前的实验中，我们曾经使用KADDR宏把一个物理地址转换为kseg0段的内核虚拟地址，实际上是给物理地址加上ULIM的值（即`0x80000000`{.c}）。正如我们上面提到的，
编写设备驱动的时候我们需要将物理地址转换为kseg1段的内核虚拟地址，也就是将物理地址加上
kseg1 的偏移值(`0xA0000000`{.c})。

::: thinking
[]{#think-fs-cache label="think-fs-cache"}

如果通过kseg0读写设备，那么对于设备的写入会缓存到Cache中。通过kseg0访问设备是一种**错误**的行为，在实际编写代码的时候这么做会引发不可预知的问题。请思考：这么做这会引发什么问题？对于不同种类的设备（如我们提到的串口设备和IDE磁盘）的操作会有差异吗？可以从缓存的性质和缓存更新的策略来考虑。
:::

以我们编写完成的串口设备驱动为例，Gxemul 提供的 console
设备的地址为0x10000000，设备寄存器映射如表[1.1](#lab5-table-console-mem-map){reference-type="ref"
reference="lab5-table-console-mem-map"}所示：

::: {#lab5-table-console-mem-map}
   Offset  Effect
  -------- -------------------------------------------------------------------
    0x00   Read: getchar() (non-blocking; returns 0 if no char is available)
           Write: putchar(ch)
    0x10   Read or write: halt()
           (Useful for exiting the emulator.)

  : Gxemul Console 内存映射
:::

现在，通过往内存的 (0x10000000+0xA0000000) 地址写入字符，就能在 shell
中看到对应的输出。
`drivers/gxconsole/console.c`{.c}中的`printcharc`{.c}函数的实现如下所示：

``` {.c linenos=""}
void printcharc(char ch)
{
  *((volatile unsigned char *) PUTCHAR_ADDRESS) = ch;
}
```

而在本次实验中，我们需要编写IDE磁盘的驱动完全位于用户空间，用户态进程若是直接读写内核虚拟地址将会由处理器引发一个地址错误（ADEL/S)。所以对于设备的读写必须通过系统调用来实现。这里我们引入了sys_write_dev和sys_read_dev两个系统调用来实现设备的读写操作。这两个系统调用接受用户虚拟地址，设备的物理地址和读写的长度（按字节计数）作为参数，在内核空间中完成I/O操作。

::: exercise
[]{#exercise-syscall-dev label="exercise-syscall-dev"} 请根据
lib/syscall_all.c 中的说明，完成 `sys_write_dev`{.c} 函数和
`sys_read_dev`{.c} 函数的实现，并且在 user/lib.h, user/syscall_lib.c
中完成用户态的相应系统调用的接口。

编写这两个系统调用时需要注意物理地址与内核虚拟地址之间的转换。

同时还要检查物理地址的有效性，在实验中允许访问的地址范围为: console:
\[`0x10000000`{.c}, `0x10000020`{.c}), disk: \[`0x13000000`{.c},
`0x13004200`{.c}), rtc: \[`0x15000000`{.c},
`0x15000200`{.c})，当出现越界时，应返回指定的错误码。
:::

### IDE磁盘

在MOS操作系统实验中，Gxemul 模拟器提供的"磁盘"是一个 IDE
仿真设备，需要此基础上实现文件系统，接下来，我们将了解一些读写 IDE
磁盘的基础知识。

#### 磁盘的物理结构

我们首先简单介绍一下与磁盘相关的基本知识。首先是几个基本概念：

1.  扇区(Sector)：磁盘盘片被划分成很多扇形的区域，叫做扇区。扇区是磁盘执行读写操作的单位，一般是512字节。
    扇区的大小是一个磁盘的硬件属性。

2.  磁道(track): 盘片上以盘片中心为圆心，不同半径的同心圆。

3.  柱面(cylinder)：硬盘中，不同盘片相同半径的磁道所组成的圆柱。

4.  磁头(head)：每个磁盘有两个面，每个面都有一个磁头。当对磁盘进行读写操作时，磁头在盘片上快速移动。

典型的磁盘的基本结构如图[1.1](#lab5-pic-1){reference-type="ref"
reference="lab5-pic-1"}所示。

![磁盘结构示意图](lab5-pic-1){#lab5-pic-1 width="12cm"}

#### IDE磁盘操作

前文中我们提到过，扇区(Sector) 是磁盘读写的基本单位，Gxemul
也提供了对扇区进行操作的基本方法。Gxemul提供了模拟IDE磁盘（Simulated IDE
disk），我们可以把它当作真实的磁盘去读写数据，同时还可以通过查看特定位置判断读写操作是否成功（特定位置和缓冲区的偏移量在表[1.2](#lab5-table-disk-mem-map){reference-type="ref"
reference="lab5-table-disk-mem-map"}中给出）。。

Gxemul 提供的模拟IDE磁盘的地址是 0x13000000，I/O寄存器相对于
`0x13000000`{.c} 的偏
移和对应的功能如表[1.2](#lab5-table-disk-mem-map){reference-type="ref"
reference="lab5-table-disk-mem-map"}所示：

::: {#lab5-table-disk-mem-map}
      Offset      Effect
  --------------- ------------------------------------------------------------------------------------------------------------------------------------
      0x0000      Write: Set the offset (in bytes) from the beginning of the disk image. This offset will be used for the next read/write operation.
      0x0008      Write: Set the high 32 bits of the offset (in bytes). (\*)
      0x0010      Write: Select the IDE ID to be used in the next read/write operation.
      0x0020      Write: Start a read or write operation. (Writing 0 means a Read operation, a 1 means a Write operation.)
      0x0030      Read: Get status of the last operation. (Status 0 means failure, non-zero means success.)
   0x4000-0x41ff  Read/Write: 512 bytes data buffer.

  : Gxemul IDE disk I/O 寄存器映射
:::

### 驱动程序编写

通过对`printcharc`{.c}
函数的实现的分析，我们已经掌握了I/O操作的基本方法，那么，读写 IDE
磁盘的相关代码也就不难理解了。我们以从硬盘上读取一些扇区为例，先了解一下**内核态的驱动**是如何编写的：

`read_sector`{.c} 函数：

``` {.c linenos=""}
extern int read_sector(int diskno, int offset);
```

``` {.asm linenos=""}
# read sector at specified offset from the beginning of the disk image.
LEAF(read_sector)
    sw  a0, 0xB3000010  # select the IDE id.
    sw  a1, 0xB3000000  # offset.
    li  t0, 0
    sb  t0, 0xB3000020  # start read.
    lw  v0, 0xB3000030
    nop
    jr  ra
    nop
END(read_sector)
```

当需要从磁盘的指定位置读取一个 sector 时，我们需要调用 `read_sector`{.c}
函数来将磁盘中对应sector
的数据读到设备缓冲区中。注意，**所有的地址操作都需要将物理地址转换成虚拟地址**。此处设备基地址对应的kseg1的内核虚拟地址是0xB3000000。

首先，设置IDE disk 的 ID，从`read_sector`{.c} 函数的声明
`extern int read_sector(int diskno, int offset);`{.c} 中可以看出，diskno
是第一个参数，对应的就是 \$a0 寄存器的值，因此，将其写入到 0xB3000010
处，这样就表示 我们将使用编号为 \$a0 的磁盘。在本实验中，只使用了一块
simulated IDE disk, 因此，这个值应该为 0。

接下来，将相对于磁盘起始位置的 offset 写入到 0xB3000000
位置，表示在距离磁盘起始处 offset 的位置开始 进行磁盘操作。然后，根据
Gxemul 的 data
sheet(表[1.2](#lab5-table-disk-mem-map){reference-type="ref"
reference="lab5-table-disk-mem-map"})，向内存 0xB3000020 处写入 0
来开始读磁盘（如果是写磁盘，则写 入 1）。

最后，将磁盘操作的状态码放入 \$v0 中，作为结果返回。通过判断
`read_sector`{.c}
函数的返回值，就可以知道读取磁盘的操作是否成功。如果成功，将这个 sector
的数据(512 bytes) 从设备缓冲区 (offset 0x4000-0x41ff)
中拷贝到目的位置。至此，就完成了对磁盘的读操作。
写磁盘的操作与读磁盘的一个区别在于写磁盘需要先将要写入对应 sector 的 512
bytes 的数据放入设备缓冲中，然后向地址 0xB3000020处写入 1
来启动操作，并从 0xB3000030
处获取写磁盘操作的返回值。相应地，**用户态磁盘驱动**使用系统调用代替直接对内存空间的读写，从而完成寄存器配置和数据拷贝等功能。

::: exercise
[]{#exercise-ide label="exercise-ide"} 参考内核态驱动，完成 fs/ide.c
中的 `ide_write`{.c} 函数和 `ide_read`{.c}
函数，实现对磁盘的读写操作。（有多种实现方式，可以使用汇编语言直接操作地址，也可以使用系统调用的方式）
:::

## 文件系统结构

实现了IDE磁盘的驱动，就有了在磁盘上实现文件系统的基础。接下来我们设计整个文件系统的结构，并在磁盘和操作系统中分别实现对应的结构。

::: note
Unix/Linux操作系统一般将磁盘分成两个区域：inode 区域和 data 区域。inode
区域用来保存文件的状态属性， 以及指向数据块的指针。data
区域用来存放文件的内容和目录的元信息(包含的文件)。MOS操作系统的文件系统也采用类似的设计。
:::

### 磁盘文件系统布局

磁盘空间的基本布局如图[1.2](#lab5-pic-2){reference-type="ref"
reference="lab5-pic-2"}所示。

![磁盘空间布局示意图](lab5-pic-2){#lab5-pic-2 width="12cm"}

从图[1.2](#lab5-pic-2){reference-type="ref"
reference="lab5-pic-2"}中可以看到磁盘最开始的一个扇区(512字节)被当成是启动扇区和分区表使用。接下来的一个扇区用作
超级块(Super Block)，用来描述文件系统的基本信息，如 Magic
Number、磁盘大小以及根目录的位置。

::: note
在真实的文件系统中，一般会维护多个超级块，通过复制分散到不同的磁盘分区中，以防止超级块的损坏造成整个磁盘
无法使用。
:::

MOS操作系统中超级块的结构如下:

``` {.c linenos=""}
struct Super {
    u_int s_magic;      // Magic number: FS_MAGIC
    u_int s_nblocks;    // Total number of blocks on disk
    struct File s_root; // Root directory node
};
```

其中每个域的意义如下：

-   `s_magic`{.c}：魔数，用于识别该文件系统，为一个常量。

-   `s_nblocks`{.c}：记录本文件系统有多少个磁盘块，本文件系统为1024。

-   `s_root`{.c}为根目录，其`f_type`{.c}为`FTYPE_DIR`{.c}，`f_name`{.c}为\"/\"。

通常采用两种数据结构来管理可用的资源：链表和位图。在lab2实验和lab3实验中，我们使用了链表来管理
空闲内存资源和进程控制块。在文件系统中，将使用位图(Bitmap)法来管理空闲的磁盘资源，也就是通过
一个二进制位来表示一个磁盘块(Block)是否使用。

接下来，我们来学习如何使用位图来标识磁盘中的所有块的使用情况。

tools/fsformat
是用于创建符合我们定义的文件系统结构的工具，用于将多个文件按照内核所定义的文件系统写入到磁盘镜像中。
这里我们参考 tools/fsformat
表述文件系统标记空闲块的机制。在写入文件之前，在
fs/fsformat.c的`init_disk`{.c} 中，我们将所有的块都标为空闲块：

``` {.c linenos=""}
    nbitblock = (NBLOCK + BIT2BLK - 1) / BIT2BLK;
    for(i = 0; i < nbitblock; ++i) {
        memset(disk[2+i].data, 0xff, nblock/8);
    }
    if(nblock != nbitblock * BY2BLK) {
        diff = nblock % BY2BLK / 8;
        memset(disk[2+(nbitblock-1)].data+diff, 0x00, BY2BLK - diff);
    }
```

nbitblock
记录了整个磁盘上所有块的使用信息，需要多少个块来存储位图。紧接着，使用memset将位图中的每一个字节都设成
0xff ，即将所有位图块的每一位都设为 1
，表示这一块磁盘处于空闲状态。如果位图还有剩余，不能将最后一块位图块中靠后的一部分内容标记为
空闲，因为这些位所对应的磁盘块并不存在，不可使用。因此，将所有的位图块的每一位都置为
1 之后，还需要根据实际情况，将多出来的位图设为 0。

相应地，在MOS操作系统中，文件系统也需要根据位图来判断和标记磁盘的使用情况。fs/fs.c
中的 `block_is_free`{.c}
函数就用来通过位图中的特定位来判断指定的磁盘块是否被占用。

``` {.c linenos=""}
int block_is_free(u_int blockno)
{
    if (super == 0 || blockno >= super->s_nblocks) {
        return 0;
    }
    if (bitmap[blockno / 32] & (1 << (blockno % 32))) {
        return 1;
    }
    return 0;
}
```

::: exercise
[]{#exercise-free-block label="exercise-free-block"}
文件系统需要负责维护磁盘块的申请和释放，在回收一个磁盘块时，需要更改位图中的标志位。如果要将一个磁盘块设置为
free， 只需要将位图中对应的位的值设置为 1 即可。请完成 fs/fs.c 中的
`free_block`{.c} 函数， 实现这一功能。同时思考为什么参数 `blockno`{.c}
的值不能为 0 ？

``` {.c linenos=""}
// Overview:
//  Mark a block as free in the bitmap.
void
free_block(u_int blockno)
{
    // Step 1: Check if the parameter `blockno` is valid (`blockno` can't be zero). 

    // Step 2: Update the flag bit in bitmap.

}
```
:::

### 文件系统详细结构

操作系统要想管理一类资源，就得有相应的数据结构。对于描述和管理文件来说，一般使用文件控制块。
其定义如下：

``` {.c linenos=""}
// file control blocks, defined in include/fs.h
struct File {
    u_char f_name[MAXNAMELEN];  // filename
    u_int f_size;           // file size in bytes
    u_int f_type;           // file type
    u_int f_direct[NDIRECT];
    u_int f_indirect;
    struct File *f_dir;
    u_char f_pad[BY2FILE - MAXNAMELEN - 4 - 4 - NDIRECT * 4 - 4 - 4];
};
```

结合文件控制块的示意图[1.3](#lab5-pic-3){reference-type="ref"
reference="lab5-pic-3"}，我们对各个域进行解读：
`f_name`{.c}为文件名称，文件名的最大长度为 MAXNAMELEN
值为128。`f_size`{.c}为文件的大小，单位为字节。`f_type`{.c}为文件类型，
有普通文件(`FTYPE_REG`{.c})和文件夹(`FTYPE_DIR`{.c})
两种。`f_direct[NDIRECT]`{.c} 为文件的直接指针，每个文件控制块设有 10
个直接指针， 用来记录文件的数据块在磁盘上的位置。每个磁盘块的大小为
4KB，也就是说，这十个直接指针能够表示最大 40KB
的文件，而当文件的大小大于 40KB 时，就需要用到间接指针。
`f_indirect`{.c}指向一个间接磁盘块，用来存储指向文件内容
的磁盘块的指针。为了简化计算，我们不使用间接磁盘块的前十个指针。
`f_dir`{.c}指向文件所属的文件目录。`f_pad`{.c}则是为了让一个文件结构体占用一个磁盘块，填充结构体中剩下的字节。

![文件控制块](lab5-pic-3){#lab5-pic-3 width="12cm"}

::: note
我们的文件系统中文件控制块只是用了一级间接指针域，也只有一个。而在真实的文件系统中，对了支持更大的文件，通
常会使用多个间接磁盘块，或使用多级间接磁盘块。MOS操作系统内核在这一点上做了极大的简化。
:::

::: thinking
[]{#think-filesize label="think-filesize"} 一个磁盘块最多存储 1024
个指向其他磁盘块的指针，试计算，我们的文件系统支持的单个文件最大为多大？
:::

对于普通的文件，其指向的磁盘块存储着文件内容，而对于目录文件来说，其指向的磁盘块存储着该目录下各个文件对应的的文件控制块。当我们要查找某个文件时，首先从超级块中读取根目录的文件控制块，然后沿着目标路径，挨个查看当前目录所包含的文件是否与下一级目标文件同名，如此便能查找到最终的目标文件。

::: thinking
[]{#think-filenum label="think-filenum"}
查找代码中的相关定义，试回答一个磁盘块中最多能存储多少个文件控制块？一个目录下最多能有多少个文件？
:::

为了更加细致地了解文件系统的内部结构，我们通过
fsformat（由fs/fsformat.c编译而成）程序来创建一个磁盘镜像文件
gxemul/fs.img。 通过观察头文件和
fs/Makefile中我们可以看出，fs/fsformat.c
的编译过程与其他文件有所不同。生成的镜像文件 fs.img
可以模拟与真实的磁盘文件设备的交互。 请阅读 fs/fsformat.c
中的代码，掌握如何将文件和文件夹按照文件系统的格式写入磁盘，了解文件系统结构的具体细节。（fsformat.c
中的主函数十分灵活，可以通过修改命令行参数来生成不同的镜像文件。）

::: exercise
[]{#exercise-fsformat label="exercise-fsformat"}
请参照文件系统的设计，完成fsformat.c中的
`create_file`{.c}函数，并按照个人兴趣完成 `write_directory`{.c}
函数（不作为考查点），实现将一个文件或指定目录下的文件按照目录结构写入到
fs/fs.img 的根目录下的功能。

在实现的过程中，你可以将你的实现同我们给出的参考可执行文件
tools/fsformat 进行对比。具体来讲，可以通过Linux提供的 xxd 命令将两个
fsformat 产生的二进制镜像转化为可阅读的文本文件，手工进行查看或使用 diff
等工具进行对比。
:::

### 块缓存

块缓存指的是借助虚拟内存来实现磁盘块缓存的设计。MOS操作系统中，文件系统服务是一个用户进程（将在下文介绍），一个进程可以拥有
4GB 的虚拟内存空间，将 `DISKMAP`{.c} 到 `DISKMAP+DISKMAX`{.c}
这一段虚存地址空间 (0x10000000-0x4fffffff)
作为缓冲区，当磁盘读入内存时，用来映射相关的页。`DISKMAP`{.c} 和
`DISKMAX`{.c} 的值定义在 fs/fs.h 中：

``` {.c linenos=""}
#define DISKMAP   0x10000000
#define DISKMAX   0x40000000
```

::: thinking
[]{#think-disksize label="think-disksize"}
请思考，在满足磁盘块缓存的设计的前提下，实验使用的内核支持的最大磁盘大小是多少？
:::

为了建立起磁盘地址空间和进程虚存地址空间之间的缓存映射，我们采用的设计如图[1.4](#lab5-pic-4){reference-type="ref"
reference="lab5-pic-4"}所示。

![块缓存示意图](lab5-pic-4){#lab5-pic-4 width="12cm"}

::: exercise
[]{#exercise-diskaddr label="exercise-diskaddr"} fs/fs.c 中的 diskaddr
函数用来计算指定磁盘块对应的虚存地址。完成 `diskaddr`{.c} 函数，根据
一个块的序号(block
number)，计算这一磁盘块对应的虚存的起始地址。（提示：fs/fs.h 中的宏
`DISKMAP`{.c} 和 `DISKMAX`{.c} 定义了磁盘映射虚存的地址空间）。
:::

::: thinking
[]{#think-diskmap-userspace label="think-diskmap-userspace"}
如果将DISKMAX改成0xC0000000,
超过用户空间，我们的文件系统还能正常工作吗？为什么？
:::

当把一个磁盘块中的内容载入到内存中时，需要为之分配对应的物理内存；当结束使用这一磁盘块时，需要释放对应的
物理内存以回收操作系统资源。fs/fs.c 中的 `map_block`{.c} 函数和
`unmap_block`{.c} 函 数实现了这一功能。

::: exercise
[]{#exercise-map-block label="exercise-map-block"} 实现 `map_block`{.c}
函数，检查指定的磁盘块是否已经映射到内存，如果没有，分配一页内存来保存磁盘上的数据。相应地，
完成 `unmap_block`{.c}
函数，用于解除磁盘块和物理内存之间的映射关系，回收内存。（提示：注意磁盘虚拟内存地址空间
和磁盘块之间的对应关系）。
:::

`read_block`{.c} 函数和 `write_block`{.c}
函数用于读写磁盘块。`read_block`{.c}
函数将指定编号的磁盘块读入到内存中，首先检查这块磁盘块是否已经在内存中，如果不在，先分配一页物理内存，然后调用
`ide_read`{.c} 函数来读取磁盘上的数据到对应的虚存地址处。

在完成块缓存部分之后我们就可以实现文件系统中的一些文件操作了。

::: exercise
[]{#exercise-dir-lookup label="exercise-dir-lookup"} 补全
`dir_lookup`{.c}
函数，查找某个目录下是否存在指定的文件。（提示：使用`file_get_block`{.c}可以将某个指定文件指向的磁盘块读入内存）。
:::

这里我们给出了文件系统结构中部分函数可能的调用参考(图[1.5](#lab5-function-hint-fs){reference-type="ref"
reference="lab5-function-hint-fs"})，希望同学们仔细理解每个文件、函数的作用和之间的关系。

![fs/下部分函数调用关系参考](lab5-function-hint-fs.pdf){#lab5-function-hint-fs
width="15cm"}

## 文件系统的用户接口

文件系统建立之后，需要向用户提供相关的接口使用。MOS操作系统内核符合一个典型的微内核的设计，文件系统属于用户态进程，以服务的形式供其他进程调用。
这个过程中，不仅涉及了不同进程之间通信的问题，也涉及了文件系统如何隔离底层的文件系统实现，抽象地表示一个文件的问题。首先，我们引入文件描述符（file
descriptor）作为用户程序管理、操作文件资源的方式。

### 文件描述符

当用户进程试图打开一个文件时，需要一个文件描述符来存储文件的基本信息和用户进程中关于文件的状态；
同时，文件描述符也起到描述用户对于文件操作的作用。当用户进程向文件系统发送打开文件的请求时，文件系统进程会将这些基本信息记录在内存中，
然后由操作系统将用户进程请求的地址映射到同一个物理页上，因此一个文件描述符至少需要独占一页的空间。当用户进程获取了文件大小等基本信息后，
再次向文件系统发送请求将文件内容映射到指定内存空间中。

::: thinking
[]{#think-fd-mem label="think-fd-mem"} 阅读
user/file.c，思考文件描述符和打开的文件分别映射到了内存的哪一段空间。
:::

::: thinking
[]{#think-Filefd-Fd label="think-Filefd-Fd"} 阅读
user/file.c，大家会发现很多函数中都会将一个 struct Fd \* 型的指针转换为
struct Filefd \* 型的指针，请解释为什么这样的转换可行。
:::

::: exercise
[]{#exercise-fs-open label="exercise-fs-open"} 完成user/file.c中的
`open`{.c}
函数。（提示：若成功打开文件，则该函数返回文件描述符的编号）。
:::

当要读取一个大文件中间的一小部分内容时，从头读到尾是极为浪费的，因此需要一个指针帮助我们在文件中定位，在C语言中拥有类似功能的函数是
`fseek`{.c}
。而在读写期间，每次读写也会更新该指针的值。请自行查阅C语言有关文件操作的函数，理解相关概念。

::: exercise
[]{#exercise-fs-read label="exercise-fs-read"} 参考user/fd.c中的
`write`{.c} 函数，完成 `read`{.c} 函数。
:::

::: thinking
[]{#think-structures label="think-structures"}请解释 Fd, Filefd, Open
结构体及其各个域的作用。比如各个结构体会在哪些过程中被使用，是否对应磁盘上的物理实体还是单纯的内存数据等。说明形式自定，要求简洁明了，可大致勾勒出文件系统数据结构与物理实体的对应关系与设计框架。
:::

### 文件系统服务

MOS操作系统中的文件系统服务通过 IPC
的形式供其他进程调用，进行文件读写操作。具体来说，在内核开始运行时，就启动了文件系统服务进程
`ENV_CREATE(fs_serv)`{.c}，用户进程需要进行文件操作时，使用
`ipc_send/ipc_recv`{.c} 与 `fs_serv`{.c} 进行交互，完成操作。
在文件系统服务进程的主函数serv.c/umain中，首先调用了 `serv_init`{.c}
函数准备好全局的文件打开记录表 `opentab`{.c}，然后调用 `fs_init`{.c}
函数来初始化文件系统。`fs_init`{.c}
函数首先通过读取超级块的内容获知磁盘的基本信息，然后检查磁盘是否能够正常读写，最后调用
`read_bitmap`{.c}
函数检查磁盘块上的位图是否正确。执行完文件系统的初始化后，调用`serve`{.c}
函数，文件系统服务开始运行， 等待其他程序的请求。

::: thinking
[]{#think-fs-serve label="think-fs-serve"}
阅读`serv.c/serve`{.c}函数的代码，我们注意到函数中包含了一个死循环`for (;;) {...}`{.c}，
为什么这段代码不会导致整个内核进入panic状态？
:::

文件系统支持的请求类型定义在 include/fs.h 中，包含以下几种：

``` {.c linenos=""}
#define FSREQ_OPEN      1
#define FSREQ_MAP       2
#define FSREQ_SET_SIZE  3
#define FSREQ_CLOSE     4
#define FSREQ_DIRTY     5
#define FSREQ_REMOVE    6
#define FSREQ_SYNC      7
```

用户程序在发出文件系统操作请求时，将请求的内容放在对应的结构体中进行消息的传递，
`fs_serv`{.c} 进程 收到其他进行的 IPC 请求后，IPC
传递的消息包含了请求的类型和其他必要的参数，根据请求的类型执行相应的文件
操作（文件的增、删、改、查等），将结果重新通过 IPC 反馈给用户程序。

::: exercise
[]{#exercise-fs-remove label="exercise-fs-remove"} 文件 user/fsipc.c
中定义了请求文件系统时用到的 IPC 操作，user/file.c
文件中定义了用户程序读写、创建、删除 和修改文件的接口。完成 user/fsipc.c
中的 `fsipc_remove`{.c} 函数、user/file.c 中的 `remove`{.c} 函数，以及
fs/serv.c 中的 `serve_remove`{.c} 函数，实现删除指定 路径的文件的功能。
:::

::: thinking
[]{#think-Dev label="think-Dev"} 观察 user/fd.h 中结构体 `Dev`{.c}
及其调用方式。

``` {.c linenos=""}
struct Dev {
    int dev_id;
    char *dev_name;
    int (*dev_read)(struct Fd *, void *, u_int, u_int);
    int (*dev_write)(struct Fd *, const void *, u_int, u_int);
    int (*dev_close)(struct Fd *);
    int (*dev_stat)(struct Fd *, struct Stat *);
    int (*dev_seek)(struct Fd *, u_int);
};
```

综合此次实验的全部代码，思考这样的定义和使用有什么好处。
:::

## 正确结果展示

在 init/init.c 中启动一个 fstest 进程和文件系统服务进程：

``` {.c linenos=""}
    ENV_CREATE(user_fstest);
    ENV_CREATE(fs_serv);
```

就能开始对文件系统的检测，运行文件系统服务，等待应用程序的请求。注意：我们必须将文件系统进程作为1号进程启动，其原因是我们在user/fsipc.c
中定义的文件系统ipc请求的目标 env_id 为1。

::: note
使用 gxemul -E testmips -C R3000 -M 64 -d gxemul/fs.img elf-file 运行
(其中 elf-file 是你编译生成的 vmlinux 文件的路径)。
:::

``` {.text linenos=""}
FS is running
FS can do I/O
superblock is good
diskno: 0
diskno: 0
read_bitmap is good
diskno: 0
alloc_block is good
file_open is good
file_get_block is good
file_flush is good
file_truncate is good
diskno: 0
file rewrite is good
serve_open 00000800 ffff000 0x2
open is good
read is good
diskno: 0
serve_open 00000800 ffff000 0x0
open again: OK
read again: OK
file rewrite is good
file remove: OK
```

## 任务列表

-   [**[完成 sys_write_dev 和
    sys_read_dev]{style="color: baseB"}**](#exercise-syscall-dev)

-   [**[完成 fs/ide.c]{style="color: baseB"}**](#exercise-ide)

-   [**[完成 free_block]{style="color: baseB"}**](#exercise-free-block)

-   [**[完成 create_file]{style="color: baseB"}**](#exercise-fsformat)

-   [**[完成 diskaddr]{style="color: baseB"}**](#exercise-diskaddr)

-   [**[完成 map_block 和
    unmap_block]{style="color: baseB"}**](#exercise-map-block)

-   [**[完成 dir_lookup]{style="color: baseB"}**](#exercise-dir-lookup)

-   [**[完成 open]{style="color: baseB"}**](#exercise-fs-open)

-   [**[完成 read]{style="color: baseB"}**](#exercise-fs-read)

-   [**[实现删除指定路径的文件的功能]{style="color: baseB"}**](#exercise-fs-remove)

## 实验思考

-   [**[Unix /proc 文件系统]{style="color: baseB"}**](#think-proc)

-   [**[设备操作与高速缓存]{style="color: baseB"}**](#think-fs-cache)

-   [**[文件系统支持的单个文件的最大体积]{style="color: baseB"}**](#think-filesize)

-   [**[一个磁盘块最多存储的文件控制块及一个目录最多子文件]{style="color: baseB"}**](#think-filenum)

-   [**[磁盘最大容量]{style="color: baseB"}**](#think-disksize)

-   [**[磁盘映射与用户空间]{style="color: baseB"}**](#think-diskmap-userspace)

-   [**[文件描述符、打开文件与内存的映射]{style="color: baseB"}**](#think-fd-mem)

-   [**[struct Fd \* 到 struct Filefd \*
    的转换]{style="color: baseB"}**](#think-Filefd-Fd)

-   [**[文件系统的中的结构体]{style="color: baseB"}**](#think-structures)

-   [**[文件系统服务进程运行机制]{style="color: baseB"}**](#think-fs-serve)

-   [**[Dev结构体的定义与调用]{style="color: baseB"}**](#think-Dev)
