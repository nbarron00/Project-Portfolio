import numpy as np
import csv
import emlddmm as eml
import argparse
import torch
import copy

def jac_matrix(Xout,dI):
    f0 = Xout[0,...]
    f1 = Xout[1,...]
    f2 = Xout[2,...]
    
    d0 = dI[0]
    d1 = dI[1]
    d2 = dI[2]
    
    f0_grad = np.gradient(f0,d0,d1,d2)
    f1_grad = np.gradient(f1,d0,d1,d2)
    f2_grad = np.gradient(f2,d0,d1,d2)
    
    grad_list = [f0_grad, f1_grad, f2_grad]
    
    row_len = len(Xout[0,:,0,0])
    col_len = len(Xout[0,0,:,0])
    slice_len = len(Xout[0,0,0,:])
    
    jacobians = np.ndarray((row_len, col_len, slice_len, 3, 3))
    
    for x in range(row_len):
        for y in range(col_len):
            for z in range(slice_len):
                for f in range(3):
                    for d_dim in range(3):
                        jacobians[x,y,z,f,d_dim] = grad_list[f][d_dim][x,y,z]
    
    jac_dets = np.linalg.det(jacobians)
    
    return jac_dets
    
def jac_volumes(I, Xout, dI):
    jac_dets = jac_matrix(Xout, dI)
    
    label_to_index = {}
    labels = np.unique(I.ravel())
    volumes = np.zeros(len(labels))
    
    for i,l in enumerate(labels):
        label_to_index[l] = i
    
    for x in range(len(I[:,0,0])):
        for y in range(len(I[0,:,:])):
            for z in range(len(I[0,0,:])):
                curr_label = I[x,y,z]
                index = label_to_index[curr_label]
                volumes[index] += (np.prod(dI) * jac_dets[x,y,z])
                
    return volumes, label_to_index

parser = argparse.ArgumentParser(description="Argument Parser")
parser.add_argument("filename", help="Input labeled .vtk file")
parser.add_argument("out_filename", help="Output filename")
parser.add_argument("atlas", help="Atlas filename")
parser.add_argument("tr_dir", help="Directory containing transform data")

args = parser.parse_args()
filename = args.filename
out_filename = args.out_filename
atlas = args.atlas
transform_directory = args.tr_dir

xI,I,title,names = eml.read_vtk_data(filename)
I = I[0,:,:,:]

d0 = xI[0][1] - xI[0][0]
d1 = xI[1][1] - xI[1][0]
d2 = xI[2][1] - xI[2][0]
dI = [float(d0), float(d1), float(d2)]

labels = np.unique(I.ravel())
volumes = np.zeros(len(labels))
label_to_index = {}

for i,l in enumerate(labels):
    label_to_index[l] = i
    
for l in labels:
    volumes[label_to_index[l]] = np.sum(I == l) * np.prod(dI)

#volumes2 = np.zeros(len(labels))

xI_atlas,I_atlas,title_atlas,names_atlas = eml.read_vtk_data(atlas)
I_atlas = I_atlas[0,:,:,:]
Xin = torch.stack(torch.meshgrid([torch.as_tensor(x) for x in xI_atlas]))
Xout = eml.compose_sequence(transform_directory, Xin)

d0 = Xin[0,1,0,0] - Xin[0,0,0,0]
d1 = Xin[1,0,1,0] - Xin[1,0,0,0]
d2 = Xin[2,0,0,1] - Xin[2,0,0,0]
dI = [float(d0), float(d1), float(d2)]

jac_vols, jac_vols_dict = jac_volumes(I_atlas, Xout, dI)

all_labels = copy.deepcopy(labels).tolist()

for x in jac_vols_dict:
    if x not in all_labels:
        all_labels.append(x)

all_labels = np.sort(all_labels)

with open(out_filename, 'w') as csvfile:
    writer = csv.writer(csvfile)
    writer.writerow(["Label", "Volume_V", "Volume_J"])
    for l in all_labels:
        vox_volume = 0
        if l in label_to_index:
            vox_volume = volumes[label_to_index[l]]
        
        jac_volume = 0
        if l in jac_vols_dict:
            jac_volume = jac_vols[jac_vols_dict[l]]
    
        writer.writerow([l, vox_volume, jac_volume])
