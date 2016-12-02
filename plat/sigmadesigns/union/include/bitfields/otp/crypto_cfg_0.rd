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

crypto_host_key_phase_ctl_dis	 0  1
crypto_pass_thru_dis		 1  1
jtagpass_load_dis		 2  1
otp0_memdest_dis		 5  1
otp1_memdest_dis		 6  1
otp2_memdest_dis		 7  1
otp3_memdest_dis		 8  1
otp4_memdest_dis		 9  1
otp5_memdest_dis		10  1
otp0_keystor_dis		11  1
otp1_keystor_dis		12  1
otp2_keystor_dis		13  1
otp3_keystor_dis		14  1
otp4_keystor_dis		15  1
otp5_keystor_dis		16  1
otp0_jtagpass_dis		17  1
otp1_jtagpass_dis		18  1
otp2_jtagpass_dis		19  1
otp3_jtagpass_dis		20  1
otp4_jtagpass_dis		21  1
otp5_jtagpass_dis		22  1
otp0_rest_sec_proc		23  1
otp1_rest_sec_proc		24  1
otp2_rest_sec_proc		25  1
otp3_rest_sec_proc		26  1
otp4_rest_sec_proc		27  1
otp5_rest_sec_proc		28  1
crypto_des_dis			29  1
crypto_tdes_dis			30  1
crypto_aes128_256_dis		31  1
