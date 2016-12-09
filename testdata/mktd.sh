#!/bin/bash

set -e

INBAM=$1

test -f "$INBAM.bai" || samtools index $INBAM

# high quality autosomal
echo "Extracting high-quality autosomal alignments..."
samtools view -h -f 3 -F 4 -F 8 -F 256 -F 1024 -F 2048 -q30 $INBAM chr1 | samtools sort -O sam -n | samtools view | head -100 > hqaa.sam
samtools view -h -f 3 -F 4 -F 8 -F 256 -F 1024 -F 2048 -q30 $INBAM chr2 | samtools sort -O sam -n | samtools view | head -100 >> hqaa.sam

# high quality allosomal
echo "Extracting high-quality allosomal alignments..."
samtools view -h -f 3 -F 4 -F 8 -F 256 -F 1024 -F 2048 -q30 $INBAM chrX | samtools sort -O sam -n | samtools view | head -10 > hqxy.sam
samtools view -h -f 3 -F 4 -F 8 -F 256 -F 1024 -F 2048 -q30 $INBAM chrY | samtools sort -O sam -n | samtools view | head -10 >> hqxy.sam

# duplicate autosomal
echo "Extracting duplicate autosomal alignments..."
samtools view -h -f 1024 $INBAM chr1 | samtools sort -O sam -n | samtools view | head -25 > duplicate.sam
samtools view -h -f 1024 $INBAM chr2 | samtools sort -O sam -n | samtools view | head -25 >> duplicate.sam

# mitochondrial
echo "Extracting mitochondrial alignments..."
samtools view -h -f 3 -F 4 -F 8 -F 256 -F 1024 -F 2048 $INBAM chrM | samtools sort -O sam -n | samtools view | head -100 > mitochondrial.sam

# duplicate mitochondrial
echo "Extracting duplicate mitochondrial alignments..."
samtools view -h -f 1024 $INBAM chrM | samtools sort -O sam -n | samtools view | head -100 > duplicate_mitochondrial.sam

# unmapped
echo "Extracting unmapped reads..."
samtools view -h -f 4 $INBAM | samtools sort -O sam -n | samtools view | head -5 > unmapped.sam

# unmapped mate
echo "Extracting unmapped mate reads..."
samtools view -h -f 8 $INBAM | samtools sort -O sam -n | samtools view | head -5 > unmappedmate.sam

# secondary
echo "Extracting secondary alignments..."
samtools view -h -f 256 $INBAM | samtools sort -O sam -n | samtools view | head -10 > secondary.sam

# unlikely fragment length
echo "Extracting alignments with unlikely fragment length..."
(samtools view -H $INBAM; (samtools view $INBAM | awk 'sqrt($9 * $9)>1000 {print $0}')) | samtools sort -O sam -n | samtools view | head -10 > toofar.sam

# mapped to different references
echo "Extracting alignments with mates mapped to different references..."
for dr in $(samtools view -q 0 $INBAM | awk '{print $1, $3}' | sort | uniq -c | grep -P '^\s+1 ' | awk '{print $2}' | sort | uniq | head -10); do samtools view $INBAM | grep -P "^$dr\t" ; done > dr.sam

echo "Creating test BAM..."
(samtools view -H $INBAM; cat *.sam) | samtools sort -O bam -n -o test.$INBAM -

echo "Done."
