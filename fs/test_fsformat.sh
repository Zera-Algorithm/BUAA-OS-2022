#!/bin/bash
make --directory=".."
./fsformat ../gxemul/fs.img $@
xxd ../gxemul/fs.img fsimg.txt
../tools/fsformat ../gxemul/fs.img $@
xxd ../gxemul/fs.img fsimg_std.txt
diff fsimg.txt fsimg_std.txt
