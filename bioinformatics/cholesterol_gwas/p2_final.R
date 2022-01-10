#part 2: comparing plink and R snp p-values

install.packages("data.table")
library("data.table")

pheno_dataset = read.table("snpdata_12.tfam",
		header = FALSE)
pheno_data = pheno_dataset[,6]

compute_pval = function(alleles_t){
	reg_sum = summary(lm(pheno_data ~ alleles_t))$fstatistic
	p_val_r = pf(reg_sum[1], reg_sum[2], reg_sum[3], lower.tail = F)
	attributes(p_val_r) = NULL
	#p_values_R[row] = p_val_r
	return(p_val_r)
}

encode_genotype = function(genotype_t){   	
	alleles_t = c(1:2504)
	for (j in c(1:2504)){
		encoded_allele = -1
		allele_1 = genotype_t[[3+(2*j)]]
		allele_2 = genotype_t[[4+(2*j)]]

		if (allele_1 == allele_2){ 
			if (allele_1 == 2) { encoded_allele = 2 }
			else { encoded_allele = 0 }
		} else {
			encoded_allele = 1
		}	
		alleles_t[j] = encoded_allele
	}
	return(alleles_t)
}

p_vals_plink_dataset = read.table("plink.assoc.linear",
			header = TRUE)
p_values_plink = p_vals_plink_dataset[, 9]

#p_values_R = c(1:828325)
p_values_R = numeric(0)

for(row in (c(1:(828325/425) - 1)*425)){
	snp_data_buffer = fread(file = "snpdata_12.tped",
		nrows = 425, skip = row)
 
	alleles = apply(snp_data_buffer, 1, encode_genotype)

	p_values_R_vector = apply(alleles, 2, compute_pval)
	
	p_values_R = c(p_values_R, p_values_R_vector)
	print(row)
}

p_val_diff = c(1:828325)
for (k in c(1:828325)){
	p_val_diff[k] = p_values_R[k] - p_values_plink[[k]]
}

plot(p_val_diff, main = "plink vs. R allele p-values", xlab = "snp", ylab = "difference in p-value")


