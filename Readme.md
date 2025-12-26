## Welcome to the Typos project

This repository will slowly evolve into a new statically typed document language and typesetting system.

NOTE that this project is in an early stage and work-in-progress.

### Planned features

- [x] Build a stand-alone version of LuaTeX and verify that it works
- [ ] Remove all unused parts from the repository
- [ ] Remove the TeX frontend and the mktexfmt machinery 
- [ ] Replace kpathsea and ptexenc by LeanQt 
- [ ] Specify a statically (gradually) typed typesetting language in the Oberon+/Luon lineage
- [ ] Implement a new frontend and page layouter

### Status on December 26, 2025

This is a restart of my work on this project from December 20 to 25 based on LuaTeX 1.10.0. I will do the same again, but this time
with LuaTeX 1.18.0, which includes HarfBuzz and Graphite2. I found out that otherwise LuaTex depends on a 41kLOC Lua file to achieve
high-quality typography with standard fonts (not just the 1986 original TeX fonts). See the readme of the "first-attempt" branch for more information. 

I therefore [downloaded the LuaTeX 1.18.0 version](https://gitlab.lisn.upsaclay.fr/texlive/luatex/-/archive/1.18.0/luatex-1.18.0.tar.gz?ref_type=tags). 
According to my research, 1.18.0 includes all features I need, is very stable, and can be built with C99 and C++11. More recent versions seem to require
later C and C++ versions. I was able to successfully build 1.18.0 on Debian Bookworm x64 with the command 
`export MAKEINFO=true; BUILDLUAHB=TRUE; CFLAGS="-O2 -fcommon" ./build.sh`. I verified that HarfBuzz etc. is actually included, and the result passes my basic tests. 

### Additional Credits

- This work is based on the [LuaTeX 1.18 (2024)](https://gitlab.lisn.upsaclay.fr/texlive/luatex/-/tree/1.18.0?ref_type=tags) engine. 
- [Lua 5.3](http://www.lua.org) is copyright 1994-2015 by Luiz Henrique de Figueiredo, Roberto Ierusalimschy and Waldemar Celes.

