#
# Register Descriptor file (RD file, in *.rd) Template
#
# Generally, RD file is used to describe 32-bit long register,
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

# register type
$type		b #8-bit register

# bitfields
reg_start	0 1 #0:first power on in mode A; 1:not first power on in mode A
bist_error10	1 2 #bist_error[1:0] of sram
stb_en		3 1
spi_en		4 1
bist_error32	5 2 #bist_error[3:2] of sram
