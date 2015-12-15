#!/bin/bash

set -e

#
# qc.sh: run quality checks on ATAC-seq data
#

if [ -z "$PICARD_HOME" ]
then
    >&2 echo "Please set the PICARD_HOME environment variable to the directory containing picard.jar"
    exit 1
fi

BAM=$1
BASE=$(basename $1 .bam)
BAMMD=${BASE}.md.bam
PPMAU=${BASE}.ppmau.bam

SAMPLE=${2:-${BASE}}
GROUP=${3:-${BASE}}

# Reduce the references in the BAM file to the autosomal chromosomes
CHROMOSOMES=$(samtools view -H ${BAM} | grep -v '^@PG' | cut -f 2 | grep -v -e _ -e chrM -e chrX -e chrY -e 'VN:' | sed 's/SN://' | xargs echo)

QC="qc-${BAM}.sh"
cat >"$QC" <<EOF

# swarm:pbs nodes=nodes=1:ppn=4

# create a BAM file of all the original data, with duplicates marked
java -Xmx8g -Xms8g -jar $PICARD_HOME/picard.jar MarkDuplicates I=${BAM} O=${BAMMD} ASSUME_SORTED=true METRICS_FILE=${BAMMD}.markdup.metrics VALIDATION_STRINGENCY=LENIENT

# swarm:wait

# swarm:pbs nodes=nodes=1:ppn=1

# index the BAM file with duplicates marked, so we can prune it
samtools index ${BAMMD}

# swarm:wait

# extract the properly paired and mapped autosomal reads with good quality
samtools view -b -h -f 3 -F 4 -F 256 -F 1024 -F 2048 -q 30 ${BAMMD} $CHROMOSOMES > ${PPMAU}

# swarm:wait

# swarm:pbs nodes=nodes=1:ppn=4

# sort the pruned BAM file by name; bamToBed wants the read pairs together
samtools sort -@ 8 -n ${PPMAU} ${BASE}.ppmau.readsorted

# swarm:pbs nodes=nodes=1:ppn=1

# Call peaks with macs2
macs2 callpeak -t ${PPMAU} -f BAM -n ${PPMAU}.macs2 -g 'hs' --nomodel --shift -100 --extsize 200 -B --broad

# swarm:wait

# convert the pruned BAM to BEDPE
bamToBed -bedpe -i ${BASE}.ppmau.readsorted.bam > ${BASE}.ppmau.readsorted.bedpe

# swarm:wait

# Collect the templates overlapping each peak.

# Sort them by coordinates here, so we can use intersectBed's -sorted option (see
# http://bedtools.readthedocs.org/en/latest/content/tools/intersect.html)

sort -k1,1 -k2,2n ${BASE}.ppmau.readsorted.bedpe > ${BASE}.ppmau.locsorted.bedpe

intersectBed -sorted -a ${PPMAU}.macs2_peaks.broadPeak -b ${BASE}.ppmau.locsorted.bedpe -wb > ${PPMAU}.peak.templates.bed

# swarm:wait

# Run ataqc on the data from the original BAM file with duplicates marked
ataqc -p ${PPMAU}.macs2_peaks.broadPeak -o ${PPMAU}.peak.templates.bed ${BAMMD} ${SAMPLE} ${GROUP} > ${BAMMD}.ataqc.out

# swarm:wait

# Call R to plot the template lengths
plot_template_lengths.R ${BAMMD}.template_lengths "${SAMPLE} ${GROUP}"

# Call R to plot the peak metrics
plot_peak_metrics.R ${BAMMD}.peak_metrics "${SAMPLE} ${GROUP}"

# Call R to plot the summary metrics
plot_summary_metrics.R ${BAMMD}.summary "${SAMPLE} ${GROUP}"

EOF

cat <<EOF
QC script written to $QC. Please review it to make sure things like
Picard's memory sizing, MACS2 parameters, and the ataqc reference genome
are all correct for your data.
EOF
