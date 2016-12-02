#!/bin/bash -e


g_ddr_table_name_array=""
g_ddr_table_count=0
g_ddr_configration=""
ret=0
DDR_TAB_ROOT=$1
CONFIG=$2
DDR_SETTING_FILE=$3
parentdir=".."

ECHO_RED() {
	echo -e -n "\033[31m${1}\033[0m"
}
ECHO_GREEN() {
	echo -e -n "\033[32m${1}\033[0m"
}
ECHO_YELLOW() {
	echo -e -n "\033[33m${1}\033[0m"
}
ECHO_BLUE() {
	echo -e -n "\033[34m${1}\033[0m"
}

usage()
{
	cat <<-EOF >& 2

	This helper script offers user a text menu to select one ddr setting file for
	use from the specified directory.
	Usage: ./${0##*/} <ddr_table_root> <config_file> <output_file>
	  <ddr_table_root>	- the root directory of ddr_table
	  <config_file>		- ddr setting configure file
	  <tuning_file>		- ddr setting output file

	EOF
	exit 1
}

#
# get_ddr_table_list <ddr_table_PATH> [subdir]
#
get_ddr_table_list() {
	local array=""
	local str=""
	local count=1

	#echo $IFS
	if [ $2 ]; then
		#virtual node
		array="${array} ${parentdir}"
		str="<$((count++))>  ${parentdir}"
		ECHO_BLUE "${str}\n"
	fi

	for i in `ls --group-directories-first $1`
	do
		array="${array} ${i}"
		str="<$((count++))>  ${i}"
		[ -d $1/$i ] && ECHO_BLUE "${str}\n" || echo $str
	done

	g_ddr_table_name_array=${array}
	g_ddr_table_count=$((count-1))
}

#
# get entry name by given index
# get_entry_name <index>
#
get_entry_name() {
	local count=0
	for i in ${g_ddr_table_name_array}
	do
		count=$((count+1))
		if [ $count -eq $1 ] ; then
			echo $i
			break
		fi
	done
}

#
# check_input_value [input_data, g_ddr_table_count]
# return:  0 is legal   !0 is illegal
check_input_value() {

	if [ "$1" -le "$2" ] ; then
		#echo "$1 <= $2"
		return 0
	else
		#echo "$1 > $2"
		return 1
	fi
}
#
# copy_file_to_ddrsetting [file_name]
#
copy_file_to_ddrsetting() {
	cp $DDR_TAB_ROOT/$1 ${DDR_SETTING_FILE}
}
gernerate_selection_record() {
	echo $1 > $CONFIG
}
introduction() {
	
	ECHO_GREEN "==============================================================================\n"
	echo -n -e "#\t\t\t Please select a appropriate DDR configration\n"
	echo -n -e "#Example:\n"
	echo -n -e "#\tPLL_800m_1024m_650m_400m-DDR_1280m-PANEL_lvds_120-OTH_xxx.txt\n"
	echo -n -e "#\tPLL:"; ECHO_YELLOW "ddr@800MHz, plf@1GHz, av@650MHz, disp@400MHz\n"
	echo -n -e "#\tDDR: "; ECHO_GREEN "1280 MB\n"
	echo -n -e "#\tPANEL: "; ECHO_BLUE "lvds 120Hz\n"
	echo -n -e "#\tOTHER: "; ECHO_RED "xxx\n"
	echo -n -e "# "; ECHO_RED "Cann't find configration you need ?\n"
	echo -n -e "# Get latest configrations from repo: \n"
	echo -n -e "#\tsvn co http://sh-swsvn.sdesigns.com/DTV/BootLoader/release/ddr_table\n"
	ECHO_GREEN "==============================================================================\n"
}
select_ddr_configration() {
	local subdir=
	local file=
	local ret=
	while true
	do
		introduction
		[ $subdir ] && echo "in '\${base}${subdir}'"
		get_ddr_table_list ${DDR_TAB_ROOT}${subdir} ${subdir}
		#echo ${g_ddr_table_name_array}
		#echo ${g_ddr_table_count}
		echo -n "Please select a DDR configration:  "
		read x
		#echo "Input is ${x}"
	
		check_input_value $x $g_ddr_table_count
		ret=$?
		#echo "ret = ${ret}"
		if [ $ret -eq 0 ] ; then
			# check selected file
			file=`get_entry_name $x`
			if [ $file = $parentdir ]; then
				subdir=${subdir%/*}
				continue
			elif [ -d ${DDR_TAB_ROOT}${subdir}/$file ]; then
				subdir=${subdir}/${file}
				continue
			fi
			echo "OK"
			break
		else
			ECHO_RED "Incorrect selection, please re-select the Configration! \n"
		fi
	done

	ECHO_GREEN "Your selected configration is "; ECHO_RED "${file} \n"
	g_ddr_configration=${subdir}/${file}

	copy_file_to_ddrsetting $g_ddr_configration
	ret=$?
	if [ $ret -ne 0 ] ; then
		ECHO_RED "copy file to ddrsetting failed\n"
		return $ret
	fi

	gernerate_selection_record $g_ddr_configration
}

notice() {
	echo "=============================================================================="
	echo -n -e "#"; ECHO_GREEN "\t\t\t DDR Configration \t\t\n"
	echo -n -e "#\t Your DDR configration is:\n"
	echo -n -e "#"; ECHO_YELLOW "\t\t\t ${g_ddr_configration#/} \n"
	echo -n -e "#If you want select different DDR configration, \n"
	echo -n	-e "#please run build.sh with arg '"; ECHO_RED "config-ddr"; echo -n -e "' \n"
	echo "=============================================================================="
	sleep 2
}
#
# main
#
#echo $#
[ $# -lt 3 ] && usage
if [ 4 = $# ] ; then
	case $4 in
	config-ddr)
		select_ddr_configration	
		if [ $? -ne 0 ]; then
			rm $CONFIG
			echo "DDR config error"
			exit 1
		else
			notice
			exit $?
		fi
		;;
		*)
		ECHO_RED "${1}: Unknown Option\n"
		exit 1
		;;
	esac
else
	if [ -f $CONFIG -a -f "$DDR_TAB_ROOT/`cat $CONFIG`" ] ; then
		g_ddr_configration=`cat $CONFIG`	
		copy_file_to_ddrsetting $g_ddr_configration
		if [ $? -eq 0 ]; then
			notice
		else
			rm $CONFIG
			echo "Cann't find DDR configration file"
			exit 1
		fi
		exit $?
	else
		select_ddr_configration
		if [ $? -eq 0 ]; then
			notice
		else
			rm $CONFIG
			echo "DDR config error"
			exit 1
		fi
		exit $?
	fi
fi
