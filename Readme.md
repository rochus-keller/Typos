## Welcome to the Typos project

This repository will slowly evolve into a new statically typed document language and typesetting system.

**Motivation for this project:**
LuaTeX produces excellent typography, but with a terrible document description language. Typst seems to be a much better language, but the quality 
of the typography is still far from LuaTeX. Why has no one created a decent document description language for LuaTeX yet? It's about time someone gives it a try.

**Intermediate Result:** 
As it seems, LuaTeX is far from suitable for the intended purpose (see my comment below). I need another engine. Fortunately, there are various libraries that 
are much easier to understand and use. Even if I only achieve 80% of the typographical quality of e.g. ConTeXt, the prospect is only 20% of the effort. 

**NOTE** that this project is in an early stage and work-in-progress.

### Planned features

- [x] Build a stand-alone version of LuaTeX and verify that it works
- [x] Replace build system by BUSY
- [x] Removed all autotools files and readme files thereof
- [x] Implement a simple OTF font loader glue which responds the callback and orchestrates the necessary libs
- [ ] Remove all unused parts from the repository (WIP)
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

Now we have again a working BUSY build which is complete, fast and works well with the source level debugger. The build.sh and all autotool related 
files have been removed so that I can make configuration changes without the need to update two build systems.

### Status on December 31, 2025

The font loader (see font_loader.c) works in principle; OTF/TTF fonts can be loaded completely without an external Lua implementation (such as the huge
luaotfload) including the math parameters. 

But the big disappointment followed quickly. One might naively think that LuaTeX is a complete, functional engine where you simply enter some TeX code 
and get a beautifully typeset PDF in return. This is apparently not the case.

I could more or less accept that I had to contribute the code that finds the fonts and loads their metrics myself (I even had to extend the HarfBuzz Lua API for this purpose). But apparently I am one of the few people who have ever tried to use TeX without a huge, ready-made format. Without a format file, this engine apparently can't do anything at all. But what's even worse is that a significant part of the typographical process and the extraordinary typographical quality of LuaTeX seems to be contained in these format files and external, huge Lua scripts (such as luaotfload), and not at all in the LuaTeX engine itself. Although I had used TeX (or LaTeX) before, and even built applications that used ConTeXt as a “reporting engine,” I was not aware of this.

Long story short: my test cases worked in principle, but the typographical quality is terrible. And I don't see an easy way to change that without significant reverse engineering/migration of the existing, huge Lua and TeX implementations into the engine. If I don't do that, there's no point in having a stand-alone engine (without the huge superstructure of TexLive or ConTeXt). 

I would say that this concludes my “excursion” into the inner workings of LuaTeX.

### How to build

Use the [BUSY build system](https://github.com/rochus-keller/busy/) or open the BUSY file with [LeanCreator](https://github.com/rochus-keller/leancreator/).
So far there is only a BUSY file in the luatex subdirectory.

The build was successfully tested on Linux x64. It is likely to also work on Linux x86 or macOS. 
Win32 has not been tested yet and likely doesn't build without changes.

### Additional Credits

- This work is based on the [LuaTeX 1.18 (2024)](https://gitlab.lisn.upsaclay.fr/texlive/luatex/-/tree/1.18.0?ref_type=tags) engine. 
- [Lua 5.3](http://www.lua.org) is copyright 1994-2015 by Luiz Henrique de Figueiredo, Roberto Ierusalimschy and Waldemar Celes.

