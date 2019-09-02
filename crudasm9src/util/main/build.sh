# This script shows how to build the Win32/Win64 versions of the CRUDASM9 main program.
g++ -O1 -m64 -o crudasm9 crudasm9.cpp ../../x86core/decoder_internal.c ../../x86core/ixdecoder.c ../../x86core/ixdisasm.c -lpsapi
g++ -O1 -m32 -o crudasm9_w32 crudasm9.cpp ../../x86core/decoder_internal.c ../../x86core/ixdecoder.c ../../x86core/ixdisasm.c -lpsapi
strip -s crudasm9.exe
strip -s crudasm9_w32.exe
