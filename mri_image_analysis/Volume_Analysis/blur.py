import numpy as np
import nibabel as nib
import torch
import emlddmm as eml
import copy
import math
import argparse

def blur_image(image, blur_rad):

    blurred_image = np.zeros_like(image)
    for i in range(-blur_rad, blur_rad+1):
        for j in range(-blur_rad, blur_rad+1):
            for k in range(-blur_rad, blur_rad+1):
                blurred_image += np.roll(image, (i,j,k), axis=(0,1,2))
    blurred_image = blurred_image / (((2*blur_rad)+1)**3)
    return blurred_image
    
parser = argparse.ArgumentParser(description="Argument Parser")
parser.add_argument("rad", help="Blur voxel radius")
parser.add_argument("filename", help="MNI image filename")
parser.add_argument("out_filename", help="Output filename")

args = parser.parse_args()
rad = int(args.rad)
filename = args.filename
output_filename = args.out_filename

xI,I,title,names = eml.read_vtk_data(filename)
I = I[0,:,:,:]

d0 = xI[0][1] - xI[0][0]
d1 = xI[1][1] - xI[1][0]
d2 = xI[2][1] - xI[2][0]
dI = [d0, d1, d2]

blurred_image = blur_image(I,rad)

nI_tuple = I.shape
nI = [nI_tuple[0], nI_tuple[1], nI_tuple[2]]
xI = [np.arange(n)*d - (n-1)*d/2 for d,n in zip(dI,nI) ]

eml.write_vtk_data(output_filename, xI, blurred_image[None], output_filename)
