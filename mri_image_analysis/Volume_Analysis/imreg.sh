#!/bin/sh

BASEDIR=/u/home/n/nbarron/Volume_Analysis
#BASEDIR=`pwd`
OUTPUTDIR=${1:-${BASEDIR}}
ATLAS_NUM=$2
TARGET_NUM=$3


ATLAS=${BASEDIR}/input_images/Adt27-55_${ATLAS_NUM}_Adt27-55_${ATLAS_NUM}_MNI.vtk
LABEL=${BASEDIR}/input_labels/Adt27-55_${ATLAS_NUM}_Adt27-55_${ATLAS_NUM}_FullLabels.vtk
CONFIG=${BASEDIR}/config.json
MODE=register
TARGET=${BASEDIR}/input_images/Adt27-55_${TARGET_NUM}_Adt27-55_${TARGET_NUM}_MNI.vtk

mkdir ${OUTPUTDIR}
mkdir ${OUTPUTDIR}/segmentations
mkdir ${OUTPUTDIR}/volume_data
mkdir ${OUTPUTDIR}/all_data

# Image registration and analysis on all non-downsampled deformation combinations

for BLUR_RAD in 0 1
do
    for NOISE_STD in 0 20
    do
        echo "Processing with no downsampling, blurring with voxel blur radius ${BLUR_RAD}, and adding gaussian noise with mean 0 and standard deviation ${NOISE_STD}"
                
        mkdir ${OUTPUTDIR}/temp
        if [ "$BLUR_RAD" -ne 0 ];
        then
            python3 ${BASEDIR}/blur.py $BLUR_RAD $TARGET ${OUTPUTDIR}/temp/temp1.vtk
        else
            cp $TARGET ${OUTPUTDIR}/temp/temp1.vtk
        fi
        
        echo "Blurring Complete"
        
        if [ "$NOISE_STD" -ne 0 ];
        then
            python3 ${BASEDIR}/noisy.py 0 $NOISE_STD ${OUTPUTDIR}/temp/temp1.vtk ${OUTPUTDIR}/temp/temp2.vtk
        else
            cp ${OUTPUTDIR}/temp/temp1.vtk ${OUTPUTDIR}/temp/temp2.vtk
        fi
        
        echo "Done adding noise"
        
        SEG_NAME=df1_axis0_br${BLUR_RAD}_nstd${NOISE_STD}
        OUT=${OUTPUTDIR}/all_data/${SEG_NAME}
        python3 ${BASEDIR}/emlddmm.py -m $MODE -c $CONFIG -o $OUT -a $ATLAS -l $LABEL -t ${OUTPUTDIR}/temp/temp2.vtk

        cp ${OUT}/to_target/atlas_seg_to_target.vtk ${OUTPUTDIR}/segmentations/${SEG_NAME}.vtk

        rm -r ${OUTPUTDIR}/temp
        
        echo "Done with image registration"
          
        python3 ${BASEDIR}/volume_analysis.py ${OUTPUTDIR}/segmentations/${SEG_NAME}.vtk ${OUTPUTDIR}/volume_data/${SEG_NAME}.csv $LABEL $OUT
        
        rm -r ${OUT}/to_atlas
        rm -r ${OUT}/to_target
        rm ${OUTPUTDIR}/segmentations/${SEG_NAME}.vtk
        
        echo "Done retrieving volumetric data from segmented images"
    done
done

# Image registration and analysis on all downsampled deformation combinations:

for DOWN_FACTOR in 2 4
do
    for AXIS in 0 1 2
    do
        for BLUR_RAD in 0 1
        do
            for NOISE_STD in 0 20
            do
                echo "Processing with downsampling factor ${DOWN_FACTOR} along axis ${AXIS}, blurring with voxel blur radius ${BLUR_RAD}, and adding gaussian noise with mean 0 and standard deviation ${NOISE_STD}"
                
                mkdir ${OUTPUTDIR}/temp
                if [ "$BLUR_RAD" -ne 0 ];
                then
                    python3 ${BASEDIR}/blur.py $BLUR_RAD $TARGET ${OUTPUTDIR}/temp/temp1.vtk
                else
                    cp $TARGET ${OUTPUTDIR}/temp/temp1.vtk
                fi
                
                echo "Blurring Complete"
                
                python ${BASEDIR}/downsample.py $DOWN_FACTOR $AXIS ${OUTPUTDIR}/temp/temp1.vtk ${OUTPUTDIR}/temp/temp2.vtk
                
                echo "Downsampling Complete"
                
                #rm ${BASEDIR}/temp/temp1.vtk
                if [ "$NOISE_STD" -ne 0 ];
                then
                    python3 ${BASEDIR}/noisy.py 0 $NOISE_STD ${OUTPUTDIR}/temp/temp2.vtk ${OUTPUTDIR}/temp/temp3.vtk
                else
                    cp ${OUTPUTDIR}/temp/temp2.vtk ${OUTPUTDIR}/temp/temp3.vtk
                fi
                #rm ${BASEDIR}/temp/temp2.vtk
                
                echo "Done adding Noise"
                
                SEG_NAME=df${DOWN_FACTOR}_axis${AXIS}_br${BLUR_RAD}_nstd${NOISE_STD}
                OUT=${OUTPUTDIR}/all_data/${SEG_NAME}
                python3 ${BASEDIR}/emlddmm.py -m $MODE -c $CONFIG -o $OUT -a $ATLAS -l $LABEL -t ${OUTPUTDIR}/temp/temp3.vtk
                cp ${OUT}/to_target/atlas_seg_to_target.vtk ${OUTPUTDIR}/segmentations/${SEG_NAME}.vtk
                rm -r ${OUTPUTDIR}/temp
                
                echo "Done with image registration"
                
                python3 ${BASEDIR}/volume_analysis.py ${OUTPUTDIR}/segmentations/${SEG_NAME}.vtk ${OUTPUTDIR}/volume_data/${SEG_NAME}.csv $LABEL $OUT
                
                echo "Done retrieving volumetric data from segmented image"
                
		rm -r ${OUT}/to_atlas
		rm -r ${OUT}/to_target
		rm ${OUTPUTDIR}/segmentations/${SEG_NAME}.vtk
            done
         done
    done
done

rm -r ${OUTPUTDIR}/segmentations
