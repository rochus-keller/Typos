## Welcome to the Typos project

This repository will slowly evolve into a new statically typed document language and typesetting system.

**Motivation for this project:**
LuaTeX produces excellent typography, but with a terrible document description language. Typst seems to be a much better language, but the quality of the typography is still far from LuaTeX. Why has no one created a decent document description language for LuaTeX yet? It's about time someone gave it a try.

**NOTE** that this project is in an early stage and work-in-progress.

### Planned features

- [x] Build a stand-alone version of LuaTeX and verify that it works
- [x] Replace build system by BUSY
- [ ] Remove all unused parts from the repository including the autotools build (WIP)
- [ ] Implement a simple OTF font loader glue which responds the callback and orchestrates the necessary libs
- [ ] Specify a statically (gradually) typed typesetting language in the Oberon+/Luon or Typst lineage
- [ ] Implement a new frontend 

### Status on December 26, 2025

This is a restart of my work on this project from December 20 to 25 based on LuaTeX 1.10.0. I will do the same again, but this time
with LuaTeX 1.18.0, which includes HarfBuzz and Graphite2. I found out that otherwise LuaTex depends on a 41kLOC Lua file to achieve
high-quality typography with standard fonts (not just the 1986 original TeX fonts). See the readme of the "first-attempt" branch for more information. 

I therefore [downloaded the LuaTeX 1.18.0 version](https://gitlab.lisn.upsaclay.fr/texlive/luatex/-/archive/1.18.0/luatex-1.18.0.tar.gz?ref_type=tags). 
According to my research, 1.18.0 includes all features I need, is very stable, and can be built with C99 and C++11. More recent versions seem to require
later C and C++ versions. I was able to successfully build 1.18.0 on Debian Bookworm x64 with the command 
`export MAKEINFO=true; BUILDLUAHB=TRUE; CFLAGS="-O2 -fcommon" ./build.sh`. I verified that HarfBuzz etc. is actually included, and the result passes my basic tests. 

### Status on December 27, 2025

Now we have again a working BUSY build which is complete, fast and works well with the source level debugger. The build.sh has been removed (since broken). I will 
soon also remove the autotools build so that I can make configuration changes without the need to update two build systems.

### How to build

Use the [BUSY build system](https://github.com/rochus-keller/busy/) or open the BUSY file with [LeanCreator](https://github.com/rochus-keller/leancreator/).
So far there is only a BUSY file in the luatex subdirectory.

The build was successfully tested on Linux x64. It is likely to also work on Linux x86 or macOS. 
Win32 has not been tested yet and likely doesn't build without changes.

### Additional Credits

- This work is based on the [LuaTeX 1.18 (2024)](https://gitlab.lisn.upsaclay.fr/texlive/luatex/-/tree/1.18.0?ref_type=tags) engine. 
- [Lua 5.3](http://www.lua.org) is copyright 1994-2015 by Luiz Henrique de Figueiredo, Roberto Ierusalimschy and Waldemar Celes.

