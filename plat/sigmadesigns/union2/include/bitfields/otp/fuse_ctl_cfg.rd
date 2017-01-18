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

sig_prog		 0  1 #1 = FC signature has been programmed (this bit is blown automatically by FC HW)
obf_data_prog		 1  1 #1 = FC obfuscation data has been programmed (this bit is blown automatically by FC HW)
debug_dis		 2  1 #1 = FC has debug capability disabled; debug registers may not be accessed and debug bus is unavailable.
factory_mode_dis	 3  1 #1 = FC will use a longer (and slower) boot process where every fuse is read two times.
sm_mismatch_check_en	 4  1
sm_one_hot_check_en	 5  1
non_trusted_global_wp	 6  1
trusted_global_wp	 7  1
non_trusted_global_rp	 8  1
trusted_global_rp	 9  1
external_wp_en		10  1 #1 = Allow an external HW module to block write access to fuses
soft_prog_enable_code	16 16 #Correct 16 bit code = Allow soft programming; any other value disables it
