#
# Register Descriptor file (RD file, in *.rd) Template
#
# Generally, RD file is used to describe register,
# and each line of it represents a bit field in that register.
#
# Syntax:
# Configure line, optional
# $<key> <val> [comments]
#
# where,
#   <key>      specifies key name.
#              for now it can only be string of:
#                  "type"      - specify register type, default 32-bit
#   <val>      specifies value for the specified key.
#              when <key> = "type", the allowed values and their meaning are:
#                  'b' or 'B'  - byte (8-bit)
#                  'h' or 'H'  - halfword (16-bit)
#                  'w' or 'W'  - word (32-bit)
#                  'q' or 'Q'  - quadword (64-bit)
#
# Bitfield line
# <name>  <start>  <bits> [comments]
#
# where,
#   <name>     specifies bit field name
#   <start>    specifies the start bit
#   <bits>     specifies number of bits
#   [comments] comments, optional
#
# Note, <start> and <bits> shall be given in decimal form
#
# Author:  Tony He
# Date:    2016/06/13
#

# bitfields
cipher		 0  4	#3 = TDES2-ECB; 4 = AES-ECB
rksel		10  2	#0 = OTP3; 1 = OTP4; 2 = OTP5
dki		12 10	#dest key index
klen		24  4	#1 = 64bit; 3|4 = 128bit
cwsel		28  1	#0 = lsb; 1 = msb
dst		29  2	#0 = KV; 1 = TSP KT
