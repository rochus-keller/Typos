## Welcome to the Typos project

This repository will slowly evolve into a new statically typed document language and typesetting system.

**Motivation for this project:**
LuaTeX produces excellent typography, but with a terrible document description language. Typst seems to be a much better language, but the quality of the typography is still far from LuaTeX. Why has no one created a decent document description language for LuaTeX yet? It's about time someone gave it a try.

**NOTE** that this project is in an early stage and work-in-progress.

### Planned features

- [x] Build a stand-alone version of LuaTeX and verify that it works
- [x] Replace build system by BUSY
- [ ] Remove all unused parts from the repository including the autotools build (WIP)
- [ ] Remove the TeX frontend and the mktexfmt machinery 
- [ ] Replace kpathsea and ptexenc by LeanQt 
- [ ] Specify a statically (gradually) typed typesetting language in the Oberon+/Luon lineage
- [ ] Implement a new frontend and page layouter

### Status on December 20, 2025

Found and [downloaded a LuaTeX version](https://gitlab.lisn.upsaclay.fr/texlive/luatex/-/archive/1.10.0/luatex-1.10.0.tar.gz?ref_type=tags)
which works independently of the huge TeX Live system. To build it on Debian Bookworm x64 without errors, the command 
`export MAKEINFO=true; CFLAGS="-O2 -fcommon" ./build.sh` must be used (to avoid texinfo dependency and an issue added to GCC 10.

### Status on December 21, 2025

Now we have a working BUSY build which is complete, fast and works well with the source level debugger. The test case renders the same results
as if built with the original build.sh script. I will soon remove the autotools build so that I can make configuration changes without the need to 
update two build systems.

### Status on December 25, 2025

While making experiments with the stripped-down LuaTeX version and trying to run the "TeX by Topic" book or the 
[TeX version of the Project Oberon book](https://github.com/guidoism/tex-oberon), it turned out that (contrary to my previous assumption) LuaTeX
is not complete and self-sufficient at all, but indeed requires a 41kLOC Lua source file to be able to use "normal" fonts and enable 
hight-quality typography using them. The luafontloader library which includes fontforge covers only a subset of the required functionality.
So I had to download and add [luatex-fonts-merged.lua](https://github.com/contextgarden/context/blob/main/tex/generic/context/luatex/luatex-fonts-merged.lua) 
to the project, which is about 1 megabyte of Lua source. Even after more than a day of experimenting, I wasn't able to correctly typeset "Hello World". After
re-adding luafilesystem and luapeg, I at least managed to run the whole machinery and generate PDFs, but apparently I didn't manage to meet the assumptions 
luatex-fonts-merged.lua makes about its environment (usually ConTeXt), and due to the dynamic typing nature of the Lua code it was neither possible to
trace and derive the dependencies. From that I concluded that this approach is too brittle and unlikely to ever work without heavy (reverse)engineering. 
Instead, I will now turn my focus to the newer, HarfBuzz-based version (LuaHBTeX); it appears to (finally) include all the necessary functions to correctly 
layout standard fonts without having to resort to complex Lua machinery, which, in my opinion, significantly overstretches the feasibility and robustness of Lua.

### How to build

Use the [BUSY build system](https://github.com/rochus-keller/busy/) or open the BUSY file with [LeanCreator](https://github.com/rochus-keller/leancreator/).
So far there is only a BUSY file in the luatex subdirectory.

The build was successfully tested on Linux x64. It is likely to also work on Linux x86 or macOS. 
Win32 has not been tested yet and likely doesn't build without changes.

### Additional Credits

- This work is based on the [LuaTeX 1.10 (2019)](https://gitlab.lisn.upsaclay.fr/texlive/luatex/-/tree/1.10.0?ref_type=tags) engine. 
- [Lua 5.3](http://www.lua.org) is copyright 1994-2015 by Luiz Henrique de Figueiredo, Roberto Ierusalimschy and Waldemar Celes.

