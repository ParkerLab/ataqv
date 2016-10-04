#!/bin/bash

INBAM=$1

[[ -f "$INBAM.bai" ]] || samtools index $INBAM

# high quality autosomal
samtools view -h -f 3 -F 4 -F 8 -F 256 -F 1024 -F 2048 -q30 $INBAM chr1 | samtools sort -n | samtools view -O SAM | head -100 > hqaa.sam
samtools view -h -f 3 -F 4 -F 8 -F 256 -F 1024 -F 2048 -q30 $INBAM chr2 | samtools sort -n | samtools view -O SAM | head -100 >> hqaa.sam

# high quality allosomal
samtools view -h -f 3 -F 4 -F 8 -F 256 -F 1024 -F 2048 -q30 $INBAM chrX | samtools sort -n | samtools view -O SAM | head -10 > hqxy.sam
samtools view -h -f 3 -F 4 -F 8 -F 256 -F 1024 -F 2048 -q30 $INBAM chrY | samtools sort -n | samtools view -O SAM | head -10 >> hqxy.sam

# duplicate autosomal
samtools view -h -f 1024 $INBAM chr1 | samtools sort -n | samtools view -O SAM | head -25 > duplicate.sam
samtools view -h -f 1024 $INBAM chr2 | samtools sort -n | samtools view -O SAM | head -25 >> duplicate.sam

# mitochondrial
samtools view -h -f 3 -F 4 -F 8 -F 256 -F 1024 -F 2048 $INBAM chrM | samtools sort -n | samtools view -O SAM | head -100 > mitochondrial.sam

# duplicate mitochondrial
samtools view -h -f 1024 $INBAM chrM | samtools sort -n | samtools view -O SAM | head -100 > duplicate_mitochondrial.sam

# unmapped
samtools view -h -f 4 $INBAM | samtools sort -n | samtools view -O SAM | head -5 > unmapped.sam

# unmapped mate
samtools view -h -f 8 $INBAM | samtools sort -n | samtools view -O SAM | head -5 > unmappedmate.sam

# secondary
samtools view -h -f 256 $INBAM | samtools sort -n | samtools view -O SAM | head -10 > secondary.sam

# unlikely fragment length
(samtools view -H $INBAM; (samtools view $INBAM | awk 'sqrt($9 * $9)>1000 {print $0}')) | samtools sort -n | samtools view -O SAM | head -10 > toofar.sam

# mapped to different references
for dr in $(samtools view -q 0 $INBAM | awk '{print $1, $3}' | sort | uniq -c | grep -P '^\s+1 ' | awk '{print $2}' | sort | uniq | head -10); do samtools view $INBAM | grep -P "^$dr\t" ; done > dr.sam

(samtools view -H $INBAM; cat *.sam) | samtools sort -n -O BAM -o test.bam
