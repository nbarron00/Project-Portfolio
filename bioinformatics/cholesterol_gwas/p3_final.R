#part 3: Manhattan Plot

plink_linreg_data = read.table("plink.assoc.linear",
				header = TRUE)
dframe = data.frame("CHR" = plink_linreg_data["CHR"], 
			"BP" = plink_linreg_data["BP"],
			"P" = plink_linreg_data["P"])
manhattan(dframe)