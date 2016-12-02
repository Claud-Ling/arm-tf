#
# UNION fuse register descriptor file
# Based on <ProUNION_revA_Fuse_Data_Map 20160815.xlsx>
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
# Date:    2016/11/25
#

boot_from_rom			 0  1
sec_boot_en			 1  1
boot_val_user_id		 2  5
rsa_key_flsh_val		 7  1
flash_res_rsa_key_gen_check	 8  1
rsa_comp			 9  1
flash_auth_shutdown_on_fail	10  1
boot_code_dbg_print_en		11  1 #1 = Enable debug print (out UART port) in ROM code
new_nand_ctrl_sel		12  1 #1 = use new NAND controller; 0 = use old NAND controller
nand_id_dis			13  1 #used by ROM code
uart_prot_en			14  1 #0 = UART is open; 1 = UART is protected by HOST_DBGPORT_PROT
emmc_clock_divider		15 10
emmc_bit_width			25  2 #0 = 1 bit; 1 = reserved; 2 = 4 bits; 3 = 8 bits
emmc_high_speed			27  1 #0 = normal speed mode; 1 = high speed mode
emmc_ddr_mode			28  1 #0 = non DDR mode; 1 = DDR mode
a9axi_resp_ok			29  1
a9_spiden_dis			30  1
a9_spniden_dis			31  1


