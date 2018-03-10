suppressPackageStartupMessages(library("optparse"))

option_list <- list(
  make_option(c("--ucsc-refgene"), dest = 'ucsc_refgene', action = 'store', type = 'character', help = '[Required] Path to the UCSC refGene file.'),
  make_option(c("--protein-coding"), dest = 'protein_coding', action = 'store_true', default = F, help = '[Optional] Include only protein-coding genes.'),
  make_option(c("--single-tss"), dest = 'single_tss', action = 'store_true', default = F, help = '[Optional] Include only genes that have a single TSS.'),
  make_option(c("--include-genes"), dest = 'include_genes', action = 'store', help = '[Optional] Include only genes that are listed (one per line) in this file.'),
  make_option(c("--out"), action = 'store', type = 'character', help = '[Required] Name of the output.')
)

option_parser <- OptionParser(usage = "usage: Rscript %prog [options]", option_list = option_list, add_help_option = T, 
                              description = 'Given the refGene table from UCSC, create a TSS BED file.')
opts <- parse_args(option_parser)

suppressPackageStartupMessages(library('dplyr'))

transcripts <- read.table(opts$ucsc_refgene, head = T, as.is = T, sep = '\t', ) %>%
  dplyr::select(name, chrom, txStart, txEnd, strand, name2, score) %>%
  dplyr::rename(transcript = name, gene = name2)

# get TSS
# if on - strand, start and end must be flipped
tss <- transcripts %>% 
  dplyr::mutate(tss_start = ifelse(strand == '+', txStart, txEnd - 1),
                tss_end = tss_start + 1) %>%
  dplyr::select(-txStart, -txEnd) %>%
  unique()


# remove genes with more than one TSS, if desired
if (opts$single_tss) {
  unique_tss <- tss %>%
    dplyr::select(gene, chrom, strand, tss_start) %>% 
    unique()
  number_tss_per_gene <- table(unique_tss$gene)
  genes_with_one_tss <- names(number_tss_per_gene)[number_tss_per_gene == 1]
  tss <- tss[tss$gene %in% genes_with_one_tss,]
}


# filter down to protein-coding genes, if desired
if (opts$protein_coding) {
  tss <- tss[grep('^NM_', tss$transcript),]
}


# filter down to the desired genes, if list passed
if (!is.null(opts$include_genes)) {
  genes_to_include <- read.table(opts$include_genes, head = F, as.is = T)
  
  if (ncol(genes_to_include) > 1) {
    stop('Only one column expected in the --include-genes file.')
  }
  
  tss <- tss[toupper(tss$gene) %in% toupper(genes_to_include$V1),]
}

tss <- unique(tss[,c('chrom', 'tss_start', 'tss_end')])
tss <- tss[order(tss$chrom, tss$tss_start),]

write.table(x = tss, file = opts$out, quote = F, append = F, sep = "\t", row.names = F, col.names = F)
