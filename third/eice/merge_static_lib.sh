#!/bin/bash

function mergelinux() {
	ARSCRIPT_PATH=$1
	OUTPUT_LIB=$2
	ARGS=($@)
	# MERG_LIBS=${ARGS[2]} 
	MERG_LIBS=($@)
	unset MERG_LIBS[0]
	unset MERG_LIBS[1]
	# for a in $(MERG_LIBS); do (echo "a $a"); done

	# MERG_LIBS=${MERG_LIBS[@]} 


	echo ARSCRIPT_PATH=$ARSCRIPT_PATH
	echo OUTPUT_LIB=$OUTPUT_LIB
	echo MERG_LIBS=$MERG_LIBS

	echo "CREATE $OUTPUT_LIB" > $ARSCRIPT_PATH
	# for a in $(MERG_LIBS); do (echo "ADDLIB $$a" >> $(ARSCRIPT_PATH)); done
	for a in ${MERG_LIBS[@]}  
	do  
    echo "ADDLIB ${a}" >> $ARSCRIPT_PATH
	done 
#	$(SILENT)echo "ADDMOD $(OBJECTS)" >> $(ARSCRIPT_PATH)
	echo "SAVE" >> $ARSCRIPT_PATH
	echo "END" >> $ARSCRIPT_PATH
	ar -M < $ARSCRIPT_PATH
	rm $ARSCRIPT_PATH
}

function osx() {
	echo osx
}

$@
