# TSS files

TSS files for hg19:

1. `hg19.tss.refseq.bed.gz` is all RefSeq TSS for human protein-coding genes
2. `hg19.tss.refseq.housekeeping.all.bed.gz` is all RefSeq TSS for human protein-coding housekeeping genes.

These were generated using the following commands:

```bash
# fetch the human RefSeq TSS list from UCSC
mysql --host=genome-mysql.cse.ucsc.edu --user=genome -D hg19 -e "SELECT * FROM refGene" > hg19.tss.refseq.tsv
Rscript ../../src/scripts/make_tss.R --protein-coding --ucsc-refgene hg19.tss.refseq.tsv --out hg19.tss.refseq.bed
gzip hg19.tss.refseq.bed

# fetch the list of human housekeeping genes
# from this paper:
# "Human housekeeping genes revisited". Eisenberg and E.Y. Levanon, Trends in Genetics, 29 (2013)
wget http://www.tau.ac.il/~elieis/HKG/HK_genes.txt

# filter the human refseq TSS list to the housekeeping genes
cut -f1 HK_genes.txt | perl -pe 's/\s+$/\n/' > housekeeping_genes.txt
Rscript ../../src/scripts/make_tss.R --protein-coding --ucsc-refgene hg19.tss.refseq.tsv --include-genes housekeeping_genes.txt --out hg19.tss.refseq.housekeeping.all.bed
gzip hg19.tss.refseq.housekeeping.all.bed

# clean-up
rm HK_genes.txt housekeeping_genes.txt hg19.tss.refseq.tsv
```
