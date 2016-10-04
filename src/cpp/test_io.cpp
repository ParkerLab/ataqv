#include <cstdio>
#include <iostream>

#include "catch.hpp"

#include "IO.hpp"


TEST_CASE("Test gzipped filename detection", "[io/is_gzipped_filename]") {
    REQUIRE_FALSE(is_gzipped_filename("foo.bed"));
    REQUIRE(is_gzipped_filename("foo.bed.gz"));
}


TEST_CASE("Test gzipped output", "[io/mostream]") {

    SECTION("Happy path") {
        std::string filename = "mostream.test.gz";
        {
            auto out = mostream(filename);
            *out << "Hey there.\n";
        }

        REQUIRE(is_gzipped(filename));

        {
            auto in = mistream(filename);
            std::string content;
            std::getline(*in, content);
            REQUIRE("Hey there." == content);
        }
        std::remove(filename.c_str());
    }

    SECTION("Missing file") {
        REQUIRE_THROWS(is_gzipped("something/not/there.gz"));

        REQUIRE_THROWS(mostream(""));
        REQUIRE_THROWS(mostream("something/not/there.gz"));

        REQUIRE_THROWS(mistream(""));
        REQUIRE_THROWS(mistream("something/not/there.gz"));
    }
}
