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

hevc_dec_dis			 0  1
watermark_en			 3  1
mem_enc				 4  1
select_root_key_kl1		 5  2 #Selects which OTP key is used by KL1: 0=OTP_KEY_3, 1=OTP_KEY_4; 2=OTP_KEY_5
default_dft_tap_only_en		 7  1
cscan_mbist_dbgport_prot	 8  2
bscan_dbgport_prot		10  2
host_dbgport_prot		12  2
tsp_dbgport_prot		14  2
fin_dest_kl1			16  2 #Destination allowed for final key in KL1: 00=both; 01=KV; 10=TSP; 11=neither
lock_root_key_kl1		19  1 #1 = Root key is locked according to the SELECT_ROOT_KEY_KL1 fuse bit for the KL1; 0 = Root key is selectable by the Host
disable_kl1			20  1 #1 = Disable HW key ladder KL1
three_kl1			21  1 #1 = Key ladder KL1 is 3-level / 0 = 2-level
kl1_final_key_att_from_otp	22  1
i2c_debug_dis			23  1 #1 = i2c debug is disabled
watermark_id_from_otp		24  1 #1 = OTP; 0 = register
watermark_en_from_otp		25  1 #1 = OTP; 0 = register
watermark_operatorid_from_otp	26  1 #1 = OTP; 0 = register
itmo_dis			27  1 #1 = ITMO disabled
emmc_card_detect_time		28  4 #EMMC card detect time: 0000 = 2ms; 0001 = 10ms; 0010 = 25ms; 0011 = 50ms; 0100 = 100ms; 0101 = 200ms; 0110 = 250ms; 0111 = 300ms; 1000 = 400ms; 1001 = 500ms; 1010 = 600ms; 1011 = 700ms; 1100 = 750ms; 1101 = 800ms; 1110 = 900ms; 1111 = 1024ms
