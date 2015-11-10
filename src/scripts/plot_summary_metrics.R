#!/usr/bin/env Rscript

#
# Copyright 2015 The Parker Lab at the University of Michigan
#
# Licensed under Version 3 of the GPL or any later version
#

#
# plot each metric in a file of combined summary metrics
#

suppressMessages(library("ggplot2"))
suppressMessages(library("grid"))
suppressMessages(library("gtools"))

argv <- commandArgs(TRUE)

df = read.delim(file=argv[1], header=TRUE, sep="\t")
df <- subset(df, select=grep("Sample|Group|Percentage|Ratio", names(df)))
df$Sample <- factor(df$Sample)
df$Group <- factor(df$Group)

pdf_name <- paste0(argv[1], ".pdf")
pdf(file=pdf_name, width=11.5, height=8)
for (variable in grep("Percentage|Ratio", names(df), perl=TRUE, value=TRUE)) {
    cat(paste("Plotting", variable, "\n"))
    p <- ggplot(df, aes_string(x="Sample", y=variable, color="Group", label="Group"))
    p <- p + scale_colour_discrete(name="Group", breaks=mixedsort(levels(df$Group)), label=mixedsort(levels(df$Group)))
    p <- p + geom_point(alpha=0.25, size=8) + geom_text(size=3) + ggtitle(paste(argv[2], ": ", variable, sep=""))
    p <- p + xlab("Sample")
    p <- p + theme_bw(base_size=10)
    p <- p + theme(axis.text.x = element_text(angle = 45, vjust = 1, hjust=1))
    p <- p + theme(
      strip.background = element_rect(fill="gray90", colour=FALSE),
      panel.border = element_rect(colour="gray90"),
      panel.margin = unit(1, "lines"),
      plot.title=element_text(vjust=1),
      axis.title.x=element_text(vjust=-0.5),
      axis.title.y=element_text(vjust=1)
    )
    p <- p + scale_fill_brewer(palette="Dark2")
    print(p)
}
dev.off()

cat(paste("Plot written to", pdf_name, "\n"))
