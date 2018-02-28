#!/bin/bash

##############################################
PIN_PATH=/home/majid/phd/pin/pin-3.2-81205-gcc-linux
PIN_BIN=$PIN_PATH/pin 
MEM_APPROX_TOOL=$PIN_PATH/source/tools/ManualExamples/obj-intel64/memapprox.so

##############################################
APP_BIN=$PIN_PATH/examples/canny/bin/x86/canny

VIDEO=bunny

INPUT=$PIN_PATH/examples/inputs/$VIDEO/$VIDEO
OUTPUT=$PIN_PATH/examples/canny/outputs/$VIDEO/$VIDEO

TL=0.45098039215686275
TH=0.8941176470588236

NUM_FRAMES=10
REPEAT_FRAME=1
SAMPLING_FREQ=1

INITIAL_READ_BER=0
INITIAL_WRITE_BER=1E-4

# CALIBRATE=-calibrate
REPORT_ERROR=-report-error
SAVE_OUTPUT=-save-output
##############################################
$PIN_BIN \
-t $MEM_APPROX_TOOL -- \
$APP_BIN \
-in $INPUT \
-out $OUTPUT \
-sigma 0.33 \
-tlow $TL \
-thigh $TH \
-num-frames $NUM_FRAMES \
$SAVE_OUTPUT \
$CALIBRATE \
-read-ber $INITIAL_READ_BER \
-write-ber $INITIAL_WRITE_BER \
$REPORT_ERROR \
-sampling-freq $SAMPLING_FREQ \
-repeat-frame $REPEAT_FRAME \
-control-mode 0