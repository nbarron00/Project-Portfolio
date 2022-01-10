import sys
import argparse
import time
import zipfile


def parse_reads_file(reads_fn):
    """
    :param reads_fn: the file containing all of the reads
    :return: outputs a list of all paired-end reads
    """
    try:
        with open(reads_fn, 'r') as rFile:
            print("Parsing Reads")
            first_line = True
            count = 0
            all_reads = []
            for line in rFile:
                count += 1
                if count % 1000 == 0:
                    print(count, " reads done")
                if first_line:
                    first_line = False
                    continue
                ends = line.strip().split(',')
                all_reads.append(ends)
        return all_reads
    except IOError:
        print("Could not read file: ", reads_fn)
        return None


def parse_ref_file(ref_fn):
    """
    :param ref_fn: the file containing the reference genome
    :return: a string containing the reference genome
    """
    try:
        with open(ref_fn, 'r') as gFile:
            print("Parsing Ref")
            first_line = True
            ref_genome = ''
            for line in gFile:
                if first_line:
                    first_line = False
                    continue
                ref_genome += line.strip()
        return ref_genome
    except IOError:
        print("Could not read file: ", ref_fn)
        return None


"""
    TODO: Use this space to implement any additional functions you might need

"""

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='basic_aligner.py takes in data for homework assignment 1 consisting '
                                     'of a genome and a set of reads and aligns the reads to the reference genome, '
                                     'then calls SNPs based on this alignment')
    parser.add_argument('-g', '--referenceGenome', required=True, dest='reference_file',
                        help='File containing a reference genome.')
    parser.add_argument('-r', '--reads', required=True, dest='reads_file',
                        help='File containg sequencing reads.')
    parser.add_argument('-o', '--outputFile', required=True, dest='output_file',
                        help='Output file name.')
    parser.add_argument('-t', '--outputHeader', required=True, dest='output_header',
                        help='String that needs to be outputted on the first line of the output file so that the '
                             'online submission system recognizes which leaderboard this file should be submitted to.'
                             'This HAS to be practice_W_1_chr_1 for the practice data and hw1_W_2_chr_1 for the '
                             'for-credit assignment!')
    args = parser.parse_args()
    reference_fn = args.reference_file
    reads_fn = args.reads_file

    input_reads = parse_reads_file(reads_fn)
    if input_reads is None:
        sys.exit(1)
    reference = parse_ref_file(reference_fn)
    if reference is None:
        sys.exit(1)

    """
        TODO: Call functions to do the actual read alignment here

    
        
    """
    def mismatch_count(string1, string2):
        if len(string1) != len(string2):
            return -1
        mismatches = 0
        for i in range(0, len(string1)):
            if string1[i] != string2[i]:
                mismatches = mismatches + 1
        return mismatches
    
    def most_common_in_list(input_list):
        if len(input_list) == 0:
            return 'E'
        top_count = 0
        most_common_item = input_list[0]
        for item in input_list:
            item_freq = input_list.count(item)
            if (item_freq > top_count):
                top_count = item_freq
                most_common_item = item
        return most_common_item
    
    def find_insertion(ref_string, read_string):
        index = 0
        #if ref_string[0] != read_string[0]:
        #    return [-1, -1]
         
        for i in range(0, 10):
            if ref_string[i] != read_string[i]:
                return [-1, -1]
         
        for i in range(10, len(read_string) - 10):        #set index to first index of a mismatching base
            if ref_string[i] != read_string[i]:
                index = i
                break
            if i == len(read_string) - 2:
                return [-1, -1]
        
        for ins_len in range(1, 5):                 #find valid insertion length between 1 and 12
            if ins_len + index >= len(read_string):
                break
                
            valid_insertion = True
            for i in range(0, len(read_string) - ins_len - index):
                if read_string[index + ins_len + i] != ref_string[index + i]:
                        valid_insertion = False
                        break
            if valid_insertion:
                #print("ins - reference: " + ref_string + ", read: " + read_string + ", index: " + str(index) + ", length: " + str(ins_len))
                return [index, ins_len]
        return [-1, -1]
    
    def find_deletion(ref_string, read_string):
        index = 0
        #if ref_string[0] != read_string[0]:     #deletion cannot start at beginning of read
        #    return [-1, -1]
         
        for i in range(0, 10):
            if ref_string[i] != read_string[i]:
                return [-1, -1]
         
        for i in range(10, len(read_string) - 10):        #set index to first index of a mismatching base
            if ref_string[i] != read_string[i]:
                index = i
                break
            if i == len(read_string) - 2:
                return [-1, -1]
        
        for del_len in range(1, 5):                 #find valid deletion length between 1 and 12
            valid_deletion = True
            for j in range(index, len(read_string)):
                if read_string[j] != ref_string[j + del_len]:
                        valid_deletion = False
                        break
            if valid_deletion: #
                #print("del - reference: " + ref_string + ", read: " + read_string + ", index: " + str(index) + ", length: " + str(del_len))
                return [index, del_len]
        return [-1, -1]
    
    snps = []
    insertions = []
    deletions = []
    genome_dict = {}
    reconstructed_genome_alleles = {}
    
    for index in range(0, len(reference) - 50):
        substring = reference[index:index + 50]
        if substring in genome_dict:
            genome_dict[substring].append(index)
        else:
            genome_dict[substring] = [index]

    for paired_read in input_reads:

        read_forward = [paired_read[0], paired_read[1][::-1]]
        read_backward = [paired_read[0][::-1], paired_read[1]]
        min_pair_mismatch_count = 50
        best_match_position = -1
        best_pair_match_position = -1
        best_pair_read = ""
        best_read = ""

        #check forward direction for exact matches
        
        if read_forward[0] in genome_dict: #if first part an exact match, look for min mismatches for second part

            match_index_list = genome_dict[read_forward[0]]
            for match_position in match_index_list:
                for gap_distance in range(90,110 + 1):
                    pair_index = match_position + gap_distance + 50

                    if pair_index + 50 > len(reference):
                        break
                    
                    curr_pair_mismatches = mismatch_count(read_forward[1], reference[pair_index:pair_index + 50])  
                    if curr_pair_mismatches < min_pair_mismatch_count:
                        min_pair_mismatch_count = curr_pair_mismatches
                        best_match_position = match_position
                        best_pair_match_position = pair_index
                        best_pair_read = read_forward[1]
                        best_read = read_forward[0]
        
        
        if read_forward[0] in genome_dict:      #search for indels
            insertion_found = False
            deletion_found = False
            match_index_list = genome_dict[read_forward[0]]
            for match_position in match_index_list:
                for gap_distance in range(90,110+1):
                    ref_index = match_position + 50 + gap_distance
                    
                    if ref_index + 62 >= len(reference):             #will not find indels in last 57 bases of genome
                        break
                        
                    if not insertion_found:
                        insertion_info = find_insertion(reference[ref_index:ref_index + 50], read_forward[1])
                    if not deletion_found:
                        deletion_info = find_deletion(reference[ref_index:ref_index + 62], read_forward[1])
                    
                    if not insertion_found and insertion_info[0] != -1:
                        ins_read_index = insertion_info[0]
                        ins_length = insertion_info[1]
                        ins_string = (read_forward[1])[ins_read_index:ins_read_index + ins_length]
                        pos_in_genome = ref_index + ins_read_index
                        insertions.append([ins_string, pos_in_genome])
                        insertion_found = True
                        
                        #print("ins len : " + str(ins_length) + " ref: " + str(reference[ref_index:ref_index + 50]) + " read: " + str(read_forward[1]) + " pos: " + str(ins_read_index) + " g_pos: " + str(pos_in_genome) + " ref_index: " + str(ref_index) + " string: " + ins_string)
                        
                    if not deletion_found and deletion_info[0] != -1:
                        del_read_index = deletion_info[0]
                        del_length = deletion_info[1]
                        pos_in_genome = ref_index + del_read_index
                        del_string = reference[pos_in_genome:pos_in_genome + del_length]
                        deletions.append([del_string, pos_in_genome])
                        deletion_found = True
                        
                        #print("del len : " + str(del_length) + " ref: " + str(reference[ref_index:ref_index + 62]) + " read: " + str(read_forward[1]) + " pos: " + str(del_read_index) + " g_pos: " + str(pos_in_genome) + " ref_index: " + str(ref_index) + " string: " + del_string)
                
                if insertion_found or deletion_found:
                    break
        
        if read_forward[1] in genome_dict: #if second part and exact match, look for min mismatches in first part

            match_index_list = genome_dict[read_forward[1]]
            for match_position in match_index_list:
                for gap_distance in range(90,110 + 1):
                    pair_index = match_position - gap_distance - 50

                    if pair_index - 50 < 0:
                        break
                    
                    curr_pair_mismatches = mismatch_count(read_forward[0], reference[pair_index:pair_index + 50])  
                    if curr_pair_mismatches < min_pair_mismatch_count:
                        min_pair_mismatch_count = curr_pair_mismatches
                        best_match_position = match_position
                        best_pair_match_position = pair_index
                        best_pair_read = read_forward[0]
                        best_read = read_forward[1]
                        
        #check backward direction for exact matches
        

        if read_forward[1] in genome_dict:      #search for indels
            insertion_found = False
            deletion_found = False
            match_index_list = genome_dict[read_forward[1]]
            for match_position in match_index_list:
                for gap_distance in range(90,110+1):
                    ref_index = match_position - 50 - gap_distance
                    
                    if ref_index < 0:             #will not find indels in last 57 bases of genome
                        break
                        
                    if not insertion_found:
                        insertion_info = find_insertion(reference[ref_index:ref_index + 50], read_forward[0])
                    if not deletion_found:
                        deletion_info = find_deletion(reference[ref_index:ref_index + 62], read_forward[0])
                    
                    if not insertion_found and insertion_info[0] != -1:
                        ins_read_index = insertion_info[0]
                        ins_length = insertion_info[1]
                        ins_string = (read_forward[1])[ins_read_index:ins_read_index + ins_length]
                        pos_in_genome = ref_index + ins_read_index
                        insertions.append([ins_string, pos_in_genome])
                        insertion_found = True
                        
                    if not deletion_found and deletion_info[0] != -1:
                        del_read_index = deletion_info[0]
                        del_length = deletion_info[1]
                        pos_in_genome = ref_index + del_read_index
                        del_string = reference[pos_in_genome:pos_in_genome + del_length]
                        deletions.append([del_string, pos_in_genome])
                        deletion_found = True
                
                if insertion_found or deletion_found:
                    break
        
        if read_backward[0] in genome_dict: #if first part an exact match, look for min mismatches for second part

            match_index_list = genome_dict[read_backward[0]]
            for match_position in match_index_list:
                for gap_distance in range(90,110 + 1):
                    pair_index = match_position + gap_distance + 50

                    if pair_index + 50 > len(reference):
                        break
                    
                    curr_pair_mismatches = mismatch_count(read_backward[1], reference[pair_index:pair_index + 50])  
                    if curr_pair_mismatches < min_pair_mismatch_count:
                        min_pair_mismatch_count = curr_pair_mismatches
                        best_match_position = match_position
                        best_pair_match_position = pair_index
                        best_pair_read = read_backward[1]
                        best_read = read_backward[0]
                        
                
        if read_backward[0] in genome_dict:      #search for indels
            insertion_found = False
            deletion_found = False
            match_index_list = genome_dict[read_backward[0]]
            for match_position in match_index_list:
                for gap_distance in range(90,110+1):
                    ref_index = match_position + 50 + gap_distance
                    
                    if ref_index + 62 >= len(reference):             #will not find indels in last 57 bases of genome
                        break
                        
                    if not insertion_found:
                        insertion_info = find_insertion(reference[ref_index:ref_index + 50], read_backward[1])
                    if not deletion_found:
                        deletion_info = find_deletion(reference[ref_index:ref_index + 62], read_backward[1])
                    
                    if not insertion_found and insertion_info[0] != -1:
                        ins_read_index = insertion_info[0]
                        ins_length = insertion_info[1]
                        ins_string = (read_forward[1])[ins_read_index:ins_read_index + ins_length]
                        pos_in_genome = ref_index + ins_read_index
                        insertions.append([ins_string, pos_in_genome])
                        insertion_found = True
                        
                    if not deletion_found and deletion_info[0] != -1:
                        del_read_index = deletion_info[0]
                        del_length = deletion_info[1]
                        pos_in_genome = ref_index + del_read_index
                        del_string = reference[pos_in_genome:pos_in_genome + del_length]
                        deletions.append([del_string, pos_in_genome])
                        deletion_found = True
                
                if insertion_found or deletion_found:
                    break
        
                        
        if read_backward[1] in genome_dict: #if second part and exact match, look for min mismatches in first part

            match_index_list = genome_dict[read_backward[1]]
            for match_position in match_index_list:
                for gap_distance in range(90,110 + 1):
                    pair_index = match_position - gap_distance - 50

                    if pair_index - 50 < 0:
                        break
                    
                    curr_pair_mismatches = mismatch_count(read_backward[0], reference[pair_index:pair_index + 50])  
                    if curr_pair_mismatches < min_pair_mismatch_count:
                        min_pair_mismatch_count = curr_pair_mismatches
                        best_match_position = match_position
                        best_pair_match_position = pair_index
                        best_pair_read = read_backward[0]
                        best_read = read_backward[1]
                        
                
        if read_backward[1] in genome_dict:      #search for indels
            insertion_found = False
            deletion_found = False
            match_index_list = genome_dict[read_backward[1]]
            for match_position in match_index_list:
                for gap_distance in range(90,110+1):
                    ref_index = match_position - 50 - gap_distance
                    
                    if ref_index < 0:             #will not find indels in last 57 bases of genome
                        break
                        
                    if not insertion_found:
                        insertion_info = find_insertion(reference[ref_index:ref_index + 50], read_backward[0])
                    if not deletion_found:
                        deletion_info = find_deletion(reference[ref_index:ref_index + 62], read_backward[0])
                    
                    if not insertion_found and insertion_info[0] != -1:
                        ins_read_index = insertion_info[0]
                        ins_length = insertion_info[1]
                        ins_string = (read_forward[1])[ins_read_index:ins_read_index + ins_length]
                        pos_in_genome = ref_index + ins_read_index
                        insertions.append([ins_string, pos_in_genome])
                        insertion_found = True
                        
                    if not deletion_found and deletion_info[0] != -1:
                        del_read_index = deletion_info[0]
                        del_length = deletion_info[1]
                        pos_in_genome = ref_index + del_read_index
                        del_string = reference[pos_in_genome:pos_in_genome + del_length]
                        deletions.append([del_string, pos_in_genome])
                        deletion_found = True
                
                if insertion_found or deletion_found:
                    break
        
                        
        if min_pair_mismatch_count in range(1,2) and best_pair_match_position > 0:
            for index in range(0, 50):
                match_index = best_match_position + index
                pair_index = best_pair_match_position + index
                if match_index in reconstructed_genome_alleles:
                    reconstructed_genome_alleles[match_index].append(best_read[index])
                else:
                    reconstructed_genome_alleles[match_index] = [best_read[index]]

                if pair_index in reconstructed_genome_alleles:
                    reconstructed_genome_alleles[pair_index].append(best_pair_read[index])
                else:
                    reconstructed_genome_alleles[pair_index] = [best_pair_read[index]]

    for genome_position in reconstructed_genome_alleles:
        recon_allele = most_common_in_list(reconstructed_genome_alleles[genome_position])
        if recon_allele != reference[genome_position]:
            snps.append([reference[genome_position], recon_allele, genome_position])
            
    del_dict = {}                   #remove duplicates
    for del_list in deletions:
        d_str = del_list[0]
        d_pos = del_list[1]
        if d_pos not in del_dict:
            del_dict[d_pos] = [d_str]
        else:
            del_dict[d_pos].append(d_str)
    
    ins_dict = {}
    for ins_list in insertions:
        i_str = ins_list[0]
        i_pos = ins_list[1]
        if i_pos not in ins_dict:
            ins_dict[i_pos] = [i_str]
        else:
            ins_dict[i_pos].append(i_str)
            
    deletions = []
    insertions = []
    
    for del_index in del_dict:
        del_str = most_common_in_list(del_dict[del_index])
        deletions.append([del_str, del_index])
    
    for ins_index in ins_dict:
        ins_str = most_common_in_list(ins_dict[ins_index])
        insertions.append([ins_str, ins_index])
        
    #snps = [['A', 'G', 3425]]

    output_fn = args.output_file
    zip_fn = output_fn + '.zip'
    with open(output_fn, 'w') as output_file:
        output_file.write('>' + args.output_header + '\n>SNP\n')
        for x in snps:
            output_file.write(','.join([str(u) for u in x]) + '\n')
        output_file.write('>INS\n')
        for x in insertions:
            output_file.write(','.join([str(u) for u in x]) + '\n')
        output_file.write('>DEL\n')
        for x in deletions:
            output_file.write(','.join([str(u) for u in x]) + '\n')
    with zipfile.ZipFile(zip_fn, 'w') as myzip:
        myzip.write(output_fn)

