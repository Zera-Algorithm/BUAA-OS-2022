# 初识操作系统

## 实验目的

1.  认识操作系统实验环境

2.  掌握操作系统实验所需的基本工具

在本章中，我们需要去了解实验环境，熟悉Linux
操作系统（Ubuntu），了解控制终端，掌握一些常用工具并能够脱离可视化界面进行工作。本章节难度不大，旨在让大家熟悉操作系统实验环境的各类工具，为后续实验奠定基础。

## 初识实验

工欲善其事必先利其器，我们需要对实验环境和工具有足够的了解，才能顺利完成实验工作。下面来熟悉一下实验环境。

### 了解实验环境

实验环境整体配置如下：

-   操作系统：Linux虚拟机，Ubuntu

-   硬件模拟器：Gxemul

-   编译器：GCC

-   版本控制：git

Ubuntu操作系统是一款开源的GNU/Linux操作系统，是目前较为流行的几个Linux发行版之一。GNU
（GNU is Not
Unix的递归缩写）是一套开源计划，为我们带来了大量开源软件；Linux：严格意义上指代Linux内核，基于该内核的操作系统众多，具有免费、可靠、安全、稳定、多平台等特点。

GXemul，一款计算机架构仿真器，可以模拟所需硬件环境，例如我们实验需要的MIPS架构下的CPU。

GCC，一套免费、开源的编译器，诞生并服务于GNU计划，最初名称为GNU C
Compiler，后来支持了更多编程语言而改名为GNU Compiler
Collection。很多我们熟知的IDE集成开发环境的编译器用的便是GCC，例如Dev-C++，Code::Blocks等。我们的实验将使用基于mips的GCC交叉编译器。

git，一款免费、开源的版本控制系统，我们的实验将用它为大家提供管理、发布、提交、评测等功能。1.5小节将会详细介绍git如何使用。

### ssh---远程访问实验环境

在简单了解实验环境之后，大家可能有个疑问：这些环境全部需要我们自行安装和配置吗？为了简化大家的安装工作，我们已经建立了一个完整的实验集成环境！

本实验总计7个Lab完全依赖于远程的多台虚拟机，最终成果也需要通过这些虚拟机进行提交，所以同学们几乎不用在个人电脑配置实验环境，只需要一个能够支持ssh协议的远程连接工具。

一般Linux或Mac
OS等类Unix操作系统都会附带ssh客户端，即直接在终端使用ssh命令。Windows平台一般不自带ssh客户端，需要下载第三方软件，在这里建议大家使用一款轻量级的开源软件，名为PuTTY的小工具。当然也用很多功能强大的工具，接触过git
for Windows，或对Windows 10有所研究的同学也可以使用git bash的以及Windows
10的几款Linux子系统。

在Host
Name部分填入username@ip便可，username为同学们的学号，IP就是需要登录的虚拟机的IP地址，之后单击open就可以了。

在光标处输入密码，密码初始为同学们的学号，因此建议登录后立刻使用passwd命令更改密码。

如果同学们是在Mac OS的终端或Linux系统的终端，只需输入 ssh username@ip
之后的操作与PuTTY便相同了。

``` {.bash linenos=""}
# username 处填写你的用户名，ip处填写远程主机的地址
$ ssh username@ip
# 之后等候片刻会要求你输入密码，输入的密码不会被显示在屏幕上，输入完成后按回车即可
# 链接后会显示一些欢迎信息，下面是欢迎信息的一个例子
Welcome to Ubuntu 12.04.5 LTS (GNU/Linux 3.13.0-32-generic i686)

 * Documentation:  https://help.ubuntu.com/

  System information as of Tue Aug 11 09:55:40 CST 2015
  System load:  0.0                Processes:           118
  Usage of /:   8.7% of 145.55GB   Users logged in:     0
  Memory usage: 6%                 IP address for eth0: 0.0.0.0
  Swap usage:   0%

  Graph this data and manage this system at:
    https://landscape.canonical.com/

0 packages can be updated.
0 updates are security updates.
# 欢迎信息后会出现命令提示符，等待你输入命令。
```

