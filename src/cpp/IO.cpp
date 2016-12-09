#include <iostream>

#include "IO.hpp"


///
/// Check for the GZIP header in a file
///
bool is_gzipped(std::string filename) {
    bool gzipped = false;
    FILE* f = fopen(filename.c_str(), "rb");

    if (f == nullptr) {
        throw FileException("Could not open file \"" + filename + "\": " + std::strerror(errno));
    } else {
        if (fgetc(f) == 0x1f && fgetc(f) == 0x8b) {
            gzipped = true;
        }
    }
    fclose(f);
    return gzipped;
}


bool is_gzipped_filename(std::string filename) {
    std::string suffix = ".gz";
    if (filename.length() >= suffix.length()) {
        return (0 == filename.compare(filename.length() - suffix.length(), suffix.length(), suffix));
    } else {
        return false;
    }
}


///
/// mistream : "magic istream" opens a file, automatically decompressing as needed
///
boost::shared_ptr<boost::iostreams::filtering_istream> mistream(const std::string& filename) {
    boost::shared_ptr<boost::iostreams::filtering_istream> filtering_istream(new boost::iostreams::filtering_istream());

    if (filename.empty()) {
        throw FileException("Cannot open without a filename.");
    }

    boost::iostreams::file_source source(filename);
    if (!source.is_open()) {
        throw FileException(strerror(errno));
    }

    if (is_gzipped(filename)) {
        filtering_istream->push(boost::iostreams::gzip_decompressor());
    }

    filtering_istream->push(source, std::ifstream::binary);
    return filtering_istream;
}


///
/// mostream : "magic ostream" opens a file, automatically compressing if the filename ends in ".gz"
///
boost::shared_ptr<boost::iostreams::filtering_ostream> mostream(const std::string& filename) {
    boost::shared_ptr<boost::iostreams::filtering_ostream> filtering_ostream(new boost::iostreams::filtering_ostream());

    if (filename.empty()) {
        throw FileException("Cannot open the file without a filename.");
    }

    if (is_gzipped_filename(filename)) {
        filtering_ostream->push(boost::iostreams::gzip_compressor());
    }

    boost::iostreams::file_sink sink(filename, std::ofstream::binary);
    if (!sink.is_open()) {
        throw FileException(strerror(errno));
    }

    filtering_ostream->push(sink);
    return filtering_ostream;
}
