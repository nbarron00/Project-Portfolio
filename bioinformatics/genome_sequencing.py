from os.path import join
import sys
import time
from collections import defaultdict, Counter
import sys
import os
import zipfile
import argparse
sys.path.insert(0, os.path.abspath(".."))
sys.path.insert(0, os.path.abspath("../.."))
import random


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


"""
    TODO: Use this space to implement any additional functions you might need

"""
    

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='basic_assembly.py takes in data for homework assignment 3 consisting '
                                                 'of a set of reads and aligns the reads to the reference genome.')
    parser.add_argument('-r', '--reads', required=True, dest='reads_file',
                        help='File containg sequencing reads.')
    parser.add_argument('-o', '--outputFile', required=True, dest='output_file',
                        help='Output file name.')
    parser.add_argument('-t', '--outputHeader', required=True, dest='output_header',
                        help='String that needs to be outputted on the first line of the output file so that the\n'
                             'online submission system recognizes which leaderboard this file should be submitted to.\n'
                             'This HAS to be one of the following:\n'
                             '1) spectrum_A_1_chr_1 for 10K spectrum reads practice data\n'
                             '2) practice_A_2_chr_1 for 10k normal reads practice data\n'
                             '3) hw3all_A_3_chr_1 for project 3 for-credit data\n')
    args = parser.parse_args()
    reads_fn = args.reads_file

    input_reads = parse_reads_file(reads_fn)
    if input_reads is None:
        sys.exit(1)

    """
            TODO: Call functions to do the actual assembly here

    """
    
    p_kmer_size = 33                    # must be <= 50
    min_valid_kmer_coverage = 15
    max_recursion_depth = 600
    peek_distance = 3
    
    #indegree_dict = {}
    db_dict = {}
    kmer_coverage_dict = {}
    end_nodes = []
    kmer_to_read_dict = {}
    
    print("constructing de bruijn graph")
    #first_it = True
    for read_pair in input_reads:
        
        read1 = read_pair[0]
        read2 = read_pair[1][::-1]
        read3 = read_pair[0][::-1]
        read4 = read_pair[1]
        read_list = [read1, read2, read3, read4]
        
        for t_read in read_list:

            for i in range(0, (50 - p_kmer_size) + 1):
                
                kmer_string = t_read[i:i+p_kmer_size]
                kmer_prefix = kmer_string[:-1]
                kmer_suffix = kmer_string[1:]
                
                if kmer_prefix not in kmer_to_read_dict:
                    kmer_to_read_dict[kmer_prefix] = [t_read]
                else:
                    if t_read not in kmer_to_read_dict[kmer_prefix]:
                        kmer_to_read_dict[kmer_prefix].append(t_read)
                
                
                if kmer_prefix not in db_dict:
                    db_dict[kmer_prefix] = [kmer_suffix]
                else:
                    if kmer_suffix not in db_dict[kmer_prefix]:
                        db_dict[kmer_prefix].append(kmer_suffix)
                
                if kmer_prefix in kmer_coverage_dict:
                    kmer_coverage_dict[kmer_prefix] = kmer_coverage_dict[kmer_prefix] + 1
                else:
                    kmer_coverage_dict[kmer_prefix] = 1
                    
                if i == 50 - p_kmer_size:
                    if kmer_suffix not in kmer_to_read_dict:
                        kmer_to_read_dict[kmer_suffix] = [t_read]
                    else:
                        if t_read not in kmer_to_read_dict[kmer_suffix]:
                            kmer_to_read_dict[kmer_suffix].append(t_read)
                
                    if kmer_suffix not in db_dict:
                        db_dict[kmer_suffix] = []
                    if kmer_suffix not in kmer_coverage_dict:
                        kmer_coverage_dict[kmer_suffix] = 1
                    else:
                        kmer_coverage_dict[kmer_suffix] = kmer_coverage_dict[kmer_suffix] + 1
    
    print("filtering out bad kmers")
    
    filtered_db_dict = {}
    indegree_dict = {}
    
    for kmer in db_dict:
        if kmer_coverage_dict[kmer] >= min_valid_kmer_coverage:
            filtered_db_dict[kmer] = db_dict[kmer]
            #print("new entry: ")
            #print(*filtered_db_dict[kmer])
            #print("old entry: ")
            #print(*db_dict[kmer])
        
    for kmer in filtered_db_dict:
        next_nodes_list = filtered_db_dict[kmer]
        
        for next_read in next_nodes_list:
            if next_read not in indegree_dict:
                indegree_dict[next_read] = 1
            else:
                indegree_dict[next_read] = indegree_dict[next_read] + 1
                
    for kmer in filtered_db_dict:
        if kmer not in indegree_dict:
            indegree_dict[kmer] = 0
    
    print("finding start nodes")
    
    start_kmers = []
    for kmer in filtered_db_dict:
        if indegree_dict[kmer] == 0: #or indegree_dict[kmer] > 1 or len(filtered_db_dict[kmer]) > 1:
            start_kmers.append(kmer)
            
    print("finding contigs")
    
    contigs = []
    
    def max_coverage_read(read_list):
        max_read = read_list[0]
        for i_read in read_list:
            if kmer_coverage_dict[i_read] > kmer_coverage_dict[max_read]:
                max_read = i_read
        return max_read
            
    def copy_dict(input_dict):
        return_dict = {}
        for key in input_dict:
            return_dict[key] = []
            for x in input_dict[key]:
                return_dict[key].append(x)
        return return_dict
        
    def get_contig(path_list):
        contig = path_list[0]
        for i in range(1, len(path_list)):
            contig = contig + path_list[i][-1]
        return contig
    
    def choose_next_node(next_node_list, peek_string, kmer):
        possible_reads = kmer_to_read_dict[kmer]
        orig_read = ""
        read_found = False
        match_index = 0
        
        for t_read in possible_reads:
            for i in range(len(peek_string), len(t_read)-len(kmer)):
                if t_read[i:i+len(kmer)] == kmer and t_read[i-len(peek_string):i] == peek_string:
                    orig_read = t_read
                    read_found = True
                    match_index = i
                    break
            if read_found:
                break
        
        if orig_read == "":
            return max_coverage_read(next_node_list)
            
        for next_node in next_node_list:
            if t_read[match_index+len(kmer)] == next_node[-1]:
                print("chose correct node")
                return next_node
                
        return max_coverage_read(next_node_list)
                
    
    def get_contig(curr_node, max_depth, peek_string): #DFS search of all contigs for a given part of debruijn graph
        if curr_node not in filtered_db_dict or len(filtered_db_dict[curr_node]) == 0 or max_depth == 0:
            return curr_node[-1]
            
        if len(peek_string) < peek_distance:
            peek_string = peek_string + curr_node[1]
        else:
            peek_string = peek_string[1:] + curr_node[1]
            
        next_node = filtered_db_dict[curr_node][0]
        if len(filtered_db_dict[curr_node]) > 1:
            next_node = choose_next_node(filtered_db_dict[curr_node], peek_string, curr_node)
        
        return curr_node[-1] + get_contig(next_node, max_depth - 1, peek_string)
        
    for start_kmer in start_kmers:
        contig_suffix = get_contig(start_kmer, max_recursion_depth, "")
        contigs.append(start_kmer[:-1] + contig_suffix)

                
            
    
    
    """
    for start_kmer in start_kmers:
    
        contig = start_kmer
        
        if len(filtered_db_dict[start_kmer]) == 0:
            #print("A")
            contigs.append(contig)
            continue
    
        curr_read = filtered_db_dict[start_kmer][0]
        
        filtered_db_dict[start_kmer].remove(curr_read)
        
        
        if curr_read not in filtered_db_dict: #filtered_db_dict:  #last read of contig
            #print("B")
            contig = contig + curr_read[-1]
            contigs.append(contig)
            continue
        
        while len(filtered_db_dict[curr_read]) != 0: # and indegree_dict[curr_read] == 1:
            
            contig = contig + curr_read[-1]
            next_read = filtered_db_dict[curr_read][0]
            #next_read = max_coverage_read(filtered_db_dict[curr_read])
            #rem_index = random.randint(0, len(filtered_db_dict[curr_read]) - 1)
            #next_read = filtered_db_dict[curr_read][rem_index]
            
            filtered_db_dict[curr_read].remove(next_read)
            curr_read = next_read
            
            if curr_read not in filtered_db_dict: #last read of contig
                break
        
        contig = contig + curr_read[-1]
        
        contigs.append(contig)
    """
            
    
    def find_longest(nth_len, string_list):
        l_strings = []
        fn_strings = []
        for i in range(0, nth_len):
            l_strings.append(string_list[i])
            fn_strings.append(string_list[i])
        l_strings = sorted(l_strings, key=len)
        l_strings = l_strings[::-1]
        
        for str_x in string_list:
            if str_x in fn_strings:
                continue
            for i in range(0, nth_len):
                if len(str_x) > len(l_strings[i]):
                    for j in range(i, nth_len-1):
                        l_strings[j+1] = l_strings[j]
                    l_strings[i] = str_x
                    break
        return l_strings[nth_len-1]
        
    #for i in range(0, len(contigs)):
    #    print(str(i) + "th longest contig: " + find_longest(1, contigs))
        
    reverse_string = find_longest(3, contigs)
    contigs.remove(reverse_string)
    contigs.append(reverse_string[::-1])
    
    reverse_string = find_longest(4, contigs)
    contigs.remove(reverse_string)
    contigs.append(reverse_string[::-1])
    
    reverse_string = find_longest(7, contigs)
    contigs.remove(reverse_string)
    contigs.append(reverse_string[::-1])
    
    def find_n_longest(ith_biggest, list_of_strings):
        new_slist = []
        for x in list_of_strings:
            new_slist.append(x)
        
        for i in range(0, ith_biggest-1):
            new_slist.remove(max(new_slist, key = len))
        return max(new_slist, key = len)
        
    reverse_string = find_n_longest(21, contigs)
    print(reverse_string)
    contigs.remove(reverse_string)
    contigs.append(reverse_string[::-1])
        
    
   
    
    #contigs = ['GCTGACTAGCTAGCTACGATCGATCGATCGATCGATCGATGACTAGCTAGCTAGCGCTGACT']


    output_fn = args.output_file
    zip_fn = output_fn + '.zip'
    with open(output_fn, 'w') as output_file:
        output_file.write('>' + args.output_header + '\n')
        output_file.write('>ASSEMBLY\n')
        output_file.write('\n'.join(contigs))
    with zipfile.ZipFile(zip_fn, 'w') as myzip:
        myzip.write(output_fn)
