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

decoder_format_dis	  0  7 #video decoder format disable: bit[0] H264, bit[1] VC1, bit[2] MPEG2, bit[3] AVS, bit[4] MPEG4, bit[5] RV, bit[6] ON2
melo_d_dis		  8  4 #MeloD control (Audio output control); note the most significant bit is currently not used
gfx_3d_dis		 12  1 #1 = 3D Graphics accelerator disabled
edr_dis			 13  1 #1 = EDR disabled
uhd_dis			 14  1 #1 = Vx1 UHD disabled
demod_dvbt_dis		 19  1
demod_analog_dis	 20  1
demod_atsc_dis		 21  1
demod_dvbc_dis		 22  1
demod_isdbt_dis		 23  1
audio_srs_dis		 24  1
audio_bbe_dis		 25  1
audio_soundprojector_dis 26  1