![PuTTY登录界面](0-2){#fig:0-2 height="8cm"}

::: note
ssh是Secure
Shell的缩写，是一种用于建立安全的远程连接的网络协议。在Unix类系统上被广泛采用。
除了连接到远程网络，目前ssh还有一种较为有趣的用法。当你既想用Windows，又有时需要Unix环境时，
可以利用Windows8及以上版本自带的Hyper-V开启一个Linux虚拟机（不必开启图形界面）。
之后通过ssh客户端连接到本机上的Linux虚拟机上，即可获得一个Unix环境。甚至可以开启X11转发，
即可在Windows上开启一些Linux上的带图形界面的程序，十分方便。
:::

### 接触CLI Shell，告别GUI Shell

当你使用如上方法登录了自己的账号后，就会接触到命令行界面（CLI），会有很多同学不太习惯。可能只有操作系统这门课能让大家学习使用命令行界面了。

简单来说，这就是我们要接触的操作系统Ubuntu。在这里为从未使用过命令行的同学们解释一下，当你阅读这份实验手册时，你所使用的一定是一款拥有图形化界面的操作系统，一般来讲是Windows、Mac
OS、Ubuntu等。而你面前的这个黑黑的东西就是一个没有图形化界面的操作系统，也就是模拟终端，我们为什么没有让大家使用图形化界面呢，原因有三：1.为了锻炼同学们在没有图形界面环境下工作的能力；2.实验任务通过命令行终端全部可以实现；3.减轻虚拟机的压力，可以允许多个同学共享虚拟机。

当然上文的描述是不严谨的，你所面对的并不是真正的"Ubuntu操作系统"，而是它的"壳"（Shell）。一般的，我们把操作系统核心部分称为内核（英文：Kernel），与其相对的就是它最外层的壳（英文：Shell）。Shell是用于访问操作系统服务的用户界面。操作系统shell使用命令行界面（CLI）或图形用户界面（GUI），这个纯文本的界面就是命令行界面，它能接收你发送的命令，如果命令存在于环境中，它便会为你完成相应的功能。

在Ubuntu中，我们默认使用的CLI
shell是bash，也是一款基于GNU的免费、开源软件。

那么接下来我们小试牛刀。

::: exercise
在bash中分别输入

-   echo "Hello Ubuntu"

-   bash --version

-   ls

三条命令，简单思考其回显结果
:::

可以保证的是，当你掌握了本章的知识后，再使用没有图形化界面的操作系统时，将会得心应手。

::: thinking
[]{#think-Shell简析 label="think-Shell简析"}
根据你的使用经验，简单分析CLI Shell，GUI
Shell在你使用过程中的各自优劣（100字以内）
:::

### 获取实验包

登录了课程实验环境后，就要准备开始我们的实验了，好奇的你可能会问：环境有了，工具有了，在哪里完成实验呢？

我们在服务器上为所有同学部署了实验的远程仓库，接下来我们要先进行一条命令来获取实验代码包。

``` {.bash linenos=""}
$ git clone git@ip:id-lab
```

接下来系统会提示如下内容

``` {.bash linenos=""}
Cloning into '16xxxxxx-lab'...
Enter passphrase for key '/home/16xxxxxx_2018_jac/.ssh/id_rsa':
```

这里让你输入你的rsa密钥，也就是每个人的学号。

执行之后若输入ls，你会发现你的主目录下多了一个id-lab，输入如下命令访问它

``` {.bash linenos=""}
$ cd id-lab
```

再使用ls会发现里面空空如也，接下来还需要再执行一条命令

``` {.bash linenos=""}
$ git checkout lab0
```

这样就进入Lab0工作区了，也许你现在对上面的几条命令的功能不太了解，本章的后续内容会为你介绍它们。

## 基础操作介绍

### 命令行

现在我们要正式与那黑黑的界面交流了，命令行界面(Command Line
Interface，简称CLI)中，用户或客户端通过单条或连续命令行的形式向程序发出命令，从而达到与计算机程序进行人机交互的目的。在Linux系统中，命令即是对Linux系统进行管理的一系列命令，其一般格式为：命令名
\[选项\] \[参数\]，其中中括号表示可选，意为可有可无（例如： ls -a
directory）。对于Linux系统来说，这些管理命令是它正常运行的核心，与Windows的命令提示符（CMD）命令类似。

对于刚接触到命令行界面的各位同学来说，由于不清楚基本的Linux命令而不知所措，这是再正常不过的事。不过不要着急，希望大家能够学会并可以熟练使用下面介绍的一些基本操作，相信你们一定会对命令行界面有一个全新的认识。

### Linux基本操作命令

通过ssh连接服务器打开Linux命令行界面后，首先会看到光标前的如下内容，其中@符号前的是用户名，@符号后的是计算机名，冒号后为当前所在的文件目录（/表示根目录root；～表示主目录home，其一般等价于/home/\<user_name\>），最后的\$或#分别表示当前用户为普通用户或超级用户root。

``` {.bash linenos=""}
16xxxxxx_2018_jac@ubuntu:~$
```

``` {.bash linenos=""}
root@ubuntu:~#
```

现在通过键盘输入命令，按回车后即可执行。首先，需要更改自己的用户密码，使用passwd命令即可更改当前用户的密码，注意：输入密码时不会在屏幕上显示密码内容（若输入过程中手抖打错，可重复多按几次退格键清空之前的输入后重新输入）。

``` {.bash linenos=""}
16xxxxxx_2018_jac@ubuntu:~$ passwd
更改16xxxxxx_2018_jac的密码。
（当前）UNIX密码：
输入新的UNIX密码：
重新输入新的UNIX密码：
passwd：已成功更新密码
```

更改完密码后就可以安全的使用此用户来完成工作了。面对一个全新的界面，我们首先要知道当前目录中都有哪些文件，这时就需要使用ls命令，其输出信息可以进行彩色加亮显示，以分区不同类型的文件。ls是使用率较高的命令，其详细信息如下图所示。一般情况下，该命令的参数省略则默认显示该目录下所有文件，所以我们只需使用ls即可看到所有非隐藏文件，若要看到隐藏文件则需要加上-a选项，若要查看文件的详细信息则需要加上-l选项。

``` {.bash linenos=""}
ls - list directory contents
用法:ls [选项]... [文件]...
选项（常用）：
        -a      不隐藏任何以. 开始的项目
        -l      每行只列出一个文件
```

::: note
与ls命令类似的目录查看命令还有tree命令。
:::

然后你就会发现主目录中空空如也。这时可以使用mkdir命令创建文件目录（即Windows系统中的文件夹），该命令的参数为创建新目录的名称，如：mkdir
newdir 为创建一个名为newdir的目录。

``` {.bash linenos=""}
mkdir - make directories
用法:mkdir [选项]... 目录...
```

删除空目录的命令rmdir与其类似，其命令参数为需要删除的空目录的名称，这里要注意只有空目录才可以使用rmdir命令删除。

``` {.bash linenos=""}
rmdir - remove empty directories
用法:rmdir [选项]... 目录...
```

那么目录非空时怎么办呢？这就需要用到rm命令，rm命令可以删除一个目录中的一个或多个文件或目录，也可以将某个目录及其下属的所有文件及其子目录均删除掉。对于链接文件，只是删除整个链接文件，而原有文件保持不变。

``` {.bash linenos=""}
rm - remove files or directories
用法:rm [选项]... 文件...
选项（常用）：
        -r      递归删除目录及其内容
        -f      强制删除。忽略不存在的文件，不提示确认
```

::: note
使用rm命令要格外小心，因为一旦删除了一个文件，就无法再恢复它。所以，在删除文件之前，最好再看一下文件的内容，确定是否真要删除。
:::

此外，rm命令可以用-i选项，这个选项在使用文件扩展名字符删除多个文件时特别有用。使用这个选项，系统会要求你逐一确定是否要删除。这时，必须输入y并按Enter键，才能删除文件。如果仅按Enter键或其他字符，文件不会被删除。与之相对应的就是-f选项，强制删除文件或目录，不询问用户。使用该选项配合-r选项，进行递归强制删除，强制将指定目录下的所有文件与子目录一并删除，可能导致灾难性后果。例如：rm
-rf / 即可强制递归删除全盘文件，绝对不要轻易尝试！

::: note
在需要键入文件名/目录名时，可以使用TAB键补足全名，当有多种补足可能时双击TAB键可以显示所有可能选项。
:::

学会创建目录后，我们就可以进入新建的目录中创建和修改文件来完成工作了。使用cd命令来切换工作目录至dirname，其中dirname的表示法可为绝对路径或相对路径。若目录名称省略，则变换至使用者的主目录home
(等价于cd `~` )。另外，在Linux文件系统中， . 表示当前所在目录， ..
表示当前目录位置的上一层目录，同学们在使用ls -a后看到的目录中的 . 和 ..
即是如此。

``` {.bash linenos=""}
cd - change working directory
用法:cd [路径]
e.g.
        cd .    变更至当前目录
        cd ..   变更至父目录
```

现在，进入新建的目录中后依旧空空如也，那么我们就来创建几个文件。创建文件的方法有很多，可以直接使用重定向输出`>`filename来新建一个空文件，也可以使用文本编辑工具vim或nano等新建文件后保存（使用命令vim
filename或nano
filename，编辑工具的使用方法将在1.4节介绍）。打开后可以直接进行编辑后保存，所以大家可以先在编辑工具中编写代码，然后通过保存新建文件。这里有些问题：重定向输出是什么意思？大于号是什么意思？重定向输出就是使用`>`来改变送出的数据通道，使`>`前命令的输出数据输出到`>`后指定的文件中去。例如：ls
/ `>` filename
可以将根目录下的文件名输出至当前目录下的filename文件中。与之类似的，还有重定向追加输出`>>`，将`>>`前命令的输出数据追加输出到`>>`后指定的文件中去；以及重定向输入`<`，将`<`后指定的文件中的数据输入到`<`前的命令中，同学们可以自己动手实践一下。

在此顺带介绍一下echo命令，echo命令用于在Shell中打印Shell变量的值，或者直接输出指定的字符串。简单来说，就是将echo命令中的参数输出到屏幕上显示。那么将echo命令和重定向相结合会产生什么样的效果呢？请大家自己进行尝试。

好了，现在我们已经创建了新的文件，那么想要查看文件内容要怎么办呢？除了前面说到的编辑工具以外，还有一种简单快速查看文件内容的方法------cat命令。使用cat
filename即可将文件内容输出到屏幕上显示，若要显示行数，添加-n选项即可。

``` {.bash linenos=""}
cat - concatenate files and print
用法:cat [选项]... [文件]...
选项（常用）：
        -n      对输出的所有行编号
```

::: exercise
执行如下命令,并查看结果

-   echo first

-   echo second `>` output.txt

-   echo third `>` output.txt

-   echo forth `>>` output.txt
:::

之后就是对于文件的操作：复制和移动。复制命令为cp，命令的第一个参数为源文件路径，命令的第二个参数为目标文件路径。

``` {.bash linenos=""}
cp - copy files and directories
用法:cp [选项]... 源文件... 目录
选项（常用）：
        -r      递归复制目录及其子目录内的所有内容
```

移动命令为mv，与cp的操作相似。例如：mv file
../file_mv就是将当前目录中的file文件移动至上一层目录中且重命名为file_mv。大家应该已经看出来，在Linux系统中想要对文件进行重命名操作，使用mv
oldname newname命令就可以了。

``` {.bash linenos=""}
mv - move/rename file
用法:mv [选项]... 源文件... 目录
```

在以后的工作中，可能会遇到重复多次用到单条或多条长而复杂命令的情况，初学者可能会想把这些命令保存在一个文件中，以后再打开文件复制粘贴运行。其实可以不用复制粘贴，将文件按照批处理脚本运行即可。简单来说，批处理脚本就是存储了一条或多条命令的文本文件，Linux系统中有一种简单快速执行批处理文件的方法（类似Windows系统中的.bat批处理脚本）------source命令。source命令是bash的内置命令，该命令通常用点命令
.
来替代。这两个命令都以一个脚本为参数，该脚本将作为当前Shell的环境执行，不会启动一个新的子进程，所有在脚本中设置的变量将成为当前Shell的一部分。使用方法如下图所示，其命令格式与之前介绍的命令类似，请同学们自己动手举例尝试。

``` {.bash linenos=""}
source - execute commands in file
用法:source 文件名 [参数]
注：文件应为可执行文件，即为绿色
```

::: thinking
[]{#think-文件的操作 label="think-文件的操作"}
使用你知道的方法（包括重定向）创建下图内容的文件（文件命名为test），将创建该文件的命令序列保存在command文件中，并将test文件作为批处理文件运行，将运行结果输出至result文件中。给出command文件和result文件的内容，并对最后的结果进行解释说明（可以从test文件的内容入手）.
具体实现的过程中思考下列问题: echo echo Shell Start 与 echo 'echo Shell
Start'效果是否有区别; echo echo \$c\>file1 与 echo 'echo
\$c\>file1'效果是否有区别.
:::

![文件内容](0-15){#fig:0-15 height="8cm"}

此外，还有两种常用的查找命令：find和grep

使用find命令并加上-name选项可以在当前目录下递归地查找符合参数所示文件名的文件，并将文件的路径输出至屏幕上。

``` {.bash linenos=""}
find - search for files in a directory hierarchy
用法:find -name 文件名
```

grep是一种强大的文本搜索工具，它能使用正则表达式搜索文本，并把匹配的行打印出来。简单来说，grep命令可以从文件中查找包含pattern部分字符串的行，并将该文件的路径和该行输出至屏幕。[当你需要在整个项目目录中查找某个函数名、变量名等特定文本的时候，grep
将是你手头一个强有力的工具。]{style="color: red"}

``` {.bash linenos=""}
grep - print lines matching a pattern
用法:grep [选项]... PATTERN [FILE]...
选项（常用）：
        -a      不忽略二进制数据进行搜索
        -i      忽略文件大小写差异
        -r      从文件夹递归查找
    -n    显示行号
```

以上就是Linux系统入门级的部分常用操作命令以及这些命令的常用选项。如果想要查看这些命令的其他功能选项或者新命令的详尽说明，就需要使用Linux下的帮助命令------man命令，通过man指令可以查看Linux中的指令帮助、配置文件帮助和编程帮助等信息。

``` {.bash linenos=""}
man - manual
用法:man page
e.g.
    man ls
```

最后，还有下面几个常用的快捷键介绍给同学们。

-   Ctrl+C 终止当前程序的执行

-   Ctrl+Z 挂起当前程序

-   Ctrl+D 终止输入（若正在使用Shell，则退出当前Shell）

-   Ctrl+I 清屏

其中，Ctrl+Z挂起程序后会显示该程序挂起编号，若想要恢复该程序可以使用fg
\[job_spec\]即可，job_spec即为挂起编号，不输入时默认为最近挂起进程。

对其他内容感兴趣的同学可以自行查阅网上资料，或用man命令看帮助手册进行学习和了解。

::: note
在多数shell中，四个方向键也是有各自特定的功能的：←和→可以控制光标的位置，↑和↓可以切换最近使用过的命令
:::

### linux操作补充

tree指令可以根据文件目录生成文件树，作用类似于ls。

``` {.bash linenos=""}
tree
用法: tree [选项] [目录名]
选项(常用)：
        -a 列出全部文件
        -d 只列出目录
```

locate也是查找文件的指令，与find的不同之处在于: find 是去硬盘找，locate
只在/var/lib/slocate资料库中找。locate的速度比find快，它并不是真的查找文件，而是查数据库，所以locate的查找并不是实时的，而是以数据库的更新为准，一般是系统自己维护，也可以手工升级数据库。

``` {.bash linenos=""}
locate
用法: locate [选项] 文件名
```

Linux的文件调用者权限分为三类 : 文件拥有者、群组、其他。利用 chmod
可以藉以控制文件如何被不同人所调用。

``` {.bash linenos=""}
chmod
用法: chmod 权限设定字串 文件...
权限设定字串格式 :
        [ugoa...][[+-=][rwxX]...][,...]
```

其中： u 表示该文件的拥有者，g 表示与该文件的拥有者属于同一个群组，o
表示其他以外的人，a 表示这三者皆是。
+表示增加权限、-表示取消权限、=表示唯一设定权限。
r表示可读取，w表示可写入，x表示可执行，X表示只有当该文件是个子目录或者该文件已经被设定过为可执行。

此外chmod也可以用数字来表示权限，格式为：

``` {.bash linenos=""}
chmod abc 文件
```

abc为三个数字，分别表示拥有者，群组，其他人的权限。r=4，w=2，x=1，用这些数字的加和来表示权限。例如
chmod 777 file和chmod a=rwx file效果相同。

diff命令用于比较文件的差异。

``` {.bash linenos=""}
diff [选项] 文件1 文件2
常用选项
-b 不检查空格字符的不同
-B 不检查空行
-q 仅显示有无差异，不显示详细信息
```

sed是一个文件处理工具，可以将数据行进行替换、删除、新增、选取等特定工作。

``` {.bash linenos=""}
sed
sed  [选项] ‘命令’ 输入文本
选项(常用):
        -n:使用安静模式。在一般 sed 的用法中，输入文本的所有内容都会被输出。
           加上 -n 参数后，则只有经过sed 处理的内容才会被显示。
        -e: 进行多项编辑，即对输入行应用多条sed命令时使用。
        -i:直接修改读取的档案内容，而不是输出到屏幕。使用时应小心。
命令(常用)：
        a :新增， a后紧接着\\，在当前行的后面添加一行文本
        c :取代， c后紧接着\\， 用新的文本取代本行的文本
        i :插入， i后紧接着\\，在当前行的上面插入一行文本
        d :删除，删除当前行的内容
        p :显示，把选择的内容输出。通常 p 会与参数 sed -n 一起使用。
        s :取代，格式为s/re/string，re表示正则表达式，string为字符串，
           功能为将正则表达式替换为字符串。
```

举例

``` {.bash linenos=""}
sed -n '3p' my.txt
#输出my.txt的第三行
sed '2d' my.txt
#删除my.txt文件的第二行
sed '2,$d' my.txt
#删除my.txt文件的第二行到最后一行
sed 's/str1/str2/g' my.txt
#在整行范围内把str1替换为str2。
#如果没有g标记，则只有每行第一个匹配的str1被替换成str2
sed -e '4a\str ' -e 's/str/aaa/' my.txt
#-e选项允许在同一行里执行多条命令。例子的第一条是第四行后添加一个str，
#第二个命令是将str替换为aaa。命令的执行顺序对结果有影响。
```

awk是一种处理文本文件的语言，是一个强大的文本分析工具。这里只举几个简单的例子，学有余力的同学可以自行深入学习。

``` {.bash linenos=""}
awk '$1>2 {print $1,$3}' my.txt
```

这个命令的格式为awk 'pattern action'
file，pattern为条件，action为命令，file为文件。命令中出项的\$n代表每一行中用空格分隔后的第n项。所以该命令的意义是文件my.txt中所有第一项大于2的行，输出第一项和第三项。

``` {.bash linenos=""}
awk -F, '{print $2}'   my.txt
```

-F选项用来指定用于分隔的字符，默认是空格。所以该命令的\$n就是用，分隔的第n项了。

tmux是一个优秀的终端复用软件，可用于在一个终端窗口中运行多个终端会话。窗格、窗口和会话是tmux的三个基本概念，一个会话可以包含多个窗口，一个窗口可以分割为多个窗格。突然中断退出后tmux仍会保持会话，通过进入会话可以直接从之前的环境开始工作。

窗格操作

tmux的窗格（pane）可以做出分屏的效果。

-   ctrl+b %
    垂直分屏(组合键之后按一个百分号)，用一条垂线把当前窗口分成左右两屏。

-   ctrl+b \"
    水平分屏(组合键之后按一个双引号)，用一条水平线把当前窗口分成上下两屏。

-   ctrl+b o 依次切换当前窗口下的各个窗格。

-   ctrl+b Up\|Down\|Left\|Right 根据按箭方向选择切换到某个窗格。

-   ctrl+b Space (空格键)
    对当前窗口下的所有窗格重新排列布局，每按一次，换一种样式。

-   ctrl+b z 最大化当前窗格。再按一次后恢复。

-   ctrl+b x
    关闭当前使用中的窗格，操作之后会给出是否关闭的提示，按y确认即关闭。

窗口操作

每个窗口（window）可以分割成多个窗格（pane）。

-   ctrl+b c 创建之后会多出一个窗口

-   ctrl+b p 切换到上一个窗口。

-   ctrl+b n 切换到下一个窗口。

-   ctrl+b 0 切换到0号窗口，依此类推，可换成任意窗口序号

-   ctrl+b w 列出当前session所有串口，通过上、下键切换窗口

-   ctrl+b & 关闭当前window，会给出提示是否关闭当前窗口，按下y确认即可。

会话操作

一个会话(session)可以包含多个窗口(window)

-   tmux new -s 会话名 新建会话

-   ctrl+b d 退出会话，回到shell的终端环境

-   tmux ls 终端环境查看会话列表

-   tmux a -t 会话名 从终端环境进入会话

-   tmux kill-session -t 会话名 销毁会话

### shell脚本

当有很多想要执行的linux指令来完成复杂的工作，或者有一个或一组指令会经常执行时，我们可以通过shell脚本来完成。本节将学习使用bash
shell。
首先创建一个文件my.sh，向其中写入一些内容(写入内容可以查看后面的vim和nano教学)：

``` {.bash linenos=""}
#!/bin/bash
#balabala
echo "Hello World!"
```

我们自己创建的shell脚本一般是不能直接运行的，需要添加运行权限: chmod +x
my.sh
添加权限之后，我们可以使用bash命令来运行这个文件，把我们的文件作为它的参数：

``` {.bash linenos=""}
bash my.sh
```

或者使用之前介绍的指令source:

``` {.bash linenos=""}
source my.sh
```

这样有些不方便，一种更方便的用法是：

``` {.bash linenos=""}
./my.sh
```

在脚本中我们通常会加入#!/bin/bash到文件首行，以保证我们的脚本默认会使用bash。第二行的内容是注释，以#开头。第三行是命令。

shell传递参数与函数

我们可以向shell脚本传递参数。 my2.sh的内容

``` {.bash linenos=""}
echo $1
```

执行命令

``` {.bash linenos=""}
./my2.sh msg
```

则shell会执行echo msg这条指令。
\$n就代表第几个参数，而\$0也就是命令，在例子中就是./my2.sh。
除此之外还有一些可能有用的符号组合

-   \$# 传递的参数个数

-   \$\* 一个字符串显示传递的全部参数

shell中的函数也用类似的方式传递参数。

``` {.bash linenos=""}
function 函数名 ()
{
    commands
    [return int]
}
```

function或者()可以省略其中一个。 举例

``` {.bash linenos=""}
fun(){
    echo $1
    echo $2
    echo "the number of parameters is $#"
}
fun 1 str2
```

shell流程控制
shell脚本中也可以使用分支和循环语句。学有余力的同学可以学习一下。

if的格式：

``` {.bash linenos=""}
if condition
then
    command1
    command2
    ...
fi
```

或者写到一行

``` {.bash linenos=""}
if condition; then command1; command2; … fi
```

举例

``` {.bash linenos=""}
a=1
if [ $a -ne 1 ]; then echo ok; fi
```

条件部分可能会让同学们感到疑惑。中括号包含的条件表达式的-ne是关系运算符，它们和c语言的比较运算符对应如下。

  ----- ----- --------------------
  -eq   ==    (equal)
  -ne   !=    (not equal)
  -gt   \>    (greater than)
  -lt   \<    (less than)
  -ge   \>=   (greater or equal)
  -le   \<=   (less or equal)
  ----- ----- --------------------

条件也可以写true或false。
变量除了使用自己定义的以外，还有一个比较常用的是\$?，代表上一个命令的返回值。比如刚执行完diff后，若两文件相同\$?为0。

实际上condition的位置上也是命令，当返回值为0时执行。左中括号是指令，\$a、-ne、1、\]是指令的选项，关系成立返回0。true则是直接返回0。condition也可以用diff
file1 file2来填。

while语句格式如下

``` {.bash linenos=""}
while condition
do
    commands
done
```

while语句可以使用continue和break这两个循环控制语句。

例如创建10个目录，名字是file1到file9。

``` {.bash linenos=""}
a=1
while [ $a -ne 10 ]
do
    mkdir file$a
    a=$[$a+1]
done
```

有两点请注意：流程控制的内容不可为空。运算符和变量之间要有空格。

除了以上内容，shell还有for，case，else语句，逻辑运算符等语法，内容有兴趣的同学可以自行了解。

### 重定向和管道

这部分我们将学习如何实现linux命令的输入输出怎样定向到文件，以及如何将多个指令组合实现更强大的功能。
shell使用三种流：

-   标准输入：stdin ，由0表示

-   标准输出：stdout，由1表示

-   标准错误：stderr，由2表示

重定向和管道可以重定向以上的流。
重定向在前面已经有过介绍，这里只做一点补充。
2\>\>可以将标准错误重定向。三种流可以同时重定向，举例:

``` {.bash linenos=""}
command < input.txt 1>output.txt 2>err.txt
```

管道：

管道符号"\|"可以连接命令：

``` {.bash linenos=""}
command1 | command2 | command3 | ...
```

以上内容是将command1的stdout发给command2的stdin，command2的stdout发给command3的stdin，依此类推。
举例:

``` {.bash linenos=""}
cat my.sh | grep "Hello"
```

上述命令的功能为将my.sh的内容输出给grep指令，grep在其中查找字符串。

``` {.bash linenos=""}
cat < my.sh | grep "Hello" > output.txt
```

上述命令重定向和管道混合使用，功能为将my.sh的内容作为cat指令参数，cat指令stdout发给grep指令的stdin，grep在其中查找字符串，最后将结果输出到output.txt。

### gxemul的使用

gxemul是我们运行MOS操作系统的仿真器，它可以帮助我们运行和调试MOS操作系统。直接输入gxemul会显示帮助信息。

gxemul运行选项:

-   -E 仿真机器的类型

-   -C 仿真cpu的类型

-   -M 仿真的内存大小

-   -V 进入调试模式

举例：

``` {.bash linenos=""}
gxemul -E testmips -C R3000 -M 64 vmlinux    #用gxemul运行vmlinux
gxemul -E testmips -C R3000 -M 64 -V vmlinux
#以调试模式打开gxemul，对vmlinux进行调试（进入后直接中断，
#输入continue或step才会继续运行，在此之前可以进行添加断点等操作）
```

进入gxemul后使用Ctrl-C可以中断运行。中断后可以进行单步调试，执行如下指令：

-   breakpoint add addr添加断点

-   continue 继续执行

-   step \[n\] 向后执行n条汇编指令

-   lookup name\|addr 通过名字或地址查找标识符

-   dump \[addr \[endaddr\]\] 查询指定地址的内容

-   reg \[cpuid\]\[,c\] 查看寄存器内容，添加",c"可以查看协处理器

-   help 显示各个指令的作用与用法

-   quit 退出

(以上中括号表示内容可以没有)

更多gxemul相关的信息参考<http://gavare.se/gxemul/gxemul-stable/doc/index.html>

## 实用工具介绍

学会了Linux基本操作命令，我们就可以得心应手地使用命令行界面的Linux操作系统了，但是想要使用Linux系统完成工作，光靠命令行还远远不够。在开始动手阅读并修改代码之前，我们还需要掌握一些实用工具的使用方法。这里我们首先介绍两种常用的文本编辑器：nano和vim。

### nano

我们先从一个简易的工具入手：nano。nano的主界面如下图所示，所有基本的操作都被罗列在下面。nano
较为容易上手，但功能相对有限。如果你需要更为强大的功能，那么推荐去学习和使用vim。

![Nano界面及基础介绍](0-20){#fig:0-20 height="8.5cm"}

### vim

vim被誉为编辑器之神，是程序员为程序员设计的编辑器，编辑效率高，十分适合编辑代码，其界面如图[1.4](#fig:0-21){reference-type="ref"
reference="fig:0-21"}所示。对于习惯了图形化界面文本编辑软件的同学们来说，刚接触vim时一定会觉得非常不习惯非常不顺手，所以在这里给大家总结了一些常用的基本操作以助入门。同时，推荐给大家一篇质量很高的vim教程------《简明vim练级攻略》(<http://coolshell.cn/articles/5426.html/>)，只需要十多分钟的阅读，你就可以了解
vim的所有基本操作。当然，想要完全掌握它需要相当长的时间去练习，而如果你只想把vim当成记事本用的话，几分钟的学习足矣。其他的内容网上有很多的资料，随用随查即可。此外，还可以观看北航MOOC网站上的vim教学视频。

![vim界面及基础介绍](0-21){#fig:0-21 height="11cm"}

### GCC

在没有IDE的情况下，使用GCC编译器是一种简单快捷生成可执行文件的途径，只需一行命令即可将C源文件编译成可执行文件。其常用的使用方法如下图所示，同学们可以自己动手实践，写一些简单的C代码来编译运行。如果想要同时编译多个文件，可以直接用-o选项将多个文件进行编译链接：gcc
testfun.c test.c -o test
，也可以先使用-c选项将每个文件单独编译成.o文件，再用-o选项将多个.o文件进行连接：gcc
-c testfun.c -\> gcc -c test.c -\> gcc testfun.o test.o -o test
，两者等价。

``` {.bash linenos="" breaklines=""}
语法:gcc [选项]... [参数]...
选项（常用）：
        -o      指定生成的输出文件
        -S      将C代码转换为汇编代码
        -Wall       显示警告信息
        -c      仅执行编译操作，不进行链接操作
        -M      列出依赖
        -include filename 编译时用来包含头文件，功能相当于在代码中使用#include<filename>
        -Ipath      编译时指定头文件目录，使用标准库时不需要指定目录，-I参数可以用相对路径，比如头文件在当前目录，可以用-I.来指定
参数：
    C源文件：指定C语言源代码文件
e.g.

$ gcc test.c
#默认生成名为a.out的可执行文件
#Windows平台为a.exe

$ gcc test.c -o test
#使用-o选项生成名为test的可执行文件
#Windows平台为test.exe
```

### Makefile

当我们了解像操作系统这样的大型软件的时候，会面临一个问题：这些代码应当从哪里开始阅读？答案是：Makefile。当你不知所措的时候，
从Makefile开始往往会是一个不错的选择。什么又是Makefile呢？什么是make呢？
make工具一般用于维护软件开发的工程项目。它可以根据时间戳自动判断项目的哪些部分是需要重新编译，每次只重新编译必要的部分。make工具会读取Makefile文件，
并根据Makefile的内容来执行相应的操作。Makefile类似于大家以前接触过的VC工程文件。只不过不像VC那样有图形界面，而是直接用类似脚本的方式实现。

相较于VC工程而言，Makefile具有更高的灵活性（当然，高灵活性的代价就是学习成本会有所提升），可以方便地管理大型的项目。
而且Makefile理论上支持任意的语言，只要其编译器可以通过shell命令来调用。当你的项目可能会混合多种语言，有着复杂的构建流程的时候，
Makefile便能展现出它真正的威力来。
为了使大家更为清晰地了解Makefile的基本概念，我们来写一个简单的Makefile。假设有一个Hello
World程序需要编译。让我们从头开始，如果没有Makefile，直接动手编译这个程序，需要下面这样一个指令：

``` {.bash linenos=""}
# 直接使用gcc编译Hello World程序
$ gcc -o hello_world hello_world.c
```

那么，如果我们想把它写成Makefile，应该怎么办呢？Makefile最基本的格式是这样的:

``` {.make linenos=""}
target: dependencies
    command 1
    command 2
    ...
    command n
```

其中，target是我们构建(Build)的目标，可以是真的目标文件、可执行文件，也可以是一个标签。而dependencies是构建该目标所需的其它文件或其他目标。之后是构建出该目标所需执行的指令。
有一点尤为需要注意：每一个指令(command)之前必须有一个TAB。这里必须使用TAB而不能是空格，否则make会报错。

我们通过在makefile中书写这些显式规则来告诉make工具文件间的依赖关系：如果想要构建target，那么首先要准备好dependencies，接着执行command中的命令，然后target就会最终完成。在编写完恰当的规则之后，只需要在shell中输入make
target(目标名)，即可执行相应的命令、生成相应的目标。

还记得我们之前提到过make工具是根据时间戳来判断是否编译的吗？make只有在依赖文件中存在文件的修改时间比目标文件的修改时间晚的时候(也就是对依赖文件做了改动)，shell命令才被会执行，编译生成新的目标文件。

我们的简易Makefile可以写成如下的样子。之后执行make
all或是make，即可产生hello_world这个可执行文件。

``` {.make linenos=""}
all: hello_world.c
    gcc -o hello_world hello_world.c
```

在lab0阶段，建议同学们自己尝试写一个简单的Makefile文件，体会make工具的妙处。这里为同学们提供几个网址供大家参考：

1.  <http://www.cs.colby.edu/maxwell/courses/tutorials/maketutor>

2.  <http://www.gnu.org/software/make/manual/make.html#Reading-Makefiles>

3.  <https://seisman.github.io/how-to-write-makefile/introduction.html>

第一个网站详细描述了如何从一个简单的Makefile入手，逐步扩充其功能，最终写出一个相对完善的Makefile，同学们可以从这一网站入手，仿照其样例，写出一个自己的Makefile。
第二个网站提供了一份完备的Makefile语法说明，今后同学们在实验中遇到了未知的Makefile语法，均可在这一网站上找到满意的答案。
第三个网站则是一份入门级中文教程，同学们可以通过这个网站了解和学习Makefile简单的基础知识，对Makefile各个重要部分有一个大致的了解。

## git专栏--轻松维护和提交代码

我们的实验是通过git版本控制系统进行管理，在本章的最后，我们就来了解一下git相关的内容。

### git是什么？

说到git是什么，我们就得需要了解一下什么是版本控制。最原始的版本控制是纯手工完成：修改文件，保存文件副本。有时候偷懒省事，保存副本时命名比较随意，时间长了就不知道哪个是新的，哪个是老的了，即使知道新旧，可能也不知道每个版本是什么内容，相对上一版作了什么修改了，当几个版本过去后，很可能就是下面的样子了。

![手工版本控制](0-23){#fig:0-23 height="6cm"}

当然，有些时候，我们不是一个人写代码，很多工程项目也往往是由多个人一起完成。在多人项目开始时，分工、制定计划、埋头苦干，看起来一切井然有序，但后期可能会让人头疼不已。本质原因在于每个人都会对项目的内容进行改动，结果最后有了这样一副情形：A把添加完功能的项目打包发给了B，然后自己继续添加功能。一天后，B把他修改后的项目包又发给了A，这时A就必须非常清楚发给B之后到他发回来的这段期间，自己究竟对哪里做了改动，然后还要进行合并，这相当困难。

这时我们发现了一个无法避免的事实：如果每一次小小的改动，开发者之间都要相互通知，那么一些错误的改动将会令我们付出很大的代价：一个错误的改动要频繁在几方同时通知纠正。如果一次性改动了大幅度的内容，那么只有概览了项目的很多文件后才能知道改动在哪儿，也才能合并修改。项目只有10个文件时还好接受，如果是40个，
60个，80个呢。

于是就产生了下面这些需求，我们希望有一款软件：

-   自动帮我记录每次文件的改动，而且最好是有撤回的功能，改错了一个东西，可以轻松撤销。

-   还可以支持多人协作编辑，有着简洁的指令与操作。

-   最好能像时光机一样，能在不满意的时候恢复到以前的状态！

-   如果想查看某次改动，可以在软件里直接找到。

版本控制系统就是这样一种的系统。而
git，则是目前世界上最先进的分布式版本控制系统之一。

git是由Linux的缔造者Linus
Torvalds创造，最开始也是用于管理自己的Linux开发过程。他对于git的解释是：The
stupid content
tracker,傻瓜内容追踪器。git一词本身也是个俚语，大概表示"混帐"。

::: note
版本控制是一种记录若干文件内容变化, 以便将来查阅特定版本修订情况的系统。
:::

### git基础指引

git在实际应用中究竟是怎样的一个系统呢？我们先从git最基础的指令讲起，通过前几节的学习，接下来请进行如下操作：

1.  创建一个名为learngit的文件夹

2.  进入learngit目录

3.  输入git init

4.  用ls指令添加适当参数看看多了什么

我们会发现，新建的目录下面多了一个叫.git的隐藏目录，这个目录就是git版本库，更多的被称为仓库(repository)。需要注意的是，**在我们的实验中是不会对.git文件夹进行任何直接操作的，所以不要轻易动这个文件夹中的任何文件**

在init执行完后，我们就拥有了一个仓库。我们建立的learngit文件夹就是git里的工作区。目前我们除了.git版本库目录以外空无一物。

::: note
在我们的MOS操作系统实验中不需要使用到 git init
命令，每个人一开始就都有一个名为 16xxxxxx-lab 的版本库，包含了 lab0
的实验内容。
:::

既然工作区里空荡荡的，那我们来加些东西：
用你已知的方法在工作区中建立一个readme.txt，内容为"BUAA_OSLAB"

当然这只是创建了一个文件而已，下面我们要将它添加至版本库，执行如下内容

``` {.bash linenos=""}
$ git add readme.txt
```

注意，到这里还没有结束，你可能会想，那我既然都把 readme.txt
加入了，难道不是已经提交到版本库了吗？但事实就是这样，git------同其他大多数版本控制系统一样，add之后需要再执行一次提交操作，提交操作的命令如下:

``` {.bash linenos=""}
$ git commit
```

如果不带任何附加选项，执行后会弹出一个说明窗口，如下所示：

``` {.bash linenos=""}
GNU nano 2.2.6    文件： /home/13061193/13061193-lab/.git/COMMIT_EDITMSG

Notes to test.
# 请为您的变更输入提交说明。以 '#' 开始的行将被忽略，而一个空的提交
# 说明将会终止提交。
# 位于分支 master
# 您的分支与上游分支 'origin/master' 一致。
#
# 要提交的变更：
#       修改:         readme.txt
#

                                 [ 已读取9 行 ]
^G 求助      ^O 写入      ^R 读档      ^Y 上页      ^K 剪切文字  ^C 游标位置
^X 离开      ^J 对齐      ^W 搜索      ^V 下页      ^U 还原剪切  ^T 拼写检查
```

在上面里书写的 **Notes to test**. 是我们本次提交所附加的说明。

注意，弹出的窗口中我们**必须**得添加本次 commit
的说明，这意味着不能提交空白说明，否则这次提交不会成功。而且在添加评论之后，可以按提示按键来成功保存。

::: note
初学者一般不太重视 git commit
内容的有效性，总是使用无意义的字符串作为说明提交。但以后你可能就会发现自己写了一个自己看得懂，别人也能看得懂提交说明是多么庆幸。所以尽量让你的每次提交显得有意义，比如"fixedabug
in \..." 这样的描述，顺便推荐一条命令：git commit
--amend，这条命令可以重新书写你最后一次 commit 的说明。
:::

可能这样的窗口提交方式比较繁琐，我们可以采用一种简洁的方式：

``` {.bash linenos=""}
$ git commit -m [comments]
```

\[comments\]格式为**"评论内容"**，上述的提交过程可以简化为下面一条指令

``` {.bash linenos=""}
$ git commit -m "Notes to test."
```

如果提交之后看到类似的提示就说明提交成功了。

``` {.bash linenos=""}
[master 955db52] Notes to test.
 1 file changed, 1 insertion(+), 1 deletion(-)
```

从我们本次提交中可以得到以下信息，可能现在你还不能完全理解这些信息代表的意思，但是没关系，之后会讲解。

-   本次提交的分支[]{#分支 label="分支"}是 master

-   本次提交的ID是 955db52

-   提交说明是 Notes to test

-   共有1个文件相比之前发生了变化：1行的添加与1行的删除行为

但是在我们实验中，第一次提交可能不会这么一帆风顺，第一次提交往往会出现下面的提示：

``` {.c linenos=""}
*** Please tell me who you are.

Run

  git config --global user.email "you@example.com"
  git config --global user.name "Your Name"

# to set your account’s default identity.
# Omit --global to set the identity only in this repository.
```

相信大家从第一句也能推测出，这是要求我们设置提交者身份。

::: note
从上面我们也知道了，我们可以用

git config \--global user.email \"you@example.com\"

git config \--global user.name \"Your Name\"

这两条命令设置我们的名字和邮箱，在我们的实验中对这两个没有什么要求，大家随性设置就好，给个示例：

git config \--global user.email \"qianlxc@126.com\"

git config \--global user.name \"Qian\"
:::

现在你已设置了提交者的信息，提交者信息是为了告知所有负责该项目的人每次提交是由谁提交的，并提供联系方式以进行交流

那么做一下这个小练习来快速上手 git 的使用吧。

::: exercise
-   在/home/16xxxxxx_2018_jac/learngit（已init）目录下创建一个名为README.txt的文件。这时使用
    git status \> Untracked.txt 。

-   在 README.txt 文件中随便写点什么，然后使用刚刚学到的 add
    命令，再使用 git status \> Stage.txt 。

-   之后使用上面学到的 git 提交有关的知识把 README.txt
    提交，并在提交说明里写入自己的学号。

-   使用 cat Untracked.txt 和 cat
    Stage.txt，对比一下两次的结果，体会一下README.txt
    两次所处位置的不同。

-   修改 README.txt 文件，再使用 git status \> Modified.txt 。

-   使用 cat Modified.txt ，观察它和第一次 add 之前的 status
    一样吗，思考一下为什么？
:::

::: note
git status 是一个查看当前文件状态的有效指令，而 git log
则是提交日志，每commit一次，git 会在提交日志中记录一次。git log
将在后面的恢复机制中发挥很大的作用。
:::

相信你做过上述实验后，心里还是会有些疑惑，没关系，我们来一起看一下刚才得到的
Untracked.txt，Stage.txt 和 Modified.txt 的内容

``` {.bash linenos=""}
Untracked.txt的内容如下

# On branch master
# Untracked files:
#   (use "git add <file>..." to include in what will be committed)
#
#   README.txt
nothing added to commit but untracked files present (use "git add" to track)

Stage.txt的内容如下

# On branch master
# Changes to be committed:
#   (use "git reset HEAD <file>..." to unstage)
#
#   new file:   README.txt
#

Modified.txt的内容如下

# On branch master
# Changes not staged for commit:
#   (use "git add <file>..." to update what will be committed)
#   (use "git checkout -- <file>..." to discard changes in working directory)
#
#   modified:   README.txt
#
no changes added to commit (use "git add" and/or "git commit -a")
```

通过仔细观察，我们看到第一个文本文件中第 2 行是：Untracked
files，而第二个文本文件中第二行内容是：Changes to be
committed，而第三个则是 Changes not staged for
commit。这三种不同的提示意味着什么，需要你通过后面的学习找到答案。

我们开始时已经介绍了 git 中的工作区的概念，接下来的内容就是 git
中最核心的概念，为了能自如运用 git 中的命令，一定要仔细学习。

### git的对象库和三个目录

想要了解git如何维护文件的版本信息，就需要知道git如何存储和索引这些文件。git的对象库是在.git/objects中。所有需要版本控制的每个文件的内容会压缩成二进制文件，压缩后的二进制文件称为一个
git对象，保存在.git/objects目录。git计算当前文件内容的SHA1哈希值（长度40的字符串），作为该对象的文件名。下面就是一个新生成的git对象文件。

``` {.bash linenos=""}
c64694584cf480b01273f2c729fd8b6b7c320c
```

git的对象库只保存了文件信息，而没有目录结构，因此我们的本地仓库由 git
还维护了三个目录。第一个目录我们称为工作区，在本地计算机文件系统中能看到这个目录，它保存实际文件。第二个目录是暂存区（英语是Index，有时也称
Stage），是.git/index文件，用于暂存工作区中被追踪的文件。将工作区的文件内容放入暂存区，可理解为给工作区做了一个快照（snapshot）。当工作区文件被破坏的时候，可以根据暂存区的快照对工作区进行恢复。最后一个目录是版本库，是
HEAD
指向最近一次提交（commit）后的结果。在项目开发的一定阶段，将可以在将暂存区的内容归档，放入版本库。

在我们的.git 目录中，文件.git/index
实际上就是一个包含文件索引的目录树，像是一个虚拟的工作区。在这个虚拟工作区的目录树中，记录了文件名、文件的状态信息（时间戳、文件长度等），但是文件的内容并不存储其中，而是保存在
git
对象库（.git/objects）中，文件索引建立了文件和对象库中对象实体之间的对应。下面这个图展示了工作区、暂存区和版本库之间的关系，并说明了不同操作带来的影响。

![工作区、暂存区和版本库](git-stage){#fig:git-stage width="15cm"}

-   图中 objects 标识的区域为 git 的对象库，实际位于 \".git/objects\"
    目录下。

-   图中左侧为工作区，右侧为暂存区和版本库。在图中标记为 \"index\"
    的区域是暂存区（stage, index），标记为 \"HEAD\"是版本库，也是 master
    分支所代表的目录树。

-   图中我们可以看出此时 \"HEAD\" 实际是指向 master
    分支的一个指针。所以图示的命令中出现 HEAD 的地方可以用 master
    来替换。

-   当对工作区修改（或新增）的文件执行 \"git add\"
    命令时，暂存区的目录树被更新，同时工作区修改（或新增）的文件内容被写入到对象库中的一个新的对象中，而该对象的ID
    被记录在暂存区的文件索引中。

-   当执行提交操作（git
    commit）时，会将暂存区的目录树写到版本库（同时更新对象）中，master
    分支会做相应的更新。即 master 指向的目录树就是提交时暂存区的目录树。

-   当执行 \"git rm \--cached \<file\>\"
    命令时，会直接从暂存区删除文件，工作区则不做出改变。

-   当执行 \"git reset HEAD\" 命令时，暂存区的目录树会被重写，被 master
    分支指向的目录树所替换，但是工作区不受影响。

-   当执行 \"git checkout \-- \<file\>\"
    命令时，会用暂存区指定的文件替换工作区的文件。这个操作很危险，会清除工作区中未添加到暂存区的改动。

-   当执行 \"git checkout HEAD \<file\>\" 命令时，会用 HEAD 指向的
    master
    分支中的指定文件替换暂存区和以及工作区中的文件。这个命令也是极具危险性的，因为不但会清除工作区中未提交的改动，也会清除暂存区中未提交的改动。

在考虑暂存区和版本库的关系的时候，可以粗略地认为暂存区是开发版，而版本库可以认为是稳定版。

git 中引入的**暂存区**的概念可以说是 git
里是最有亮点的设计之一，我们在这里不再详细介绍其快照与回滚的原理，如果有兴趣的同学可以去看看[Pro
git](https://git-scm.com/book/zh/v2)这本书。

### git文件状态

首先对于任何一个文件, 在 git 内都有四种状态: 未跟踪 (untracked)、未修改
(unmodified)、已修改 (modified)、已暂存 (staged)

未跟踪

:   ： 表示没有跟踪(add)某个文件的变化，使用git add即可跟踪文件

未修改

:   ： 表示某文件在跟踪后一直没有改动过或者改动已经被提交

已修改

:   ： 表示修改了某个文件,但还没有加入(add)到暂存区中

已暂存

:   ： 表示把已修改的文件放在下次提交(commit)时要保存的清单中

::: note
实际上是因为 git add
指令本身是有多义性的，虽然差别较小但是不同情境下使用依然是有区别。我们现在只需要记住：新建文件后要
git add，修改文件后也需要 git add。
:::

我们使用一张图来说明文件的四种状态的转换关系

![git中的四种状态转换关系](git-change){#fig:git-change width="9.5cm"}

::: thinking
[]{#think-箭头与指令 label="think-箭头与指令"}
仔细看看这张图，思考一下箭头中的 add the file 、stage the file 和commit
分别对应的是 git 里的哪些命令呢？
:::

看到这里，相信你对 git 的设计有了初步的认识。下一步我们就来深入理解一下
git 里的一些机制。

### git恢复机制

我们在编写代码时可能遇到过，错误地删除文件、一个修改让程序再也无法运行等糟糕的情况。在学习git恢复机制之前,我们先了解一下下面这些指令。

git rm \--cached \<file\>

:   这条指令是指从暂存区中删去一些我们不想跟踪的文件，比如我们自己调试用的文件等。

git checkout \-- \<file\>

:   如果我们在工作区改呀改，把一堆文件改得乱七八糟的，发现编译不过了！如果我们还没git
    add，就能使用这条命令，把它变回曾经的样子。

git reset HEAD \<file\>

:   刚才提到，如果没有git add
    把修改放入暂存区的话，可以使用checkout命令，那么如果我们不慎已经 git
    add
    加入了怎么办呢？那就需要这条指令来帮助我们了！这条指令可以让我们的**暂存区**焕然一新。

git clean \<file\> -f

:   如果你的工作区这时候混入了奇怪的东西，你没有追踪它，但是想清除它的话就可以使用这条指令，它可以帮你把奇怪的东西剔除出去。

好了，学了这么多，我们来利用自己的知识帮助小明摆脱困境吧。

::: thinking
[]{#think-小明的困境 label="think-小明的困境"}

-   深夜，小明在做操作系统实验。困意一阵阵袭来，小明睡倒在了键盘上。等到小明早上醒来的时候，他惊恐地发现，他把一个重要的代码文件printf.c删除掉了。苦恼的小明向你求助，你该怎样帮他把代码文件恢复呢？

-   正在小明苦恼的时候，小红主动请缨帮小明解决问题。小红很爽快地在键盘上敲下了git
    rm
    printf.c，这下事情更复杂了，现在你又该如何处理才能弥补小红的过错呢？

-   处理完代码文件，你正打算去找小明说他的文件已经恢复了，但突然发现小明的仓库里有一个叫**Tucao.txt**，你好奇地打开一看，发现是吐槽操作系统实验的，且该文件已经被添加到暂存区了，面对这样的情况，你该如何设置才能使Tucao.txt在不从工作区删除的情况下不会被git
    commit指令提交到版本库？
:::

关于上面那些撤销指令，等到你哪天突然不小心犯错的时候再来查阅即可，当然更推荐你使用git
status来看当前状态下git的推荐指令。我们现阶段先掌握好add
和commit的用法即可。当然，**一定要慎用撤销指令**，否则撤销之后如何撤除撤销指令将是一件难事。

介绍完上面三条撤销指令，我们来看看一些恢复指令

``` {.bash linenos=""}
 git reset --hard
```

为了体会它的作用，我们做个小练习试一下

::: exercise
-   找到在/home/16xxxxxx_2018_jac/下刚刚创建的README.txt，没有的话就新建一个。

-   在文件里加入**Testing 1**，add，commit，提交说明写 1。

-   模仿上述做法，把1分别改为 2 和 3，再提交两次。

-   使用 git
    log命令查看一下提交日志，看是否已经有三次提交了？记下提交说明为 3
    的哈希值[^1]。

-   使用 git reset \--hard HEAD`^` ，现在再使用git log，看看什么没了？

-   找到提交说明为1的哈希值，使用 git reset \--hard \<Hash-code\>
    ，再使用git log，看看什么没了？

-   现在我们已经回到以前的版本了。使用 git reset \--hard \<Hash-code\>
    ，再使用git log，看看发生了什么！
:::

这条指令就是让我们可前进，可后退。它有两种用法，第一种是使用
HEAD类似形式，如果想退回上个版本就用 HEAD`^`，上上个的话就用
HEAD`^``^`，当然要是退50次的话写不了那么多`^`，可以使用HEAD`~`50来代替。第二种就是使用我们神器Hash值，用Hash值不仅可以回到过去，还可以"回到未来"。

必须注意，`--hard` 是 reset 命令唯一的危险用法，它也是 git
会真正地销毁数据的几个操作之一。 其他任何形式的 reset
调用都可以轻松撤消，但是 `--hard`
选项不能，因为它强制覆盖了工作目录中的文件。
若该文件还未提交，git会覆盖它从而导致无法恢复。（摘自Pro git）

现在我们已经学会了git的一个主要功能，就是**版本回退**。

### git分支

我们之前提到过[分支](#分支)这个概念，那么分支是个什么东西呢？分支有点类似科幻电影里面的平行宇宙，不同的分支间不会互相影响。使用分支意味着你可以从开发主线上分离开来，然后在不影响主线的同时继续工作。在我们实验中也会多次使用到分支的概念。首先我们来介绍一条创建分支的指令[]{#git branch
label="git branch"}

``` {.bash linenos=""}
# 创建一个基于当前分支产生的分支，其名字为<branch-name>
$ git branch <branch-name>
```

这条指令往往会在我们进行每周的课上测试时会用到。其功能相当于把当前分支的内容拷贝一份到新的分支里去，然后在新的分支上做测试功能的添加，不会影响实验分支的内容。假如我们当前在master[^2]分支下已经有过三次提交记录，这时我们可以使用
branch 命令新建了一个分支testing（参考图
[1.8](#git-branch-create){reference-type="ref"
reference="git-branch-create"}）。

![分支建立后](git-branch-create){#git-branch-create width="8cm"}

删除一个分支也很简单，只要加上-d选项(-D是强制删除)即可，就像下面这样。

``` {.bash linenos=""}
# 创建一个基于当前分支产生的分支，其名字为<branch-name>
$ git branch -d(D) <branch-name>
```

想查看分支情况以及当前所在分支，只需要加上 -a选项即可

``` {.bash linenos=""}
# 查看所有的远程与本地分支
$ git branch -a

# 使用该命令的效果如下
# 前面带*的分支是当前分支
  lab1
  lab1-exam
* lab1-result
  master
  remotes/origin/HEAD -> origin/master
  remotes/origin/lab1
  remotes/origin/lab1-exam
  remotes/origin/lab1-result
  remotes/origin/master
# 带remotes是远程分支，在后面提到远程仓库的时候我们会知道
```

我们建立了分支并不代表会自动切换到分支，那么，git
是如何知道你当前在哪个分支上工作的呢？其实git保存着一个名为 HEAD
的特别指针（前面介绍过）。在 git
中，HEAD是一个指向正在工作中本地分支的指针，可以将 HEAD
想象为当前分支的别名。运行git branch
命令，仅仅是建立了一个新的分支，但不会自动切换到这个分支中去，所以在这个例子中，我们依然还在
master 分支里工作。

那么如何切换到另一个分支去呢，这时候就要用到在实验中更常见的指令[]{#git checkout
label="git checkout"}

``` {.bash linenos=""}
# 切换到<branch-name>代表的分支，这时候HEAD游标指向新的分支
$ git checkout <branch-name>
```

比如这时候我们使用 **git checkout testing**，这样 HEAD 就指向了 testing
分支（见图[1.9](#git-branch-checkout){reference-type="ref"
reference="git-branch-checkout"}）。

![分支切换后](git-branch-checkout){#git-branch-checkout width="8cm"}

这时候你会发现你的工作区就是testing分支下的工作目录，而且在testing分支下的修改，添加与提交不会对master分支产生任何影响。

在我们的操作系统实验中，有以下几种分支：

labx

:   这是提交实验代码的分支，这个分支不需要我们手动创建。当写好代码提交到服务器上后，在该次实验结束后，使用后面提到的[更新指令](#更新指令)可获取到新的实验分支，到时只需要使用git
    checkout labx即可进行新的实验。

labx-exam、labx-extra

:   等是每周课上测试实验的分支，每次需要使用 git branch
    指令将刚完成的实验分支拷贝一份到
    labx-exam分支下，并进行测试代码的编写。

::: note
每次实验虽然是60算实验通过，但是成绩最好是100。因为每次新实验的代码是你刚完成的实验代码以及一些新的要填充的文件组成的，前面实验的错误可能会给后面的实验中造成很大的困难。当然由于测试点不会覆盖所有代码，所以成绩为100也不代表实验一定全部正确，尽可能多花点时间理解与修改。
:::

我们之前所介绍的这些指令只是在本地进行操作的，其中必须掌握

1.  [git add](#git add)

2.  [git commit](#git commit)

3.  [git branch](#git branch)

4.  [git checkout](#git checkout)

其余指令可以临时查阅，多学习git的好处可能还现在体现不出来，但当你以后与开发团队一起做项目的时候，就会体会到掌握git的知识是件多么幸福的事情。之前我们所有的操作都是在本地仓库上操作，下面要介绍的是一组和远程仓库有关的指令，这组指令比较容易出错。

### git远程仓库与本地

在我们的实验中，设立了几台服务器主机作为大家的远程仓库。远程仓库其实和你本地版本库结构是一致的，只不过远程仓库是在服务器上的仓库，而本地版本库是在本地。实验中我们每次对代码有所修改时，最后都需要在实验截止时间之前提交到服务器上，我们以服务器上的远程仓库里的代码为评测标准哦。我们先介绍一条我们实验中比较常用的一条命令

``` {.bash linenos=""}
# git clone 用于从远程仓库克隆一份到本地版本库
$ git clone git@ip:学号-lab
```

从名字也能很容易理解这条指令的含义，就是使用clone指令而把服务器上的远程仓库拷贝到本地版本库里。但是初学者在使用这条命令的时候可能会遇到一个问题，那么来仔细思考一下下面的问题

::: thinking
[]{#think-克隆 label="think-克隆"}
思考下面四个描述，你觉得哪些正确，哪些错误，请给出你参考的资料或实验证据。

1.  克隆时所有分支均被克隆，但只有HEAD指向的分支被检出（checkout）。

2.  克隆出的工作区中执行 git log、git status、git checkout、git
    commit等操作不会去访问远程仓库。

3.  克隆时只有远程仓库HEAD指向的分支被克隆。

4.  克隆后工作区的默认分支处于master分支。
:::

::: note
检出某分支指的是在该分支有对应的本地分支，使用git checkout
后会在本地检出一个同名分支自动跟踪远程分支。比如现在本地没有文件，远程有一个名为
os的分支，我们使用 git checkout os
即可在本地建立一个跟远程分支同名，自动追踪远程分支的os分支，并且在os分支下push时会默认提交到远程分支
os上。
:::

初学者最容易犯的一个错误是，克隆代码后马上进行编译。但是克隆代码时默认处于master分支，而我们实验的代码测试不是在master分支上进行，所以首先要使用[git
checkout](#git checkout)检出对应的labx分支，再进行测试。课上测试时也要先看清楚分支再进行代码编写和提交。

下面再介绍两条跟远程仓库有关的指令，其作用很简单，但要用好却是比较难。

``` {.bash linenos=""}
# git push 用于从本地版本库推送到服务器远程仓库
$ git push

# git pull 用于从服务器远程仓库抓取到本地版本库
$ git pull
```

git
push只是将本地版本库里已经commit的部分同步到服务器上去，不包括**暂存区**里存放的内容。在我们实验中还可能会加些选项。

``` {.bash linenos=""}
# origin在我们实验里是固定的，以后就明白了。branch是指本地分支的名称。
$ git push origin [branch]
```

这条指令可以将我们本地创建的分支推送到远程仓库中，在远程仓库建立一个同名的本地追踪的远程分支。比如我们实验课上测试时要在本地先建立一个**labx-exam**的分支，在提交完成后，我们要使用**git
push origin
labx-exam**在服务器上建立一个同名远程分支，这样服务器才可以通过测试该分支的代码来检测你的代码是否正确。

git pull[]{#更新指令 label="更新指令"}
是有条更新的指令。如果在服务器端发布了新的分支，下发了新的代码或者进行了一些改动的话，就需要使用
git pull来让本地版本库与远程仓库保持同步。

### git冲突与解决冲突

push和pull两条指令的含义虽然很清楚，但是还是很容易出现的下面问题。

``` {.bash linenos=""}
中文版：
To git@github.com:16xxxxxx.git
 ! [rejected]        master -> master (non-fast-forward)
error: 无法推送一些引用到 'git@github.com:16xxxxxx.git'
提示：更新被拒绝，因为您当前分支的最新提交落后于其对应的远程分支。
提示：再次推送前，先与远程变更合并（如 'git pull ...'）。详见
提示：'git push --help' 中的 'Note about fast-forwards' 小节。

英文版：
To git@github.com:16xxxxxx.git
 ! [rejected]        master -> master (non-fast-forward)
error: failed to push some refs to 'To git@github.com:16xxxxxx.git'
hint: Updates were rejected because the tip of your current branch is behind
hint: its remote counterpart. Integrate the remote changes (e.g.
hint: 'git pull ...') before pushing again.
hint: See the 'Note about fast-forwards' in 'git push --help' for details.
```

这个问题是因为什么而产生的呢？我们来分析一下，你有可能在公司和在家操作同一个分支，在公司你对一个文件进行了修改，然后进行了提交。回了家又对同样的文件做了不同的修改，在家中使用push同步到了远程分支。但等你回到公司再push的时候就会发现一个问题：现在远程仓库和本地版本库已经分离开变成两条岔路了（见图[1.10](#git-remote-branches){reference-type="ref"
reference="git-remote-branches"})。

![远程仓库与本地仓库的岔路](git-remote-branches){#git-remote-branches
width="8cm"}

这样远程仓库就不知道如何选择了。你不想浪费劳动成果，希望在公司的提交有效，在家里的提交也有效，想让远程仓库把你的提交全部接受，那么怎么才能解决这个问题呢？这时候就要用**git
pull**指令。

当然在push之前，使用git
pull不会很轻松地解决所有问题，我们不能指望git把文件中的修改全部妥善合并，git只是为我们提供了另一种机制能快速定位有冲突（conflict）的文件。这时候使用git
pull，你可能会看到有下面这样的提示

``` {.bash linenos=""}
Auto-merging test.txt
CONFLICT (content): Merge conflict in test.txt
Automatic merge failed; fix conflicts and then commit the result.
```

有冲突的文件中往往包含一部分类似如下的奇怪代码，我们打开test.txt，发现这样一些"乱码"

``` {.bash linenos=""}

a123
<<<<<<< HEAD
b789
=======
b45678910
>>>>>>> 6853e5ff961e684d3a6c02d4d06183b5ff330dcc
c
```

冲突标记`<<<<<<<`
与=======之间的内容是你在家里的修改，=======与`>>>>>>>`之间的内容是你在公司的修改。

要解决冲突还需要编辑冲突文件，将其中冲突的内容手工合理合并。另外，需要在文件中解决了冲突之后重新add该文件并commit。

不过你也可能在git pull的时候也会遇到下面问题：

``` {.bash linenos=""}
error: Your local changes to the following files would be overwritten by merge:
    16xxxxxx-lab/readme.txt
Please, commit your changes or stash them before you can merge.
Aborting
```

其实提示已经比较清楚了，这里还需要把我们之前的所有修改全部提交(commit)，提交之后再git
pull就好。当然，还有更高级的办法，如果你已经熟悉了git的基础操作，那么可以阅读[git
stash解决git pull冲突](http://www.01happy.com/git-resolve-conflicts/)

不要觉得这一节的冲突一节不需要学习，因为你可能会想：我现在怎么可能在公司和家里同时修改文件呢！但是要注意，在远程仓库编辑的不止你一个人，还有助教老师，助教老师一旦修改一些东西都有可能产生冲突。实践是最好的老师，我们再来实践一下

::: exercise
仔细回顾一下上面这些指令，然后完成下面的任务

-   在 /home/16xxxxxx_2018_jac/16xxxxxx-lab下新建分支，名字为Test

-   切换到Test分支，添加一份readme.txt，内容写入自己的学号

-   将文件提交到本地版本库，然后建立相应的远程分支。
:::

到这里git教程基本结束了，下面实验代码提交流程的简明教程，希望大家可以快速上手！

### 实验代码提交流程

modify

:   写代码。

git add ＆ git commit \<modified-file\>

:   提交到本地版本库。

git pull

:   从服务器拉回本地版本库，并解决服务器版本库与本地代码的冲突。

git add ＆ git commit \<conflict-file\>

:   将远程库与本地代码合并结果提交到本地版本库。

git push

:   将本地版本库推到服务器。

mkdir test ＆ cd test ＆ git clone

:   建立一个额外的文件夹来测试服务器上的代码是否正确。

而我们在一次实验结束，新的实验代码下发时，一般是按照以下流程的来开启新的实验之旅。

git add ＆ git commit

:   如果当前分支的暂存区还有东西的话，先提交。

git pull

:   这一步很重要！要先确保服务器上的更新全部同步到本地版本库！

git checkout labx

:   检出新实验分支并进行实验。

谨记，一定要勤使用**git pull**，这条指令很重要！随时同步一下！

如果大家对深入了解git有兴趣，可以参考[^3]。如果你希望能学到更厉害的技术，推荐[gitHug](https://github.com/Gazler/githug)，这是一个关于git的通关小游戏。

## 实战测试

随着lab0学习的结束，下面就要开始通过实战检验水平了，请同学们按照下面的题目要求完成所需操作，最后将工作区push至远端以进行评测，评测完成后会返回lab0课下测试的成绩，通过课下测试(\>=60分)即可参加上机时的课上测试。

::: exercise
1、在lab0工作区的src目录中，存在一个名为palindrome.c的文件，使用刚刚学过的工具打开palindrome.c，使用c语言实现判断输入整数n($1 \le n \le 10000$)是否为回文数的程序(输入输出部分已经完成)。通过stdin每次只输入一个整数n，若这个数字为回文数则输出Y，否则输出N。`[`注意：正读倒读相同的整数叫回文数`]`

2、在src目录下，存在一个未补全的Makefile文件，借助刚刚掌握的Makefile知识，将其补全，以实现通过make指令触发src目录下的palindrome.c文件的编译链接的功能，生成的可执行文件命名为palindrome。

3、在src/sh_test目录下，有一个file文件和hello_os.sh文件。hello_os.sh是一个未完成的脚本文档，请同学们借助shell编程的知识，将其补完，以实现通过指令bash
hello_os.sh AAA
BBB.c，在hello_os.sh所处的文件夹新建一个名为BBB.c的文件，其内容为AAA文件的第8、32、128、512、1024行的内容提取(AAA文件行数一定超过1024行)。`[`注意：对于指令bash
hello_os.sh AAA BBB.c，AAA及BBB可为任何合法文件的名称，例如bash
hello_os.sh file hello_os.c，若以有hello_os.c文件，则将其原有内容覆盖`]`

4、补全后的palindrome.c、Makefile、hello_os.sh依次复制到路径dst/palindrome.c,
dst/Makefile, dst/sh_test/hello_os.sh
`[`注意：文件名和路径必须与题目要求相同`]`

要求按照测试1`~`测试4要求完成后，最终提交的文件树图示如图[1.11](#fig:0-ex-1){reference-type="ref"
reference="fig:0-ex-1"}

5、在lab0工作区ray/sh_test1目录中，含有100个子文件夹file1`~`file100，还存在一个名为changefile.sh的文件，将其补完，以实现通过指令bash
changefile.sh，可以删除该文件夹内file71`~`file100共计30个子文件夹，将file41`~`file70共计30个子文件夹重命名为newfile41`~`newfile70。`[`注意：评测时仅检测changefile.sh的正确性`]`

要求按照测试5要求完成后，最终提交的文件树图示如图[1.12](#fig:0-ex-2){reference-type="ref"
reference="fig:0-ex-2"}(file下标只显示1`~`12，newfile下标只显示41`~`55)

6、在lab0工作区的ray/sh_test2目录下，存在一个未补全的search.sh文件，将其补完，以实现通过指令bash
search.sh file int
result，可以在当前文件夹下生成result文件，内容为file文件含有int字符串所在的行数，即若有多行含有int字符串需要全部输出。`[`注意：对于指令bash
search.sh file int
result，file及result可为任何合法文件名称，int可为任何合法字符串，若已有result文件，则将其原有内容覆盖，匹配时大小写不忽略`]`

要求按照测试6要求完成后，result内显示样式如图[1.13](#fig:0-ex-3){reference-type="ref"
reference="fig:0-ex-3"}(一个答案占一行)：

7、在lab0工作区的csc/code目录下，存在fibo.c、main.c，其中fibo.c有点小问题，还有一个未补全的modify.sh文件，将其补完，以实现通过指令bash
modify.sh fibo.c char
int，可以将fibo.c中所有的char字符串更改为int字符串。`[`注意：对于指令bash
modify.sh fibo.c char
int，fibo.c可为任何合法文件名，char及int可以是任何字符串，评测时评测modify.sh的正确性，而不是检查修改后fibo.c的正确性`]`

8、lab0工作区的csc/code/fibo.c成功更换字段后(bash modify.sh fibo.c char
int)，现已有csc/Makefile和csc/code/Makefile，补全两个Makefile文件，要求在csc目录下通过指令make可在csc/code文件夹中生成fibo.o、main.o，在csc文件夹中生成可执行文件fibo，再输入指令make
clean后只删除两个.o文件。`[`注意：不能修改fibo.h和main.c文件中的内容，提交的文件中fibo.c必须是修改后正确的fibo.c，可执行文件fibo作用是输入一个整数n(从stdin输入n)，可以输出斐波那契数列前n项，每一项之间用空格分开。比如n=5，输出1
1 2 3 5`]`

要求成功使用脚本文件modify.sh修改fibo.c，实现使用make指令可以生成.o文件和可执行文件，再使用指令
make
clean可以将.o文件删除，但保留fibo和.c文件。最终提交时文件中fibo和.o文件可有可无。
:::

![测试1`~`测试4最终提交的文件树](0-ex-image1){#fig:0-ex-1 width="8cm"}

![测试5最终提交的文件树](0-ex-image2){#fig:0-ex-2 width="8cm"}

![测试6完成后结果](0-ex-image3){#fig:0-ex-3 width="8cm"}

![make后文件树](0-ex-image7-make){#fig:0-ex-4 width="8cm"}

![make clean后文件树](0-ex-image7-clean){#fig:0-ex-5 width="8cm"}

## 实验思考

-   [**[思考-Shell简析]{style="color: baseB"}**](#think-Shell简析)

-   [**[思考-文件的操作]{style="color: baseB"}**](#think-文件的操作)

-   [**[思考-箭头与指令]{style="color: baseB"}**](#think-箭头与指令)

-   [**[思考-小明的困境]{style="color: baseB"}**](#think-小明的困境)

-   [**[思考-克隆命令]{style="color: baseB"}**](#think-克隆)

请认真做练习，然后把实验思考里的内容附在实验文档中一起提交！

[^1]: 使用git log命令时，在commit 标识符后的一长串数字和字母组成的字符串

[^2]: master分支是我们的主分支，一个仓库初始化时自动建立的默认分支

[^3]: 推荐廖雪峰老师的网站: <http://www.liaoxuefeng.com/>
