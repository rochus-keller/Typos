## Welcome to the Typos project

This repository will slowly evolve into a new statically typed document language and typesetting system.

NOTE that this project is in an early stage and work-in-progress.

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

### How to build

- Use the following command on Linux: `export MAKEINFO=true; CFLAGS="-O2 -fcommon" ./build.sh`
- Or use the [BUSY build system](https://github.com/rochus-keller/busy/) or open the BUSY file with [LeanCreator](https://github.com/rochus-keller/leancreator/)

### Additional Credits

- This work is based on the [LuaTeX 1.10 (2019)](https://gitlab.lisn.upsaclay.fr/texlive/luatex/-/tree/1.10.0?ref_type=tags) engine. 
- [Lua 5.3](http://www.lua.org) is copyright 1994-2015 by Luiz Henrique de Figueiredo, Roberto Ierusalimschy and Waldemar Celes.

