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

tsp_kt_acpu_key_read_dis	 0  1
tsp_kt_acpu_key_write_dis	 1  1
tsp_kt_pcpu_key_read_dis	 2  1
tsp_kt_pcpu_key_write_dis	 3  1
tsp_kt_acpu_switch_read_dis	 4  1
tsp_kt_acpu_switch_write_dis	 5  1
tsp_kt_pcpu_switch_read_dis	 6  1
tsp_kt_pcpu_switch_write_dis	 7  1
tsp_kt_proxy_tkw_read_dis	 8  1
tsp_kt_proxy_tkw_write_dis	 9  1
tsp_kt_esp_tkw_read_dis		10  1
tsp_kt_esp_tkw_write_dis	11  1
tsp_kt_crypto_tkw_read_dis	12  1
tsp_kt_crypto_tkw_write_dis	13  1
tsp_kt_avk_access		14  1
tsp_kt_nsk_access		15  1
tsp_kt_key_control_tkw_read_dis	16  1
tsp_kt_key_control_tkw_write_dis 17  1
tsp_kt_unspecified_requestor_access 18  1
tsp_kt_enforce_hif_auth_table_a	19  1
tsp_kt_enforce_hif_auth_table_b	20  1
tsp_kt_enforce_hif_auth_table_c	21  1
tsp_kt_enforce_hif_auth_table_d	22  1
tsp_kt_enforce_hif_auth_table_e	23  1
tsp_kt_enforce_hif_auth_table_f	24  1
tsp_kt_enforce_hif_auth_table_g	25  1
tsp_kt_enforce_hif_auth_table_h	26  1
tsp_kt_acpu_hif_write_dis	31  1
