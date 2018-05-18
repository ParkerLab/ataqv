# TSS files

TSS files for hg19:

1. `hg19.tss.refseq.bed.gz` is all RefSeq TSS for human protein-coding genes
2. `hg19.tss.refseq.housekeeping.all.bed.gz` is all RefSeq TSS for human protein-coding housekeeping genes.
3. `hg19.tss.refseq.housekeeping.ortho.bed.gz` is all RefSeq TSS for human protein-coding housekeeping genes that have 1:1:1 orthologues for human : mouse : rat.


TSS files for mm9:

1. `mm9.tss.refseq.bed.gz` is all RefSeq TSS for mouse protein-coding genes
2. `mm9.tss.refseq.housekeeping.ortho.bed.gz` is all RefSeq TSS for mouse protein-coding genes that have 1:1:1 orthologues for human : mouse : rat, where the human orthologue is a housekeeping gene.

TSS files for rn5:

1. `rn5.tss.refseq.bed.gz` is all RefSeq TSS for rat protein-coding genes
2. `rn5.tss.refseq.housekeeping.ortho.bed.gz` is all RefSeq TSS for rat protein-coding genes that have 1:1:1 orthologues for human : mouse : rat, where the human orthologue is a housekeeping gene.

These were generated using the following commands:

```bash
# fetch the human RefSeq TSS list from UCSC
mysql --host=genome-mysql.cse.ucsc.edu --user=genome -D hg19 -e "SELECT * FROM refGene" > hg19.tss.refseq.tsv
Rscript make_tss.R --protein-coding --ucsc-refgene hg19.tss.refseq.tsv --out hg19.tss.refseq.bed
gzip hg19.tss.refseq.bed

# fetch the list of human housekeeping genes
# from this paper:
# "Human housekeeping genes revisited". Eisenberg and E.Y. Levanon, Trends in Genetics, 29 (2013)
wget http://www.tau.ac.il/~elieis/HKG/HK_genes.txt

# filter the human RefSeq TSS list to the housekeeping genes
cut -f1 HK_genes.txt | perl -pe 's/\s+$/\n/' > housekeeping_genes.txt
Rscript make_tss.R --protein-coding --ucsc-refgene hg19.tss.refseq.tsv --include-genes housekeeping_genes.txt --out hg19.tss.refseq.housekeeping.all.bed
gzip hg19.tss.refseq.housekeeping.all.bed

# fetch the mouse RefSeq TSS list from UCSC
mysql --host=genome-mysql.cse.ucsc.edu --user=genome -D mm9 -e "SELECT * FROM refGene" > mm9.tss.refseq.tsv
Rscript make_tss.R --protein-coding --ucsc-refgene mm9.tss.refseq.tsv --out mm9.tss.refseq.bed
gzip mm9.tss.refseq.bed

# fetch the rat RefSeq TSS list from UCSC
mysql --host=genome-mysql.cse.ucsc.edu --user=genome -D rn5 -e "SELECT * FROM refGene" > rn5.tss.refseq.tsv
Rscript make_tss.R --protein-coding --ucsc-refgene rn5.tss.refseq.tsv --out rn5.tss.refseq.bed
gzip rn5.tss.refseq.bed

# determine the human:mouse:rat 1:1:1 orthologues, and determine which are housekeeping genes (according to human data)
git clone https://github.com/porchard/get-one2one-orthologues.git
Rscript get_human_mouse_rat_orthologues.R orthologues.txt

# make the TSS file for human protein-coding housekeeping genes with 1:1:1 orthlogues
grep TRUE orthologues.txt | cut -f1 > human.housekeeping.orthologues.txt
Rscript make_tss.R --protein-coding --ucsc-refgene hg19.tss.refseq.tsv --include-genes human.housekeeping.orthologues.txt --out hg19.tss.refseq.housekeeping.ortho.bed
gzip hg19.tss.refseq.housekeeping.ortho.bed

# make the TSS file for mouse protein-coding with 1:1:1 orthologues, where the human orthologue is a housekeeping gene
grep TRUE orthologues.txt | cut -f3 > mouse.housekeeping.orthologues.txt
Rscript make_tss.R --protein-coding --ucsc-refgene mm9.tss.refseq.tsv --include-genes mouse.housekeeping.orthologues.txt --out mm9.tss.refseq.housekeeping.ortho.bed
gzip mm9.tss.refseq.housekeeping.ortho.bed

# make the TSS file for rat protein-coding with 1:1:1 orthologues, where the human orthologue is a housekeeping gene
grep TRUE orthologues.txt | cut -f2 > rat.housekeeping.orthologues.txt
Rscript make_tss.R --protein-coding --ucsc-refgene rn5.tss.refseq.tsv --include-genes rat.housekeeping.orthologues.txt --out rn5.tss.refseq.housekeeping.ortho.bed
gzip rn5.tss.refseq.housekeeping.ortho.bed
```
