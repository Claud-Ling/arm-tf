SigmaDesigns DTV SX SoCs - Overview
======================

Contents
--------

1.  [Directory structure](#Directory-structure)
2.  [Introduction](#Introduction)
3.  [How to build](#How-to-build)
4.  [Build flags](#Build-flags)
5.  [Power management](#Power-management)

- - - - - - - - - - - - - - - - - -

Directory structure
-------------------

* plat/sigmadesigns/common - Common code for all DTV SoCs
* plat/sigmadesigns/soc/   - Chip specific code

Introduction
------------

How to build
------------

'CROSS_COMPILE=<path-to-aarch64-gcc>/bin/aarch64-linux-gnu make PLAT=<soc e.g. union> \
SPD=<dispatcher e.g. tspd> BL33=<path-to-u-boot.bin> bl1 fip'

Build flags
-------------

Platforms wanting to use different build options, can add 'OPTION=<value>'
to the build command line.


*   **BOOTDEV**
    Defines the boot device for the platform. The supporting value includes
    emmc|nand|nor. Default use emmc.

*   **STORAGE**
    Defines the main storage type for the platform. The supporting value includes
    emmc|nand|nor. This option is only required when 'BOOTDEV=nor'

*   **CONFIG_DDR**
    Boolean flag that is used to force reconfigure DDR settings. Default 0.

*   **DDR_TABLE**
    Defines path to ddr_table root. A menu for ddr setting selection will pop up
    based on it when 'CONFIG_DDR=1' or ddr setting has not been configured yet.

*   **DEBUG_EMMC**
    Boolean flag that is used to turn on debug within emmc driver. Default 0.

*   **DUMMY_EMMC**
    Boolean flag that is used to turn on choose dummy emmc driver. Default 0.
    Firmware will load FIP image from DRAM at 128M address instead of emmc
    flash when 'DUMMY_EMMC=1'.

*   **DEBUG_DDR**
    Boolean flag that is used to turn on debug within ddr driver. Default 0.

*   **SECURE**
    Boolean flag that is used to turn on security protections within firmware.
    Default 1.


Power management
----------------

