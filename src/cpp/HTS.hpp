//
// Copyright 2015 Stephen Parker
//
// Licensed under Version 3 of the GPL or any later version
//

#ifndef HTS_HPP
#define HTS_HPP

#include <map>
#include <sstream>
#include <string>

#include <htslib/bgzf.h>
#include <htslib/kstring.h>
#include <htslib/sam.h>

#include "Exceptions.hpp"
#include "Utils.hpp"

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

typedef std::map<std::string, std::vector<std::map<std::string, std::string>>> sam_header;

std::string get_qname(const bam1_t* record);
std::string record_to_string(const bam_hdr_t* header, const bam1_t* record);
sam_header parse_sam_header(const std::string &header_text);
#endif
