#
# Turing Descriptor File (TD)
# Syntax:
# <name> <ofs> [width] [comment]
#
# <name> specifies the register/block name, mandatory, derived from SPG at most.
# <ofs> shall be present in hexdecimal and must be word aligned (4-byte), mandatory.
# [width] default to 4 and shall be present in hexdecimal, word aligned if specified.
# [comment] is typically lead by a character '#'.
#

# 0x2000 (acc)
acc_operand		2000	044	#Modulus Operand up to 68x32 bits
acc_control		2f00		#ACC control
acc_op_sel		2f04		#Operand selection
acc_debug		2f08		#ACC debug status
acc_int_en_clr		2fd8		#ACC INT enable clear
acc_int_en_set		2fdc		#ACC INT enable set
acc_int_status		2fe0		#ACC INT status
acc_int_enable		2fe4		#ACC INT enable state
acc_int_st_clr		2fe8		#ACC INT status clear
acc_int_st_set		2fec		#ACC INT status set
acc_reset		2ff0		#ACC engine soft reset
acc_module_id		2ffc		#Module ID of ACC
acc_share_operand	3000	110	#Shared Operand Space upto 272x32 bits

# 0x4000 (cipher)
crypto_caps		4000		#Crypto capabilities
crypto_int_enable	4008		#Crypto interrupt enable
crypto_int_status	400c		#Crypto interrupt status
crypto_power_save	4010		#Crypto power saving
chan_acc_req		4020		#Channel access request

kc_control		4100		#Key Channel Control
kc_status		4104		#Key Channel Status
kc_kvi_src		4110		#Key Channel key vault index of source data
kc_kvi_dst		4114		#Key Channel key vault index of dest data
kc_mode			4118		#Key Channel Mode
kc_data_src		4120	010	#Key Channel data registers
kc_data_dst		4130	010	#Key Channel key out registers

bc_control		4200		#Bulk Channel Control
bc_size			4204		#Bulk Channel byte size
bc_mode			4208		#Bulk Channel Mode
bc_inc			420c		#Bulk Channel Increment
bc_iv			4210	010	#Bulk Channel IV for CBC/CTR
bc_status		425c		#Bulk Channel Status
bc_status_slave		4264		#Bulk Channel slave mode status
bc_buf_status		4268		#Bulk Channel buffer status
bc_addr_in		4270		#Bulk Channel input addr for slave mode
bc_addr_out		4274		#Bulk Channel output addr for slave mode
bc_reset		4278		#Bulk Channel Reset

vsm_dma_cmd		4300		#VSM DMA Command
vsm_dma_nda_a		4304		#VSM DMA Address of NDA A
vsm_dma_nda_b		4308		#VSM DMA Address of NDA B
vsm_dma_int_status	43e0		#VSM DMA Interrupt Status
vsm_dma_int_en		43e4		#VSM DMA Interrupt Enable
vsm_dma_int_clr		43e8		#VSM DMA Interrupt Clear
vsm_dma_int_set		43ec		#VSM DMA Interrupt Set
vsm_dma_int_out		43f0		#VSM DMA Interrupt Out (status|set)&en

kv_ram			4800	080	#Key Vault RAM (8 * 16)
kv_attr			4a00	040	#Key Vault Attributes (4 * 16)
crypto_module_id	4ffc		#Module ID of Crypto module

# 0x5000 (ckl)
ckl_param		5000		#KL Parameter
ckl_control		5004		#KL Control
ckl_status		5008		#KL Status
ckl_key			5010	010	#KL Input Key
ckl_key_attr		5020		#KL Key Attribute
ckl_module_id		5ffc		#Module ID of KL

# 0x6000 (sha)
sam_command		6000		#SAM command
sam_msg_addr		6004		#SAM message addr
sam_msg_len		6008		#SAM message length
sam_hash		6100	020	#SAM hash value
sam_count		6140		#SAM SHA byte count (up to 512M)
sam_int_en_clr		6fd8		#SAM INT enable clear
sam_int_en_set		6fdc		#SAM INT enable set
sam_int_en		6fe0		#SAM INT enable state
sam_int_status		6fe4		#SAM INT status
sam_int_st_clr		6fe8		#SAM INT status clear
sam_int_st_set		6fec		#SAM INT status set
sam_module_id		6ffc		#Module ID of SAM module

# 0x7000 (gps)
random_number_0		7004		#RANDOM_NUMBER_0
gps_module_id		7ffc		#Module ID of GPS module

# 0x8000 (ckl5, 3~5 level key ladder)
ckl5_param		8000		#KL5 Parameter
ckl5_control		8004		#KL5 Control
ckl5_status		8008		#KL5 Status
ckl5_key		800c	050	#KL5 Input Key
ckl5_key_attr		8060		#KL5 Key Attribute
ckl5_moduel_id		8ffc		#Module ID of KL5
