#include <bl_common.h>
#include <platform_def.h>
#include <sd_private.h>
#include <sd_otp.h>

DEFINE_FUSE_MAP(SD_OTP_DATA_BASE);

/*
 * default ns otp list
 */
static otp_access_ctrl_t ns_fuse_lists[] = {
	{FUSE_OFS_FC_0, 0xffffffff},	/*FC_0*/
	{FUSE_OFS_FC_1, 0xffffffff},	/*FC_1*/
	{FUSE_OFS_FC_2, 0xffffffff},	/*FC_2*/
	{FUSE_OFS_FC_3, 0xffffffff},	/*FC_3*/
	{FUSE_OFS_DIE_ID_0, 0xffffffff},/*DIE_ID_0*/
	{FUSE_OFS_RSA_PUB_KEY, 0xffffffff},	/*RSA_PUB_KEY*/
	{-1, -1},			/*The end*/
};

/*
 * The following platform setup functions are weakly defined. They
 * provide typical implementations that will be overridden by a SoC.
 */
#pragma weak sd_soc_otp_get_ns_list

otp_access_ctrl_t * sd_soc_otp_get_ns_list(void)
{
	return ns_fuse_lists;
}
