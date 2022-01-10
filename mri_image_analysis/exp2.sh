#! /bin/bash

. /u/local/Modules/default/init/modules.sh
module load python/anaconda3
. $CONDA_DIR/etc/profile.d/conda.sh
#source $CONDA_DIR/etc/profile.d/conda.csh 
conda activate pytorch-1.5.0-cpu

pip install --user nibabel
pip install --user matplotlib
pip install --user pynrrd

OUTPUT=${1:-`pwd`}
ATLAS_NUM=$2
TARGET_NUM=$3

./Volume_Analysis/imreg.sh $OUTPUT $ATLAS_NUM $TARGET_NUM
