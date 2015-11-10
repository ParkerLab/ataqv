#!/usr/bin/env Rscript

#
# Copyright 2015 The Parker Lab at the University of Michigan
#
# Licensed under Version 3 of the GPL or any later version
#

#
# plot_template_lengths: produces a plot from a file mapping ATAC-seq
# template sizes to the fractions of the number of reads in the dataset
# they represent
#
# You should normally expect the vast majority of reads to be around
# 50nt in length, with a couple of much smaller peaks around
# mononucleosomal and dinucleosomal lengths.
#

suppressMessages(library("RColorBrewer"))
suppressMessages(library("ggplot2"))
suppressMessages(library("grid"))
suppressMessages(library("gtools"))

argv <- commandArgs(TRUE)

df = read.delim(file=argv[1], header=TRUE, sep="\t")
df$Sample <- as.factor(df$Sample)
df$Group <- as.factor(df$Group)

p <- ggplot(df, aes(x=TemplateLength, y=FractionOfPerfectAutosomalReads, colour=Group))
p <- p + scale_colour_discrete(name="Group", breaks=mixedsort(levels(df$Group)), label=mixedsort(levels(df$Group)))
p <- p + geom_line(alpha=0.75) + facet_wrap(~ Sample) + ggtitle(argv[2])
p <- p + xlab('Template length (bp)') + ylab('Fraction of reads')
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
p <- p + coord_cartesian(xlim=c(0,500))

pdf_name <- paste0(argv[1], ".pdf")
ggsave(p, file=pdf_name, width=11, height=8.5, units="in")

cat(paste("Plot written to", pdf_name, "\n"))
