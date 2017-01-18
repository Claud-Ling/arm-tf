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
# from Fuse_Data_Map.xlsx file, in lowercase.
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

nand_addr_cycle		 0  4
nand_page_size		 4  4 #0 = 512; 1 = 2K; 2 = 4K; 3 = 8K; 4 = 16K
nand_parameter_location	 8  1 #0 = fuse; 1 = configure pins
nand_backup_page_step	 9  4 #in unit of 16
nand_ecc_bits		13  5 #in unit of 2
nand_ecc_unit		18  1 #0 = 512; 1 = 1024
nand_reset_en		19  1 #reset on boot entry
spi_read_mode		22  3
spi_clk_speed		25  4
flash_type		29  3 #001 = OneNand; 010 = SPI; 100 = NAND; 101 = eMMC
