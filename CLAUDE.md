This repository contains source files for my DIY eurorack synthesizer project.
I have built a KOSMO-sized synthesizer already, some modules of which are located in the 'kosmo' folder.
I am currently designing a new version in eurorack format, and I want to start learning SMD soldering as part of it. If some of my through-hole habits (e.g., choice of specific parts) should be changed, please let me know.
Some of my PCB constraints are non-intuitive: I try to keep the height of all PCBs <= 100mm, as PCBs become drastically more expensive if they are larger.

Each module is in one folder, prefixed by a number. I refer to the modules by their number. Eurorack modules are referred to by their numeric prefix (e.g. "module 01" means `01_as3340_vco`); kosmo modules are referred to by their name, since their folders are prefixed with `kosmo_` instead of a number (e.g. `kosmo_poly_midi_2_cv`). From previous experiences, it seems most efficient if you read the `.net` files, instead of the raw KiCad schematics. If a netlist is not availabe, prompt me to generate it instead of reading the raw schematic file.

Don't update files immediately if the conversation is fundamental and different options are being considered. Discuss and settle on an approach first.
