#!/usr/bin/env Rscript

#
# Copyright 2015 The Parker Lab at the University of Michigan
#
# Licensed under Version 3 of the GPL or any later version
#

#
# plot_peak_metrics: produce plots from QC peak metrics
#

suppressMessages(library("ggplot2"))
suppressMessages(library("grid"))
suppressMessages(library("scales"))

argv <- commandArgs(TRUE)

cat(paste("Reading peak metrics from ", argv[1], "...\n", sep=""))

df = read.table(file=argv[1], header=TRUE, sep="\t")

df$Sample <- as.factor(df$Sample)
df$Group <- as.factor(df$Group)

cat("Generating peak/read count plot...\n")

#
# plot read count versus fraction of reads
#

p <- ggplot(df, aes(y=CumulativeFractionOfPerfectAutosomalReads, x=PeakRank, color=Group))
p <- p + geom_line() + facet_wrap(~ Sample) + ggtitle(paste0('Peak Read Count: ', argv[2]))
p <- p + ylab('Cumulative Fraction of Perfect Autosomal Reads') + xlab('Peak Rank (peaks are ranked by number of overlapping reads)')
p <- p + theme_bw(base_size=10)
p <- p + theme(
  strip.background = element_rect(fill="gray90", colour=FALSE),
  panel.border = element_rect(colour="gray90"),
  panel.margin = unit(1, "lines"),
  plot.title=element_text(vjust=1),
  axis.title.x=element_text(vjust=-0.5),
  axis.title.y=element_text(vjust=1)
)
p <- p + scale_fill_brewer(palette="Dark2")
p <- p + scale_x_continuous(labels=comma)

pdf_name <- paste0(argv[1], ".read_counts.pdf")
ggsave(p, file=pdf_name, width=11, height=8.5, units="in")

cat(paste("Read count plot written to", pdf_name, "\n"))

cat("Generating peak territory plot...\n")

#
# plot territory versus fraction of reads
#

p <- ggplot(df, aes(y=CumulativeFractionOfPerfectAutosomalReads, x=CumulativeFractionOfTotalPeakTerritory, color=Group))
p <- p + geom_line() + facet_wrap(~ Sample) + ggtitle(paste0('Peak territory: ', argv[2]))
p <- p + ylab('Cumulative Fraction of Perfect Autosomal Reads') + xlab('Cumulative Fraction of Total Peak Territory')
p <- p + theme_bw(base_size=10)
p <- p + theme(
  strip.background = element_rect(fill="gray90", colour=FALSE),
  panel.border = element_rect(colour="gray90"),
  panel.margin = unit(1, "lines"),
  plot.title=element_text(vjust=1),
  axis.title.x=element_text(vjust=-0.5),
  axis.title.y=element_text(vjust=1)
)
p <- p + scale_fill_brewer(palette="Dark2")

pdf_name <- paste0(argv[1], ".territory.pdf")
ggsave(p, file=pdf_name, width=11, height=8.5, units="in")

cat(paste("Territory plot written to", pdf_name, "\n"))
