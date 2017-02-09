#include <algorithm>
#include <iostream>
#include <string>

#include "Exceptions.hpp"
#include "HTS.hpp"
#include "Utils.hpp"

#include <htslib/kstring.h>


std::string get_qname(const bam1_t* record) {
    return std::string(record && record->data ? ((char *)(record)->data) : "");
}


std::string record_to_string(const bam_hdr_t* header, const bam1_t* record) {
    kstring_t ks = {0,0,0};
    if (sam_format1(header, record, &ks) < 0) {
        throw HTSException("Could not format record " + get_qname(record));
    }
    return std::string(ks_release(&ks));
}


sam_header parse_sam_header(const std::string &header_text) {
    sam_header header;

    std::stringstream headers_stream(header_text);

    std::string header_tag;

    for (std::string header_string; std::getline(headers_stream, header_string); ) {
        std::stringstream header_stream(header_string);

        header_stream >> header_tag;
        header_tag = header_tag.substr(1);

        std::map<std::string, std::string> field_map;
        for (std::string field; std::getline(header_stream, field, '\t');) {
            std::stringstream field_stream(field);
            for (std::string field_text; std::getline(field_stream, field_text, '\t');) {
                std::vector<std::string> splitfield = split(field_text, ":");
                field_map[splitfield[0]] = splitfield[1];
            }
        }
        header[header_tag].push_back(field_map);
    }

    return header;
}
