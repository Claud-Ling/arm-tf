#
# Register Descriptor file (RD file, in *.rd) Template
#
# Generally, RD file is used to describe 32-bit long register,
# and each line of it represents a bit field in that register.
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

a53_clk_sel		 0  1 #0:CLK24M, 1:A53_PLL
a53_clk_on		 1  1 #0:OFF, 1:ON
a53_csclk_on		 2  1
a53_csdbg_ctrl		 3  1
irom_secure		 4  1
iram_secure		 5  1
boot_rom_pd		 6  1 #Boot ROM COT power down
boot_ram_pd		 7  1 #Boot RAM COT power down
a53_cache_pd		 8  1 #A53 cache ram power down
a53_ram_margin		 9  6 #reserved
gic_secure		16  1 #GIC register secure mode
timestamp_clear		17  1
timestamp_debug_halt	18  1 #timestamp counter halt enable by debug
warmreq_disable		19  1 #Disable warm reset request from CPU
outstanding		20  3 #outstanding ability
