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
# Date:    2017/06/13
#

#
# Bulk Channel Control Register
#
# bitfields
enable		 0  1
enc		 1  1	#0 = Dec; 1 = Enc
int_en		 3  1
cipher		 4  4	#0 = Pass thr; 1 = DES; 2 = TDES3; 3 = TDES2; 4 = AES128; 8 = M6; 9 = AES256; 12 = RC4
dma_int_en	10  1
nv		15  1	#Key type
index		16  8
submode		24  4	#0 = ECB; 2 = CBC; 3 = CTR
localmode	28  4	#Local Scrambling Mode
