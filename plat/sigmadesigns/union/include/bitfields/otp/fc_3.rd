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

sec_dis			 0  1 #1 = Security disable
boot_from_rom_dis	 1  1
select_root_key_kl2	 2  2 #Selects which OTP key is used by KL2:  0=OTP_KEY_3, 1=OTP_KEY_4; 2=OTP_KEY_5
fin_dest_kl2		 4  2 #Destination allowed for final key in KL2: 00=both; 01=KV; 10=TSP; 11=neither
lock_root_key_kl2	 6  1 #1 = Root key is locked according to the SELECT_ROOT_KEY_KL2 fuse bit for the KL2; 0 = Root key is selectable by the Host
disable_kl2		 7  1
level_sel_kl2		 8  2 #operation level selection of KL2: 0 = 2-level; 1 = 3-level; 2 = 4-level; 3 = 5-level
kl2_final_key_att_from_otp 10  1
a53_single_core		11  1 #configure whether we need to fuse off multi-CPUs. Default 1'b0; 0 = mult-core; 1 = single core enable
