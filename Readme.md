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

### Status on December 20, 2025

Found and [downloaded a LuaTeX version](https://gitlab.lisn.upsaclay.fr/texlive/luatex/-/archive/1.10.0/luatex-1.10.0.tar.gz?ref_type=tags)
which works independently of the huge TeX Live system. To build it on Debian Bookworm x64 without errors, the command 
`export MAKEINFO=true; CFLAGS="-O2 -fcommon" ./build.sh` must be used (to avoid texinfo dependency and an issue added to GCC 10.

### Additional Credits

- This work is based on the [LuaTeX 1.10 (2019)](https://gitlab.lisn.upsaclay.fr/texlive/luatex/-/tree/1.10.0?ref_type=tags) engine. 
- [Lua 5.3](http://www.lua.org) is copyright 1994-2015 by Luiz Henrique de Figueiredo, Roberto Ierusalimschy and Waldemar Celes.

