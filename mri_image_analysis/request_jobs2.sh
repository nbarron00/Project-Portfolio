#! /bin/bash

mkdir exp_outputs
for ATLAS_NUM in 01 02 03 04 05 06 07 08 09
do
    for TARGET_NUM in 01 02 03 04 05 06 07 08 09 10 11 12 13 14 15 16 17 18
    do
        if [ "$ATLAS_NUM" -ne "$TARGET_NUM" ];
        then
        qsub -l h_data=20G,h_rt=22:00:00 ./exp2.sh "exp_outputs/output_a${ATLAS_NUM}_t${TARGET_NUM}" $ATLAS_NUM $TARGET_NUM
        fi
    done
done
