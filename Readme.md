## Welcome to the Typos project

This repository will slowly evolve into a new statically typed document language and typesetting system.

The project initially started with LuaTeX as a backend, but then switched to a full custom design based on HarfBuzz, Cairo and FreeType.
I hope to achieve at least 80% of the typographical quality of e.g. ConTeXt, with a fraction of the effort to migrate the
required parts from TexLive. As a concession to feasibility, the system will only support Western languages (LTR only).

**NOTE** that this project is in an early stage and work-in-progress.

### Planned features

- [x] Prepare a new repository with harfbuzz, libpng and zlib from LuaTeX 1.18 and add Cairo, FreeType and Pixman
- [x] Replace build systems of all libraries by BUSY
- [x] Removed all autotools files and readme files thereof
- [x] Implement a test application to show that the Cairo PDF backend works
- [ ] Implement all required Knuth algorithms (paragraph, table and page layout)
- [ ] Specify a statically (gradually) typed typesetting language in the Oberon+/Luon or Typst lineage
- [ ] Implement a new frontend 

### Initial Attempts from December 20 to 31, 2025

My original intention was to re-use the LuaTeX executable to implement a new typesetting language replacing TeX. LuaTeX produces excellent typography, 
but with a terrible document description language. I wondered why apparently no one created a decent document description language for LuaTeX yet.
So I started on December 20 to declutter LuaTeX 1.10 and migrate it to my BUSY build system, so it was easy to build and debug the stand-alone executable. 
But eventually not even a Hello World printed correctly to the PDF. See the readme of the "first-attempt" branch for more information. 

So I made another attempt starting on December 26 based on LuaHBTeX 1.18, which includes the HarfBuzz library, and thus in principle everything necessary
to get all required information out of Open or TrueType fonts (thus reducing the dependency on the humongous luaotfload script). I also decluttered and
migrated this version to the BUSY build system. I implemented a font loader in C (see font_loader.c) which works in principle; OTF/TTF fonts can be loaded 
completely without an external Lua implementation including the math parameters. My test cases worked in principle, but the typographical quality was terrible. 
I didn't see a straight-forward way to migrate the essential functionality from the large Lua and TeX code to C, and reusing those parts verbatim contradicts
my goal to implement a lean, stand-alone application. See the readme of the "second-attempt" branch for more information. 

### Status on January 1, 2026

I now have all the fundamental libraries in the repository, migrated to the BUSY build system, and stripped of unused files. The project includes a test
application which draws some geometric figures and text lines with different font sizes in a PDF. Cairo thereby directly uses the font face from FreeType
without any dependency on fontconfig or other operating system font APIs. Next I will integrate MicroTex and test formula rendering.

### How to build

Use the [BUSY build system](https://github.com/rochus-keller/busy/) or open the BUSY file with [LeanCreator](https://github.com/rochus-keller/leancreator/).
So far there is only a BUSY file in the root directory.

The build was successfully tested on Linux x64. It is likely to also work on Linux x86 or macOS. 
Win32 has not been tested yet and likely doesn't build without changes.

### Additional Credits

TBD

