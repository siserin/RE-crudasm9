@echo off
nasm sample.asm
g++ -m32 -o crudbinw32.exe crudbin.c ../../x86core/decoder_internal.c ../../x86core/ixdecoder.c ../../x86core/ixdisasm.c
g++ -m64 -o crudbinw64.exe crudbin.c ../../x86core/decoder_internal.c ../../x86core/ixdecoder.c ../../x86core/ixdisasm.c
crudbinw32 sample 16 0
