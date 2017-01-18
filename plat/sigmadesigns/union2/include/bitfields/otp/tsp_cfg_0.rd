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

tsp_des_dis		 0  1
tsp_tdes_dis		 1  1
tsp_aes128_dis		 2  1
tsp_multi2_dis		 3  1
tsp_dvb_dis		 4  1
tsp_conf_dis		 5  1
tsp_ucode_key_dis	 6  1
tsp_dvb_variant_dis	 7  1
tsp_aes_variant_dis	 8  1
tsp_nds_csa2_variants_dis  9  1
tsp_dss_dis		11  1
