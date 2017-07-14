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
# Key Channel Control Register
#
# bitfields
enable		 0  1
int_en		 3  1
cipher		 4  4	#2 = TDES3; 3 = TDES2; 4 = AES128
dst		 8  2	#0 = regs; 1 = KV; 2 = TSP; 3 = JTAG pwd reg
srcsel		10  1	#0 = KV; 1 = data regs
outmode		11  1	#0 = normal; 1 = last 64bits first
nv		15  1	#Key type
index		16  8
