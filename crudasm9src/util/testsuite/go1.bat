@echo off
if exist test1.exe del test1.exe
g++ -o test1 test1.cpp ../../x86core/decoder_internal.c ../../x86core/ixdecoder.c ../../x86core/ixdisasm.c
test1
ndisasm -b 16 test16.bin -o 0x100 >test16asm.txt
ndisasm -b 32 test32.bin -o 0x100 >test32asm.txt
ndisasm -b 64 test64.bin -o 0x100 >test64asm.txt
..\crudbin\crudbin test16.bin 16 0x100 >test16new.txt
..\crudbin\crudbin test32.bin 32 0x100 >test32new.txt
..\crudbin\crudbin test64.bin 64 0x100 >test64new.txt
