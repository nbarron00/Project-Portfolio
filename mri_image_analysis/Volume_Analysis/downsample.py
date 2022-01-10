import numpy as np
import nibabel as nib
import torch
import emlddmm as eml
import copy
import math
import argparse

def downsample_image(image, t_axis, a_factor, dI):
    if t_axis not in [0,1,2]:
        sys.exit("Invalid anisotropic axis: must be 0, 1, or 2")
    if a_factor < 1:
        sys.exit("Invalid anisotropic factor: mist be an integer >= 1")
        
    dI_down = copy.copy(dI)
    dI_down[t_axis] = dI_down[t_axis] * a_factor
    image_down = np.take(image, range(0,np.size(image,t_axis),a_factor), axis=t_axis)
    return image_down, dI_down
    
parser = argparse.ArgumentParser(description="Argument Parser")
parser.add_argument("dfactor", help="Downsampling Factor")
parser.add_argument("axis", help="Downsampling Axis")
parser.add_argument("filename", help="MNI image filename")
parser.add_argument("out_filename", help="Output filename")

args = parser.parse_args()
down_factor = int(args.dfactor)
down_axis = int(args.axis)
filename = args.filename
output_filename = args.out_filename

xI,I,title,names = eml.read_vtk_data(filename)
I = I[0,:,:,:]

d0 = xI[0][1] - xI[0][0]
d1 = xI[1][1] - xI[1][0]
d2 = xI[2][1] - xI[2][0]
dI = [d0, d1, d2]

downsampled_image, dI_downsampled = downsample_image(I,down_axis,down_factor,dI)

nI_tuple = I.shape
nI = [nI_tuple[0], nI_tuple[1], nI_tuple[2]]
nI[down_axis] = int(math.ceil(nI[down_axis] / down_factor))
xI = [np.arange(n)*d - (n-1)*d/2 for d,n in zip(dI_downsampled,nI) ]

eml.write_vtk_data(output_filename, xI, downsampled_image[None], output_filename)
