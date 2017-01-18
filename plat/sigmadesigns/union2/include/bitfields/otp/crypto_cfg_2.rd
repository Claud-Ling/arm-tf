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

kc_memdest_dis		 2  1
kc_keydest_dis		 3  1
kv_w_rstct_kc		 5  1
kv_u_rstct_kc_bc	10  1
kv_r_rstct_mmio_kc	11  1
kv_w_rstct_gkl		12  1 #key-vault write permit for ssi (only KL1 and KL2 can write through ssi), 1= both KL1 and KL2 can not write key-vault
