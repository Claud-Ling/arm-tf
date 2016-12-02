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

design_id	 0  8 #chip revision ID: 0x00: A0; 0x01: A1; 0x02: A2; 0x03: A3; 0x04: A4;...0x05, A5
dts_sound_en	 8  1 #AUDIO DTS : DTS_SOUND_EN 1- DTS_SOUND_EN
dts_decode_en	 9  1 #AUDIO DTS : DTS_DECODE_EN 1- DTS_DECODE_EN
