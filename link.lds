SECTIONS
{

/*
  Exercise 1.3. Please complete this part.
  To do:
  fill in the correct address of the key sections
  such as text, data, bss.
  tips: maybe you can find the answer in include/ .
        look for the memory map.
*/
. = 0x80010000;
.text : { *(.text) }
. = 0x80010006;
.data : { *(.data) }
. = 0x80012000;
.bss : { *(.bss) }
end = . ;
}
