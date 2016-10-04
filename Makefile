#
# VARIABLES
#

VERSION = 0.5.0

#
# PATHS
#

SRC_DIR = src
CPP_DIR = $(SRC_DIR)/cpp
SCRIPTS_DIR = $(SRC_DIR)/scripts
WEB_DIR = $(SRC_DIR)/web
BUILD_DIR = build
TEST_DIR = testing

SRC_CPP := $(wildcard $(CPP_DIR)/*.cpp)
SRC_HPP := $(wildcard $(CPP_DIR)/*.hpp)
INCLUDES := $(addprefix -I,$(CPP_DIR))

SCRIPTS := $(wildcard $(SCRIPTS_DIR)/*)

# where to install -- the default location assumes environment modules
# are in use, but you can simply override PREFIX with a command like
# 'make install PREFIX=...'
MODULES_ROOT = /lab/sw/modules
PREFIX = $(MODULES_ROOT)/ataqc/$(VERSION)

# where to install the modulefile
MODULEFILES_ROOT = /lab/sw/modulefiles
MODULEFILE_PATH = $(MODULEFILES_ROOT)/ataqc/$(VERSION)

#
# FLAGS
#

CPPFLAGS = -pedantic -Wall -Wextra -Wwrite-strings -Wstrict-overflow -fno-strict-aliasing $(INCLUDES)
CXXFLAGS = -std=c++11 -pthread -O3 $(CPPFLAGS)
CXXFLAGS_DEV = -std=c++11 -Weffc++ -pthread -O3 -g

#
# Try to locate dependencies using environment variables
#

#
# Boost
#

#
# If your Boost installation used their 'tagged' layout, the libraries
# will include metadata in their names. Specify BOOST_TAGGED=yes in
# your make command to link with tagged libraries.
#
ifdef BOOST_TAGGED
	BOOST_LIBS = -lboost_filesystem-mt -lboost_iostreams-mt -lboost_system-mt -lboost_chrono-mt
else
	BOOST_LIBS = -lboost_filesystem -lboost_iostreams -lboost_system -lboost_chrono
endif

ifdef BOOST_ROOT
	CPPFLAGS += -I$(BOOST_ROOT)/include
	LDFLAGS += -L$(BOOST_ROOT)/lib
else
	ifdef BOOST_INCLUDE
		CPPFLAGS += -I$(BOOST_INCLUDE
	endif

	ifdef BOOST_LIB
		LDFLAGS += -L$(BOOST_LIB)
	endif
endif

#
# HTSlib
#
ifdef HTSLIB_ROOT
	CPPFLAGS += -I$(HTSLIB_ROOT)/include
	LDFLAGS += -L$(HTSLIB_ROOT)/lib
else
	ifdef HTSLIB_INCLUDE
		CPPFLAGS += -I$(HTSLIB_INCLUDE)
	endif

	ifdef HTSLIB_LIB
		LDFLAGS += -L$(HTSLIB_LIB)
	endif
endif

#
# htslib can be built with cURL to support HTTPS, which is cool, but
# drags in more libraries
#
ifdef HTSLIBCURL
	HTS_LIBS = -lhts -lssl -lcurl -lcrypto
else
	HTS_LIBS = -lhts
endif

LDLIBS = $(BOOST_LIBS) $(HTS_LIBS) -lz -lncurses

#
# Architecture flags
#
UNAME_P := $(shell uname -p)
ifeq ($(UNAME_P),x86_64)
	CPPFLAGS += -D AMD64
endif
ifneq ($(filter %86,$(UNAME_P)),)
	CPPFLAGS += -D IA32
endif
ifneq ($(filter arm%,$(UNAME_P)),)
	CPPFLAGS += -D ARM
endif

UNAME_S := $(shell uname -s)

ifeq ($(UNAME_S),Linux)
	CPPFLAGS += -D LINUX
endif

#
# TARGETS
#

.PHONY: all checkdirs clean install install-ataqc install-module install-scripts install-web test

all: checkdirs $(BUILD_DIR)/ataqc

checkdirs: $(BUILD_DIR) $(TEST_DIR)

$(BUILD_DIR):
	@mkdir -p $@

$(TEST_DIR):
	@mkdir -p $@

$(BUILD_DIR)/ataqc: $(BUILD_DIR)/ataqc.o $(BUILD_DIR)/Features.o $(BUILD_DIR)/HTS.o $(BUILD_DIR)/IO.o $(BUILD_DIR)/Metrics.o $(BUILD_DIR)/Peaks.o $(BUILD_DIR)/Utils.o
	$(CXX) -o $@ $^ $(LDFLAGS) $(LDLIBS)

$(BUILD_DIR)/%.o: $(CPP_DIR)/%.cpp $(SRC_HPP) $(CPP_DIR)/Version.hpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -o $@ -c $<

$(CPP_DIR)/Version.hpp: Makefile
	@echo '//' >$@
	@echo '// Copyright 2015 Stephen Parker' >>$@
	@echo '//' >>$@
	@echo '// Licensed under Version 3 of the GPL or any later version' >>$@
	@echo '//' >>$@
	@echo >>$@
	@echo '#define VERSION "$(VERSION)"' >>$@

test: checkdirs $(TEST_DIR)/run_ataqc_tests
	@cp testdata/* $(TEST_DIR)
	@cd $(TEST_DIR) && ./run_ataqc_tests -i
	@cd $(TEST_DIR) && lcov --quiet --capture --derive-func-data --directory . --output-file ataqc.info && lcov --remove ataqc.info catch.hpp --remove ataqc.info json.hpp --output-file ataqc.info && genhtml ataqc.info -o ataqc

$(TEST_DIR)/run_ataqc_tests: $(TEST_DIR)/run_ataqc_tests.o $(TEST_DIR)/test_features.o $(TEST_DIR)/test_hts.o $(TEST_DIR)/test_io.o $(TEST_DIR)/test_metrics.o $(TEST_DIR)/test_peaks.o $(TEST_DIR)/test_utils.o $(TEST_DIR)/Features.o $(TEST_DIR)/HTS.o $(TEST_DIR)/IO.o $(TEST_DIR)/Metrics.o $(TEST_DIR)/Peaks.o $(TEST_DIR)/Utils.o
	$(CXX) -o $@ $^ $(LDFLAGS) --coverage $(LDLIBS)

$(TEST_DIR)/%.o: $(CPP_DIR)/%.cpp $(SRC_HPP)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS_DEV) -fprofile-arcs -ftest-coverage -o $@ -c $<

clean:
	@rm -rf $(BUILD_DIR) $(TEST_DIR)

install: checkdirs install-ataqc install-scripts install-web

install-ataqc: $(BUILD_DIR)/ataqc
	@echo "Installing to $(PREFIX)"
	strip $(BUILD_DIR)/ataqc
	install -d -m 0755 $(PREFIX)/bin
	install -m 0755 build/ataqc $(PREFIX)/bin

install-scripts: $(SCRIPTS)
	for f in $^; do sed -e 's/{{VERSION}}/$(VERSION)/g' $$f > $(BUILD_DIR)/$$(basename $$f); done
	install -d -m 0755 $(PREFIX)/bin
	install -m 0755 $^ $(PREFIX)/bin

install-web: $(WEB_DIR)
	install -d -m 0755 $(PREFIX)/web
	cp -a $^/* $(PREFIX)/web
	find $(PREFIX)/web -type d -exec chmod 755 {} \;
	find $(PREFIX)/web -type f -exec chmod 644 {} \;

ifdef MODULEFILE_PATH

# template for environment module file
define MODULEFILE
#%Module1.0
set           version          $(VERSION)
set           app              ataqc
set           modroot          $(MODULES_ROOT)/$$app/$$version
setenv        ATAQC_MODULE     $$modroot

prepend-path  PATH             $$modroot/bin

conflict      ataqc

module-whatis "ATAC-seq QC toolkit, version $$version."
module-whatis "URL:  https://github.com/ParkerLab/ataqc"
module-whatis "Installation directory: $$modroot"
endef
export MODULEFILE

install-module: install
	mkdir -p `dirname $(MODULEFILE_PATH)`
	echo "$$MODULEFILE" > $(MODULEFILE_PATH)

endif
