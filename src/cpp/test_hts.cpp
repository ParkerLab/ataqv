#include <iostream>

#include "catch.hpp"

#include "HTS.hpp"


#define PI 3.141592653589793
#define E  2.718281828459045
#define HELLO "Hello, world!"
#define BEEF "DEADBEEF"

#define str(x) #x
#define xstr(x) str(x)


TEST_CASE("Test HTS file handling") {
    std::string sam("data:@SQ\tSN:chr20\tLN:63025520\nSRR891268.122333488\t83\tchr20\t60087\t60\t50M\t=\t60058\t-79\tAGGAAGGAGAGAGTGAAGGAACTGCCAGGTGACACACTCCCACCATGGAC\tJJJJJJJJJJJJJJJJJIHHGIGIIIJJJJJIIGGIJHHHHHFFFFFCBB\tMD:Z:50\tPG:Z:MarkDuplicates\tNM:i:0\tAS:i:50\tXS:i:23\n");

    samFile *in = sam_open(sam.c_str(), "r");
    bam_hdr_t *header = sam_hdr_read(in);
    bam1_t *record = bam_init1();

    REQUIRE(sam_read1(in, header, record) >= 0);

    SECTION("TEST HTS::get_qname", "[hts/get_qname]") {
        REQUIRE("SRR891268.122333488" == get_qname(record));
    }

    SECTION("TEST HTS::record_to_string", "[hts/record_to_string]") {
        REQUIRE("SRR891268.122333488\t83\tchr20\t60087\t60\t50M\t=\t60058\t-79\tAGGAAGGAGAGAGTGAAGGAACTGCCAGGTGACACACTCCCACCATGGAC\tJJJJJJJJJJJJJJJJJIHHGIGIIIJJJJJIIGGIJHHHHHFFFFFCBB\tMD:Z:50\tPG:Z:MarkDuplicates\tNM:i:0\tAS:i:50\tXS:i:23" == record_to_string(header, record));
    }
}


