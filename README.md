ucore-thumips [![Build Status](https://travis-ci.org/z4yx/ucore-thumips.svg?branch=ucore-fix)](https://travis-ci.org/z4yx/ucore-thumips)
=============

Build
-----

Build for FPGA:

`make CROSS_COMPILE=<path-to-mips-mti-elf>/bin/mips-mti-elf- ON_FPGA=y -j4`

Simulation
----

`make CROSS_COMPILE=<path-to-mips-mti-elf>/bin/mips-mti-elf- ON_FPGA=n qemu`

More
---

See `doc` folder.
