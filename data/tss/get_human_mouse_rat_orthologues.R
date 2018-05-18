library(dplyr)
library(tidyr)
library(glue)

source('get-one2one-orthologues/src/get_one2one_orthologues.R')

args <- commandArgs(T)
OUT <- args[1]  # name of the .txt file to write

# build the list of 1:1:1 human-mouse-rat orthologues
human_rat <- get_one2one_orthologues('homo_sapiens', 'rattus_norvegicus')
human_mouse <- get_one2one_orthologues('homo_sapiens', 'mus_musculus')
mouse_rat <- get_one2one_orthologues('mus_musculus', 'rattus_norvegicus')


human_rat <- human_rat %>% 
  dplyr::select(-ensembl_gene_id) %>% 
  tidyr::spread(key = species, value = gene_name) %>% 
  dplyr::select(-homology_id) %>%
  unique()
human_mouse <- human_mouse %>% 
  dplyr::select(-ensembl_gene_id) %>% 
  tidyr::spread(key = species, value = gene_name) %>% 
  dplyr::select(-homology_id) %>%
  unique()
mouse_rat <- mouse_rat %>% 
  dplyr::select(-ensembl_gene_id) %>% 
  tidyr::spread(key = species, value = gene_name) %>% 
  dplyr::select(-homology_id) %>%
  unique()


# now deduce the three-way orthologues...
rmh <- inner_join(human_rat, human_mouse) # match rat and mouse based on human
rmh <- inner_join(rmh, mouse_rat) # require that the rat and mouse based on human is equal to rat and mouse directly compared
orthologues <- unique(rmh[order(rmh$homo_sapiens),])
print(glue('Identified {number_orthologues} 1:1:1 orthologues', number_orthologues=nrow(orthologues)))

# load in the list of housekeeping genes
HK_GENE_LIST <- 'HK_genes.txt'
human_housekeeping_genes <- read.table(HK_GENE_LIST, head = F, as.is = T)[,1]
orthologues$is_human_housekeeping <- orthologues$homo_sapiens %in% human_housekeeping_genes
write.table(orthologues, file = OUT, append = F, quote = F, sep = '\t', row.names = F, col.names = T)
