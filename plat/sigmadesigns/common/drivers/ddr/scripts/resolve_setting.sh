#!/bin/bash -e
# tony@2013-8-27

usage()
{
	cat <<-EOF >& 2
	usage: ./${0##*/} <setting-name> [config-file]
	  <setting-name>	- "PLL_xx_xx-DDR_xx_xx-PANEL_xx_xx-OTH_xx_xx.txt" by convention
	  [config-file]		- target configure file, optional

	EOF
	exit 1
}

## inputs : "PLL_Mxxx_ARMxxx_Pxxx_SRCxxx"
## outputs: "MEM@xxx CPU@xxx PCLK@xxx SRC@xxx"
do_part_pll()
{
	local str=$1
	local prefix="PLL_"
	local settings=$(echo ${str#${prefix}} | sed 's/_/ /g')
	for each in $settings
	do
		case "$each" in
			M*)	echo -n "MEM@$(echo "obase=10;ibase=10;
				$(echo $each | sed 's/[A-Za-z]//g')*2" | bc)$(echo ${each#M} | sed 's/[0-9]//g') "
				;;
			ARM*) 	echo -n "CPU@$(echo ${each#ARM}) "	;;
			P*)	echo -n "PCLK@$(echo ${each#P}) "	;;
			SRC*)	echo -n "SRC@$(echo ${each#SRC}) "	;;
			AV*)	echo -n "AV@$(echo ${each#AV}) "	;;
			*)	echo -n "'$each' "			;; #unknown PLL items
		esac
	done
}

## inputs : "DDR_xxx"
## outputs: "xxxGB(MB)"
do_part_ddr()
{
	local str=$1
	local prefix="DDR_"
	local settings=$(echo ${str#${prefix}} | sed 's/g/GB/g' | sed 's/m/MB/g')
	echo "$settings"
}

## inputs : "PANEL_xx(xx)_xx"
## outputs: "xxHZ(xx) xx"
do_part_panel()
{
	local str=$1
	local prefix="PANEL_"
	local settings=$(echo ${str#${prefix}} | sed 's/_/ /g')
	echo $(echo $settings | awk '{print $2}' | sed 's/(/HZ(/g')
}

## inputs : "OTH_xx_xx"
## outputs: "xx xx"
do_part_other()
{
	local str=$1
	local prefix="OTH_"
	local settings=$(echo ${str#${prefix}} | sed 's/_/ /g')
	echo "$settings"
}

##
## resolve DDR settings
## inputs: DDR setting file, naming in pattern "PLL_xx_xx-DDR_xx_xx-PANEL_xx_xx-OTH_xx_xx.txt"
## outputs: update CONFIG_RT_INFO_xxx in include/config_all.h
##
do_resolve()
{
	local str=$1
	local cfg=${2:-"include/config_all.h"}
	local postfix=".txt"
	local inputs=$(echo ${str%${postfix}} | sed 's/-/ /g')
	local PLLINFO=
	local DDRINFO=
	local PANELINFO=
	local OTHINFO=
	[ -z "$str" ] && usage

	for each in $inputs
	do
		case "$each" in
			PLL*)	PLLINFO=$(do_part_pll $each)		;;
			DDR*)	DDRINFO=$(do_part_ddr $each)		;;
			PANEL*)	PANELINFO=$(do_part_panel $each)	;;
			OTH*)	OTHINFO=$(do_part_other $each)		;;
			*) echo "  WARN    Sorry '$each' is not a valid tag" >& 2 ;;
		esac
	done

	if [ -f "${cfg}" ]; then
		sed -i "s/define CONFIG_RT_INFO_PLL.*/define CONFIG_RT_INFO_PLL\t\"${PLLINFO}\"/g" ${cfg}
		sed -i "s/define CONFIG_RT_INFO_DDR.*/define CONFIG_RT_INFO_DDR\t\"${DDRINFO}\"/g" ${cfg}
		sed -i "s/define CONFIG_RT_INFO_PANEL.*/define CONFIG_RT_INFO_PANEL\t\"${PANELINFO}\"/g" ${cfg}
		sed -i "s/define CONFIG_RT_INFO_OTH.*/define CONFIG_RT_INFO_OTH\t\"${OTHINFO}\"/g" ${cfg}
	else
		echo "#define CONFIG_RT_INFO_PLL	\"${PLLINFO}\""
		echo "#define CONFIG_RT_INFO_DDR	\"${DDRINFO}\""
		echo "#define CONFIG_RT_INFO_PANEL	\"${PANELINFO}\""
		echo "#define CONFIG_RT_INFO_OTH	\"${OTHINFO}\""
	fi
}

do_resolve "$@"
