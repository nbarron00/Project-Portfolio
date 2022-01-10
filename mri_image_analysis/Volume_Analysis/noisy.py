import numpy as np
import nibabel as nib
import torch
import emlddmm as eml
import copy
import math
import argparse

def add_noise(image, mean, std):
    return image + np.random.normal(mean,std,image.shape)
    
parser = argparse.ArgumentParser(description="Argument Parser")
parser.add_argument("mean", help="Mean")
parser.add_argument("std", help="Standard Deviation")
parser.add_argument("filename", help="MNI image filename")
parser.add_argument("out_filename", help="Output filename")

args = parser.parse_args()
mean = int(args.mean)
std = int(args.std)
filename = args.filename
output_filename = args.out_filename

xI,I,title,names = eml.read_vtk_data(filename)
I = I[0,:,:,:]

d0 = xI[0][1] - xI[0][0]
d1 = xI[1][1] - xI[1][0]
d2 = xI[2][1] - xI[2][0]
dI = [d0, d1, d2]

noisy_image = add_noise(I,mean,std)

nI_tuple = I.shape
nI = [nI_tuple[0], nI_tuple[1], nI_tuple[2]]
xI = [np.arange(n)*d - (n-1)*d/2 for d,n in zip(dI,nI) ]

eml.write_vtk_data(output_filename, xI, noisy_image[None], output_filename)
