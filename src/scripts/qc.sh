#!/bin/bash

set -e

#
# qc.sh: run quality checks on ATAC-seq data
#

# The lines starting with 'drmr:' are directives for our pipeline
# system (https://github.com/ParkerLab/drmr/). You can ignore them and
# run the output of this script as an ordinary shell script, but if
# you have more than a few files to analyze, and a cluster or large
# lab server to run on, you can use drmr to easily submit the
# generated pipeline to a workload management system like Slurm or
# PBS, which will get you results faster.

if [ -z "$PICARD_HOME" ]
then
    >&2 echo "Please set the PICARD_HOME environment variable to the directory containing picard.jar"
    exit 1
fi

BAM=$1
BASE=$(basename $1 .bam)
BAMMD=${BASE}.md.bam
PPMAU=${BASE}.ppmau.bam

# Reduce the references in the BAM file to the autosomal chromosomes
CHROMOSOMES=$(samtools view -H ${BAM} | grep -v '^@PG' | cut -f 2 | grep -v -e _ -e chrM -e chrX -e chrY -e 'VN:' | sed 's/SN://' | xargs echo)

QC="qc-${BAM}.sh"
cat >"$QC" <<EOF
#!/bin/bash

# drmr:job processors=4 memory=8gb

# create a BAM file of all the original data, with duplicates marked
java -Xmx8g -Xms8g -jar $PICARD_HOME/picard.jar MarkDuplicates I=${BAM} O=${BAMMD} ASSUME_SORTED=true METRICS_FILE=${BAMMD}.markdup.metrics VALIDATION_STRINGENCY=LENIENT

# drmr:wait

# drmr:job processors=1

# index the BAM file with duplicates marked, so we can prune it
samtools index ${BAMMD}

# drmr:wait

# extract the properly paired and mapped autosomal reads with good quality
samtools view -b -h -f 3 -F 4 -F 256 -F 1024 -F 2048 -q 30 ${BAMMD} $CHROMOSOMES > ${PPMAU}

# drmr:wait

# Call broad peaks with macs2 (change genome size as needed for your data)
macs2 callpeak -t ${PPMAU} -f BAM -n ${PPMAU}.macs2 -g 'hs' --nomodel --shift -100 --extsize 200 -B --broad

# drmr:wait

# Run ataqc
ataqc --peak-file ${PPMAU}.macs2_peaks.broadPeak --metrics-file ${BAMMD}.json ${BAMMD} > ${BAMMD}.ataqc.out

# drmr:wait

mkarv ${BAMMD}.web ${BAMMD}.json

EOF

cat <<EOF
QC script written to $QC. Please review it to make sure things like
Picard's memory sizing, MACS2 parameters, and the ataqc reference genome
are all correct for your data.
EOF
