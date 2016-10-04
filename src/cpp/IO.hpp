#ifndef IO_HPP
#define IO_HPP

#include <cerrno>
#include <cstring>
#include <string>

#include <boost/iostreams/device/file.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/stream.hpp>

#include "Exceptions.hpp"


///
/// Check for the GZIP header in a file
///
bool is_gzipped(std::string filename);

//
// Check if a filename looks gzipped
//
bool is_gzipped_filename(std::string filename);


///
/// mistream : "magic istream" opens a file, automatically decompressing as needed
///
boost::shared_ptr<boost::iostreams::filtering_istream> mistream(const std::string& filename);


///
/// mostream : "magic ostream" opens a file, automatically compressing if the filename ends in ".gz"
///
boost::shared_ptr<boost::iostreams::filtering_ostream> mostream(const std::string& filename);


#endif // IO_HPP
