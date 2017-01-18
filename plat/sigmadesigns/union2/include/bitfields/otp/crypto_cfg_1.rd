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

otp0_par_chk_dis	14  1
otp1_par_chk_dis	15  1
otp2_par_chk_dis	16  1
otp3_par_chk_dis	17  1
otp4_par_chk_dis	18  1
otp5_par_chk_dis	19  1
otp0_tdes_enc_dis	20  1
otp1_tdes_enc_dis	21  1
otp2_tdes_enc_dis	22  1
otp3_tdes_enc_dis	23  1
otp4_tdes_enc_dis	24  1
otp5_tdes_enc_dis	25  1
otp0_aes_enc_dis	26  1
otp1_aes_enc_dis	27  1
otp2_aes_enc_dis	28  1
otp3_aes_enc_dis	29  1
otp4_aes_enc_dis	30  1
otp5_aes_enc_dis	31  1
