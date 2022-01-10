#part 4: find snp with min. p-val for each chromosome

snp_data_plink = read.table("plink.assoc.linear",
		header = TRUE)

snp_names = c(1:22)
min_p_vals = c(1:22)
snp_pos = c(1:22)

for(i in c(1:22)){
	chr_i_vector = NULL
	chr_i_vector = snp_data_plink[(snp_data_plink$"CHR" == i),]
	row = which.min(chr_i_vector[,9])

	min_p_vals[i] = chr_i_vector[row, 9]
	snp_names[i] = chr_i_vector[row, 2]
	snp_pos[i] = chr_i_vector[row, 3]
}

cat("SNPs with min. p-val for each chromosome:\n\n")
for (i in c(1:22)){
	cat("CHR", i, " name: ", snp_names[i], " p-val: ", min_p_vals[i], "pos: ", snp_pos[i], "\n")
}

UKBB_match_p_vals = vector()
UKBB_match_chr_nums = vector()
matches_found = 0

#i = 0L
skip_lines = 0
repeat{
	#print(skip_lines + 1)
	#print(matches_found)

	line = fread(file = "30760_irnt.gwas.imputed_v3.both_sexes.tsv",
	nrows = 1, skip = skip_lines)
	
	if(length(line) == 0) { break } #reached EOF	
	if(matches_found == 22) { break } #all SNPs found
	
	variant_name = as.list(strsplit(line[[1]], "\t"))[[1]]

	chr_num = as.list(strsplit(variant_name[1], ":")[[1]])[[1]]
	snp_basepos = as.list(strsplit(variant_name[1], ":")[[1]])[[2]]

	chr_num = strtoi(chr_num)
	snp_basepos = strtoi(snp_basepos)	
	#cat(chr_num, " ", snp_basepos, " ", snp_pos[chr_num], "\n")

	if (snp_pos[chr_num] == snp_basepos) # match found
	{
		p_val_match = line[[11]]
		matches_found = matches_found + 1

		UKBB_match_p_vals = c(UKBB_match_p_vals, p_val_match)
		UKBB_match_chr_nums = vector(UKBB_match_chr_nums, chr_num)
		print("match found")
	}
	
	skip_lines = skip_lines + 1
}

UKBB_p_vals_ordered = c(1:22)

for (j in c(1:22)){
	UKBB_p_vals_ordered[j] = UKBB_match_p_vals[which(UKBB_match_chr_nums == j)]
}

print(UKK_p_vals_ordered)

table_data = c(snp_names, min_p_vals, UKBB_p_vals_ordered)
table_m = matrix(table_data, ncol=22, byrow=TRUE)
rownames(table_m) = c("rsID", "PLINK p-val", "UKBB p-val")
colnames(table_m) = c("Chr. 1", "Chr. 2", "Chr. 3", "Chr. 4", "Chr. 5", "Chr. 6",
			"Chr. 7", "Chr. 8", "Chr. 9", "Chr. 10", "Chr. 11", "Chr. 12",
			"Chr. 13", "Chr. 14", "Chr. 15", "Chr. 16", "Chr. 17", "Chr. 18",
			"Chr. 19", "Chr. 20", "Chr. 21", "Chr. 22")
table_m <- as.table(table_m)
print(table_m)
