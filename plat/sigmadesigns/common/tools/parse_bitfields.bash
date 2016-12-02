#!/bin/bash -e

usage()
{
	cat <<-EOF >& 2

	Generate dst_dir/*.h files from src_dir/*.rd files.
	Usage: ./${0##*/} <src_dir> [dst_dir]
	  <src_dir>	- source directory to search for *.rd files
	  [dst_ir]	- destination directory to generate .h files. Same as <src_dir> if omitted

	EOF
	exit 1

}

[ $# -lt 1 ] && usage

TOOL_DIR=$(dirname $(readlink -f ${BASH_SOURCE}))
SRC_DIR=$1
if [ $# = 1 ]; then
	DST_DIR=$SRC_DIR
else
	DST_DIR=$2
fi

for x in `find $SRC_DIR -name *.rd`; do
	if [ $SRC_DIR != $DST_DIR ]; then
		output=${DST_DIR}${x#${SRC_DIR}}
		output=${output%.*}.h
		mkdir -p $(dirname $output)
	else
		output=${x%.*}.h
	fi

	echo "  BF      $x"
	# generate $DST_DIR/*.h files from $SRC_DIR/*.rd files
	perl -Iperl ${TOOL_DIR}/perl/bitfield.pl $x >$output;
done

exit 0
