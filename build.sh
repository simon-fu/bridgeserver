
TOP_PATH=$(cd `dirname $0`; pwd)

echo CMD_ARGS=$*
echo TOP_PATH=$TOP_PATH

function eice() {
	# PJ_PATH="/mnt/hgfs/simon/projects/easemob/src/voice/pj"
	if test -n "$PJ_PATH"; then
	  PJ_PATH_STR="PJ_PATH=$PJ_PATH"
	fi

	EICE_BUILD_CMD="make -f $TOP_PATH/third/eice/Makefile $PJ_PATH_STR LIB_PATH=$TOP_PATH/lib/ $@"
	echo PJ_PATH_STR=$PJ_PATH_STR
	echo EICE_BUILD_CMD=$EICE_BUILD_CMD
	$EICE_BUILD_CMD
}

function bridge() {
	make -f $TOP_PATH/Makefile $@
}

if [ "x$1" = "xeice" ] ; then
	$@
else
	bridge $@
fi










