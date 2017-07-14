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
# Date:    2017/06/14
#

#
# ACC INT Status Register
#
# bitfields
done		 0  1
r_val_err	 1  1
reg_sel_err	 2  1
lock_err	 3  1
opcode_err	 4  1
size_err	 5  1
