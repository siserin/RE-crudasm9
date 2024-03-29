# crudasm9

README.TXT - Copyright (C) 2013,2014 Willow Schlanger. All rights reserved.
Date: February 23, 2014

Author's Home Page: http://www.willowschlanger.info
Author's E-mail:    willow@willowschlanger.info
Alternate e-mail:   wschlanger@gmail.com

This is build 1.00.00002(beta) of CRUDASM9, the open-source disassembler for x86 and x64 binary files!

1. License

Copyright (c) 2011-2014, Willow Schlanger
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

    Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
    Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

2. What is this?

This is CRUDASM Version 9. CRUDASM is an open-source disassembler that turns executable x86 or x64 binaries into human-readable assembly language.

It also ships with my free coffload32.exe/coffload64.exe set of programs to allow for the disassembly of Windows Portable Executables (i.e. Win32/Win64 EXE or DLL files).

CRUDASM was designed to be an open-source replacement for Clive Turvey's DUMPPE disassembler, which in turn (according to DUMPPE.DOC) was designed to replace Andrew Schulman's W32DUMP program.
The name "CRUDASM" means "crude disassembler" and was selected because we aim to do a relatively "rough" disassembly on our first pass.

3. Files

crudasm9bin/		This folder contains the CRUDASM9 binaries. It also contains my free COFFLOAD32/COFFLOAD64 loader program.
crudasm9src/		Source code for CRUDASM9
 makecpu/		This folder contains the code that processes the master instruction set source file, x86iset.ax.
 generated/		out_*.h files, generated by the 'makecpu' program from x86iset.ax.
 x86core/		Source code for the "core" machine code disassembler and decoder.
 util/			Source code for CRUDASM utilities.
  crudbin/		This sample program does a very simple disassembly of only raw binary files. Illustrates how to use CRUDASM9.
  testsuite/		Test suite. When x86iset.ax is changed, "makecpu" should be re-run and the testsuite can make sure everything still works.
  main/			Contains crudasm9.cpp, the main crudasm disassembler program source code.

4. Conclusion

This was a quick overview of the latest release of my open-source disassembler, CRUDASM Version 9.

Because this is still a work-in-progress, your feedback would be greatly appreciated. I still need
to add debug symbol support (i.e. support for PDB files), and be a little more intelligent and
friendly in the disassembly (i.e. jmp/call [<import>] etc. should be recognized). I also want to
consider adding support for switch statements (usually some recognizable idiom followed by an
indirect jmp) and look into doing a 2-pass disassembly, so procedures can be recognized and related
code can appear together rather than in the natural order the machine code appeared in memory.

As I finish up CRUDASM, I am beginning to move on to other things (the unreleased successor to
CRUDASM is called Infrared CANDLES and part of it is already done!) The successor to CRUDASM already
has support for more instructions. You can view an opcode map I auto-generated as part of the
Infrared CANDLES project at the following location:

	http://www.willowschlanger.info/home/content/intelopcodes/opcodemap.php

Thanks for your interest, and feel free to contact me via the above e-mail address. I'd love to hear from you!
