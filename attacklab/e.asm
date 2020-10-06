
e.o:     file format elf64-x86-64


Disassembly of section .text:

0000000000000000 <.text>:
   0:	48 c7 04 24 fa 18 40 	movq   $0x4018fa,(%rsp)
   7:	00 
   8:	49 bf 35 39 62 39 39 	movabs $0x6166373939623935,%r15
   f:	37 66 61 
  12:	4c 89 7c 24 c0       	mov    %r15,-0x40(%rsp)
  17:	48 8d 7c 24 c0       	lea    -0x40(%rsp),%rdi
  1c:	c3                   	retq   
