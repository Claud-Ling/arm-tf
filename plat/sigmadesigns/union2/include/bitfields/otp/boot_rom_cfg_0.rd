#
# UNION2 fuse register descriptor file
# Based on <ProUNION2_revA_Fuse_Data_Map 20161201.xlsx>
#
# Register Descriptor file (RD file, in *.rd) Description
#
# Generally, RD file is used to describe 32-bit long register,
# and each line of it represents a bit field in that register.
#
# The file shall be named as 'fuse_entry_name'.rd for matching with FD file.
# Where 'fuse_entry_name' represents an fuse entry, which shall be derived 
# from Fuse_Data_Map file, in lowercase.
#
# Syntax:
# <name>  <start>  <bits> [comments]
#
# where,
#   <name>     specifies bit field name derived from Fuse_Data_Map.xlsx
#   <start>    specifies the start bit
#   <bits>     specifies number of bits
#   [comments] comments, optional
#
# Note, <start> and <bits> shall be given in decimal form
#
# Author:  Tony He
# Date:    2017/01/18
#

flash_extra_delay		 0 16 #in unit of ms for emmc or us for nand
nand_rb_sel			16  1 #nand ready/busy bit select
emmc_new_high_speed_en		17  1 #n/a
emmc_new_chg_clock_div		18  1 #n/a
emmc_stop_boot_en		19  1 #stop boot mode on exit
emmc_extra_delay_en		20  1
emmc_power_rst_on_boot_en	21  1 #power reset on boot
emmc_new_set_clock_div		22  1 #n/a
emmc_boot_switch_en		23  1
