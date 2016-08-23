//
// Copyright 2015 Stephen Parker
//
// Licensed under Version 3 of the GPL or any later version
//

#ifndef HTS_HPP
#define HTS_HPP
#include <htslib/bgzf.h>
#include <htslib/sam.h>
#include <htslib/kstring.h>

///
/// Most of these conditions are taken directly from samtools
///
#define IS_DUP(bam) (bam->core.flag & BAM_FDUP)
#define IS_MATE_REVERSE(bam) (bam->core.flag & BAM_FMREVERSE)
#define IS_MATE_UNMAPPED(bam) (bam->core.flag & BAM_FMUNMAP)
#define IS_ORIGINAL(bam) ((bam->core.flag & (BAM_FSECONDARY|BAM_FSUPPLEMENTARY)) == 0)
#define IS_PAIRED(bam) (bam->core.flag & BAM_FPAIRED)
#define IS_PAIRED_AND_MAPPED(bam) ((bam->core.flag & BAM_FPAIRED) && !(bam->core.flag & BAM_FUNMAP) && !(bam->core.flag & BAM_FMUNMAP))
#define IS_PROPERLYPAIRED(bam) ((bam->core.flag & (BAM_FPAIRED|BAM_FPROPER_PAIR)) == (BAM_FPAIRED|BAM_FPROPER_PAIR) && !(bam->core.flag & BAM_FUNMAP) && !(bam->core.flag & BAM_FMUNMAP))
#define IS_QCFAIL(bam) (bam->core.flag & BAM_FQCFAIL)
#define IS_READ1(bam) (bam->core.flag & BAM_FREAD1)
#define IS_READ2(bam) (bam->core.flag & BAM_FREAD2)
#define IS_REVERSE(bam) (bam->core.flag & BAM_FREVERSE)
#define IS_SECONDARY(bam) (bam->core.flag & BAM_FSECONDARY)
#define IS_SUPPLEMENTARY(bam) (bam->core.flag & BAM_FSUPPLEMENTARY)
#define IS_UNMAPPED(bam) (bam->core.flag & BAM_FUNMAP)

#endif
