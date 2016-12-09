#include <algorithm>
#include <iostream>
#include <string>

#include "HTS.hpp"
#include "Utils.hpp"


std::string get_qname(const bam1_t* record) {
    return std::string(record && record->data ? ((char *)(record)->data) : "");
}


std::string record_to_string(const bam_hdr_t* header, const bam1_t* record) {
    std::string reference_name(record->core.tid >= 0 ? header->target_name[record->core.tid] : "*");
    std::string mate_reference_name(record->core.mtid >= 0 ? header->target_name[record->core.mtid] : "*");

    std::stringstream s;
    s << get_qname(record)
      << "\t" << record->core.flag
      << "\t" << reference_name
      << "\t" << record->core.pos + 1
      << "\t" << mate_reference_name
      << "\t" << record->core.mpos + 1
      << "\t" << record->core.isize;
    return s.str();
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
