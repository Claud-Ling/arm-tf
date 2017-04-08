#!/bin/bash -e


g_pman_table_name_array=""
g_pman_table_count=0
g_pman_configration=""
ret=0
PMAN_TAB_ROOT=$1
CONFIG=$2
PMAN_SETTING_FILE=$3
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

	This helper script offers user a text menu to select one pman setting file for
	use from the specified directory.
	Usage: ./${0##*/} <pman_table_root> <config_file> <output_file>
	  <pman_table_root>	- the root directory of pman_table
	  <config_file>		- pman setting configure file
	  <setting_file>	- pman setting output file

	EOF
	exit 1
}

#
# get_pman_table_list <pman_table_PATH> [subdir]
#
get_pman_table_list() {
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

	g_pman_table_name_array=${array}
	g_pman_table_count=$((count-1))
}

#
# get entry name by given index
# get_entry_name <index>
#
get_entry_name() {
	local count=0
	for i in ${g_pman_table_name_array}
	do
		count=$((count+1))
		if [ $count -eq $1 ] ; then
			echo $i
			break
		fi
	done
}

#
# check_input_value [input_data, g_pman_table_count]
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
# copy_file_to_pmansetting [file_name]
#
copy_file_to_pmansetting() {
	cp $PMAN_TAB_ROOT/$1 ${PMAN_SETTING_FILE}
}
gernerate_selection_record() {
	echo $1 > $CONFIG
}
introduction() {
	ECHO_GREEN "==============================================================================\n"
	echo -n -e "#\t\t\t Please select a appropriate PMAN Secure configration\n"
	echo -n -e "#Example:\n"
	echo -n -e "#\tpman_setting.txt\n"
	echo -n -e "# "; ECHO_RED "Cann't find configration you need ?\n"
	ECHO_GREEN "==============================================================================\n"
}
select_pman_configration() {
	local subdir=
	local file=
	local ret=
	while true
	do
		introduction
		[ $subdir ] && echo "in '\${base}${subdir}'"
		get_pman_table_list ${PMAN_TAB_ROOT}${subdir} ${subdir}
		#echo ${g_pman_table_name_array}
		#echo ${g_pman_table_count}
		echo -n "Please select a PMAN configration:  "
		read x
		#echo "Input is ${x}"

		check_input_value $x $g_pman_table_count
		ret=$?
		#echo "ret = ${ret}"
		if [ $ret -eq 0 ] ; then
			# check selected file
			file=`get_entry_name $x`
			if [ $file = $parentdir ]; then
				subdir=${subdir%/*}
				continue
			elif [ -d ${PMAN_TAB_ROOT}${subdir}/$file ]; then
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
	g_pman_configration=${subdir}/${file}

	copy_file_to_pmansetting $g_pman_configration
	ret=$?
	if [ $ret -ne 0 ] ; then
		ECHO_RED "copy file to pmansetting failed\n"
		return $ret
	fi

	gernerate_selection_record $g_pman_configration
}

notice() {
	echo "=============================================================================="
	echo -n -e "#"; ECHO_GREEN "\t\t\t PMAN Configration \t\t\n"
	echo -n -e "#\t Your PMAN configration is:\n"
	echo -n -e "#"; ECHO_YELLOW "\t\t\t ${g_pman_configration#/} \n"
	echo -n -e "#If you want select different PMAN configration, \n"
	echo -n	-e "#please run build.sh with arg '"; ECHO_RED "config-pman"; echo -n -e "' \n"
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
	config-pman)
		select_pman_configration
		if [ $? -ne 0 ]; then
			rm $CONFIG
			echo "PMAN config error"
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
	if [ -f $CONFIG -a -f "$PMAN_TAB_ROOT/`cat $CONFIG`" ] ; then
		g_pman_configration=`cat $CONFIG`
		copy_file_to_pmansetting $g_pman_configration
		if [ $? -eq 0 ]; then
			notice
		else
			rm $CONFIG
			echo "Cann't find PMAN configration file"
			exit 1
		fi
		exit $?
	else
		select_pman_configration
		if [ $? -eq 0 ]; then
			notice
		else
			rm $CONFIG
			echo "PMAN config error"
			exit 1
		fi
		exit $?
	fi
fi