TEST_CASE("Test SAM header parsing", "[hts/parse_sam_header]") {
    std::string header_text = (
        "@HD	VN:1.4	SO:coordinate\n"
        "@SQ	SN:chr1	LN:249250621\n"
        "@SQ	SN:chr10	LN:135534747\n"
        "@SQ	SN:chr11	LN:135006516\n"
        "@SQ	SN:chr11_gl000202_random	LN:40103\n"
        "@SQ	SN:chr12	LN:133851895\n"
        "@SQ	SN:chr13	LN:115169878\n"
        "@SQ	SN:chr14	LN:107349540\n"
        "@SQ	SN:chr15	LN:102531392\n"
        "@SQ	SN:chr16	LN:90354753\n"
        "@SQ	SN:chr17	LN:81195210\n"
        "@SQ	SN:chr17_gl000203_random	LN:37498\n"
        "@SQ	SN:chr17_gl000204_random	LN:81310\n"
        "@SQ	SN:chr17_gl000205_random	LN:174588\n"
        "@SQ	SN:chr17_gl000206_random	LN:41001\n"
        "@SQ	SN:chr18	LN:78077248\n"
        "@SQ	SN:chr18_gl000207_random	LN:4262\n"
        "@SQ	SN:chr19	LN:59128983\n"
        "@SQ	SN:chr19_gl000208_random	LN:92689\n"
        "@SQ	SN:chr19_gl000209_random	LN:159169\n"
        "@SQ	SN:chr1_gl000191_random	LN:106433\n"
        "@SQ	SN:chr1_gl000192_random	LN:547496\n"
        "@SQ	SN:chr2	LN:243199373\n"
        "@SQ	SN:chr20	LN:63025520\n"
        "@SQ	SN:chr21	LN:48129895\n"
        "@SQ	SN:chr21_gl000210_random	LN:27682\n"
        "@SQ	SN:chr22	LN:51304566\n"
        "@SQ	SN:chr3	LN:198022430\n"
        "@SQ	SN:chr4	LN:191154276\n"
        "@SQ	SN:chr4_gl000193_random	LN:189789\n"
        "@SQ	SN:chr4_gl000194_random	LN:191469\n"
        "@SQ	SN:chr5	LN:180915260\n"
        "@SQ	SN:chr6	LN:171115067\n"
        "@SQ	SN:chr7	LN:159138663\n"
        "@SQ	SN:chr7_gl000195_random	LN:182896\n"
        "@SQ	SN:chr8	LN:146364022\n"
        "@SQ	SN:chr8_gl000196_random	LN:38914\n"
        "@SQ	SN:chr8_gl000197_random	LN:37175\n"
        "@SQ	SN:chr9	LN:141213431\n"
        "@SQ	SN:chr9_gl000198_random	LN:90085\n"
        "@SQ	SN:chr9_gl000199_random	LN:169874\n"
        "@SQ	SN:chr9_gl000200_random	LN:187035\n"
        "@SQ	SN:chr9_gl000201_random	LN:36148\n"
        "@SQ	SN:chrM	LN:16571\n"
        "@SQ	SN:chrUn_gl000211	LN:166566\n"
        "@SQ	SN:chrUn_gl000212	LN:186858\n"
        "@SQ	SN:chrUn_gl000213	LN:164239\n"
        "@SQ	SN:chrUn_gl000214	LN:137718\n"
        "@SQ	SN:chrUn_gl000215	LN:172545\n"
        "@SQ	SN:chrUn_gl000216	LN:172294\n"
        "@SQ	SN:chrUn_gl000217	LN:172149\n"
        "@SQ	SN:chrUn_gl000218	LN:161147\n"
        "@SQ	SN:chrUn_gl000219	LN:179198\n"
        "@SQ	SN:chrUn_gl000220	LN:161802\n"
        "@SQ	SN:chrUn_gl000221	LN:155397\n"
        "@SQ	SN:chrUn_gl000222	LN:186861\n"
        "@SQ	SN:chrUn_gl000223	LN:180455\n"
        "@SQ	SN:chrUn_gl000224	LN:179693\n"
        "@SQ	SN:chrUn_gl000225	LN:211173\n"
        "@SQ	SN:chrUn_gl000226	LN:15008\n"
        "@SQ	SN:chrUn_gl000227	LN:128374\n"
        "@SQ	SN:chrUn_gl000228	LN:129120\n"
        "@SQ	SN:chrUn_gl000229	LN:19913\n"
        "@SQ	SN:chrUn_gl000230	LN:43691\n"
        "@SQ	SN:chrUn_gl000231	LN:27386\n"
        "@SQ	SN:chrUn_gl000232	LN:40652\n"
        "@SQ	SN:chrUn_gl000233	LN:45941\n"
        "@SQ	SN:chrUn_gl000234	LN:40531\n"
        "@SQ	SN:chrUn_gl000235	LN:34474\n"
        "@SQ	SN:chrUn_gl000236	LN:41934\n"
        "@SQ	SN:chrUn_gl000237	LN:45867\n"
        "@SQ	SN:chrUn_gl000238	LN:39939\n"
        "@SQ	SN:chrUn_gl000239	LN:33824\n"
        "@SQ	SN:chrUn_gl000240	LN:41933\n"
        "@SQ	SN:chrUn_gl000241	LN:42152\n"
        "@SQ	SN:chrUn_gl000242	LN:43523\n"
        "@SQ	SN:chrUn_gl000243	LN:43341\n"
        "@SQ	SN:chrUn_gl000244	LN:39929\n"
        "@SQ	SN:chrUn_gl000245	LN:36651\n"
        "@SQ	SN:chrUn_gl000246	LN:38154\n"
        "@SQ	SN:chrUn_gl000247	LN:36422\n"
        "@SQ	SN:chrUn_gl000248	LN:39786\n"
        "@SQ	SN:chrUn_gl000249	LN:38502\n"
        "@SQ	SN:chrX	LN:155270560\n"
        "@SQ	SN:chrY	LN:59373566\n"
        "@RG	ID:SRR891275	LB:SRR891275	SM:GSM1155964	PL:ILLUMINA\n"
        "@RG	ID:SRR891278	LB:SRR891278	SM:GSM1155967	PL:ILLUMINA\n"
        "@PG	ID:bwa	PN:bwa	VN:0.7.12-r1039	CL:bwa mem -M -R @RG\\tLB:SRR891275\\tSM:GSM1155964\\tID:SRR891275\\tPL:ILLUMINA -t 4 /nfs/turbo/parkerlab1/lab/data/reference/human/hg19/index/bwa/0.7.12/hg19 GSM1155964.SRR891275.CD4+_ATACseq_Day1_Rep1.1.trimmed.fq.gz GSM1155964.SRR891275.CD4+_ATACseq_Day1_Rep1.2.trimmed.fq.gz\n"
        "@PG	ID:MarkDuplicates	VN:1.131(cd60f90fdca902499c70a4472b6162ef37f919ce_1431022382)	CL:picard.sam.markduplicates.MarkDuplicates INPUT=[SRR891275.bam] OUTPUT=SRR891275.md.bam METRICS_FILE=SRR891275.markdup.metrics ASSUME_SORTED=true TMP_DIR=[.] VALIDATION_STRINGENCY=LENIENT    MAX_SEQUENCES_FOR_DISK_READ_ENDS_MAP=50000 MAX_FILE_HANDLES_FOR_READ_ENDS_MAP=8000 SORTING_COLLECTION_SIZE_RATIO=0.25 PROGRAM_RECORD_ID=MarkDuplicates PROGRAM_GROUP_NAME=MarkDuplicates REMOVE_DUPLICATES=false DUPLICATE_SCORING_STRATEGY=SUM_OF_BASE_QUALITIES READ_NAME_REGEX=[a-zA-Z0-9]+:[0-9]:([0-9]+):([0-9]+):([0-9]+).* OPTICAL_DUPLICATE_PIXEL_DISTANCE=100 VERBOSITY=INFO QUIET=false COMPRESSION_LEVEL=5 MAX_RECORDS_IN_RAM=500000 CREATE_INDEX=false CREATE_MD5_FILE=false GA4GH_CLIENT_SECRETS=client_secrets.json	PN:MarkDuplicates\n"
        "@PG	ID:bwa-5A8E5097	PN:bwa	VN:0.7.12-r1039	CL:bwa mem -M -R @RG\\tLB:SRR891278\\tSM:GSM1155967\\tID:SRR891278\\tPL:ILLUMINA -t 4 /nfs/turbo/parkerlab1/lab/data/reference/human/hg19/index/bwa/0.7.12/hg19 GSM1155967.SRR891278.CD4+_ATACseq_Day2_Rep2.1.trimmed.fq.gz GSM1155967.SRR891278.CD4+_ATACseq_Day2_Rep2.2.trimmed.fq.gz\n"
        "@PG	ID:MarkDuplicates-571D76C	VN:1.131(cd60f90fdca902499c70a4472b6162ef37f919ce_1431022382)	CL:picard.sam.markduplicates.MarkDuplicates INPUT=[SRR891278.bam] OUTPUT=SRR891278.md.bam METRICS_FILE=SRR891278.markdup.metrics ASSUME_SORTED=true TMP_DIR=[.] VALIDATION_STRINGENCY=LENIENT    MAX_SEQUENCES_FOR_DISK_READ_ENDS_MAP=50000 MAX_FILE_HANDLES_FOR_READ_ENDS_MAP=8000 SORTING_COLLECTION_SIZE_RATIO=0.25 PROGRAM_RECORD_ID=MarkDuplicates PROGRAM_GROUP_NAME=MarkDuplicates REMOVE_DUPLICATES=false DUPLICATE_SCORING_STRATEGY=SUM_OF_BASE_QUALITIES READ_NAME_REGEX=[a-zA-Z0-9]+:[0-9]:([0-9]+):([0-9]+):([0-9]+).* OPTICAL_DUPLICATE_PIXEL_DISTANCE=100 VERBOSITY=INFO QUIET=false COMPRESSION_LEVEL=5 MAX_RECORDS_IN_RAM=500000 CREATE_INDEX=false CREATE_MD5_FILE=false GA4GH_CLIENT_SECRETS=client_secrets.json	PN:MarkDuplicates\n"
    );

    sam_header header = parse_sam_header(header_text);

    auto programs = header["PG"];
    REQUIRE(programs.size() == 4);

    REQUIRE(programs[0]["ID"] == "bwa");
    REQUIRE(programs[0]["VN"] == "0.7.12-r1039");

    REQUIRE(programs[2]["ID"] == "bwa-5A8E5097");
    REQUIRE(programs[2]["VN"] == "0.7.12-r1039");

    auto read_groups = header["RG"];
    REQUIRE(read_groups.size() == 2);
    REQUIRE(read_groups[0]["ID"] == "SRR891275");
    REQUIRE(read_groups[1]["ID"] == "SRR891278");

    auto references = header["SQ"];
    REQUIRE(references.size() == 84);
}
