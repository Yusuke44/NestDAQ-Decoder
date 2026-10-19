#!/bin/bash

#Setting
EXEC_CMD="/home/oper/spadi/build/hul-common-lib/Common/read_scr"
IP1="192.168.19.101" #StrHUL TypeA
IP2="192.168.19.102" #StrHUL TypeB
IP3="192.168.20.00" #AMANEQ StrLrTDC

CON="run0001_"

OF1="/home/oper/users/nakayama/scr_data/${CON}scr_hulA_0.bin"
OF2="/home/oper/users/nakayama/scr_data/${CON}scr_hulA_1.bin"
OF3="/home/oper/users/nakayama/scr_data/${CON}scr_hulA_2.bin"
OF4="/home/oper/users/nakayama/scr_data/${CON}scr_hulB_0.bin"
OF5="/home/oper/users/nakayama/scr_data/${CON}scr_hulB_1.bin"
OF6="/home/oper/users/nakayama/scr_data/${CON}scr_hulB_2.bin"
OF7="/home/oper/users/nakayama/scr_data/${CON}scr_AMA_0.bin"
OF8="/home/oper/users/nakayama/scr_data/${CON}scr_AMA_1.bin"
OF9="/home/oper/users/nakayama/scr_data/${CON}scr_AMA_2.bin"

MAX_RUNS=5
INTERVAL_SEC=10


echo "Start data taking: MaxRuns=${MAX_RUNS} Interval=${INTERVAL_SEC}s"

for i in $(seq 1 $MAX_RUNS); do
    if [ $i -eq 1 ]; then
        $EXEC_CMD $IP1 $OF1 0
        $EXEC_CMD $IP1 $OF2 1
        $EXEC_CMD $IP1 $OF3 2
        $EXEC_CMD $IP2 $OF4 0
        $EXEC_CMD $IP2 $OF5 1
        $EXEC_CMD $IP2 $OF6 2
        $EXEC_CMD $IP3 $OF7 0
        $EXEC_CMD $IP3 $OF8 1
        $EXEC_CMD $IP3 $OF9 2

    else
        $EXEC_CMD $IP1 $OF1 0 append
        $EXEC_CMD $IP1 $OF2 1 append
        $EXEC_CMD $IP1 $OF3 2 append
        $EXEC_CMD $IP2 $OF4 0 append
        $EXEC_CMD $IP2 $OF5 1 append
        $EXEC_CMD $IP2 $OF6 2 append
        $EXEC_CMD $IP3 $OF7 0 append
        $EXEC_CMD $IP3 $OF8 1 append
        $EXEC_CMD $IP3 $OF9 2 append
    fi

    if [ $? -ne 0 ]; then
        echo "error"
        exit 1
    fi

    if [ $i -lt $MAX_RUNS ]; then
        sleep $INTERVAL_SEC
    fi

done

echo "Finish"
