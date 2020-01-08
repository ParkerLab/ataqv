#
# VARIABLES
#

VERSION = 1.1.1

#
# PATHS
#

SRC_DIR = src
CPP_DIR = $(shell realpath -e $(SRC_DIR)/cpp)
SCRIPTS_DIR = $(SRC_DIR)/scripts
WEB_DIR = $(SRC_DIR)/web
BUILD_DIR = build
TEST_DIR = testing

SRC_CPP := $(wildcard $(CPP_DIR)/*.cpp)
SRC_HPP := $(wildcard $(CPP_DIR)/*.hpp)
INCLUDES := $(addprefix -I,$(CPP_DIR))

SCRIPTS := $(wildcard $(SCRIPTS_DIR)/*)

# where to install -- the default location assumes environment modules
# are in use, but you can simply override prefix with a command like
# 'make install prefix=...'
MODULES_ROOT = /lab/sw/modules
prefix = $(MODULES_ROOT)/ataqv/$(VERSION)

# where to install the modulefile
MODULEFILES_ROOT = /lab/sw/modulefiles
MODULEFILE_PATH = $(MODULEFILES_ROOT)/ataqv/$(VERSION)

#
# FLAGS
#

CPPFLAGS = -pedantic -Wall -Wextra -Wwrite-strings -Wstrict-overflow -fno-strict-aliasing -fPIC $(INCLUDES)
CXXFLAGS = -std=c++11 -pthread -O3 -g $(CPPFLAGS)
CXXFLAGS_DEV = -std=c++11 -pthread -O3 -g $(CPPFLAGS)
CXXFLAGS_STATIC = -std=c++11 -O3 -Wl,--whole-archive -lpthread -Wl,--no-whole-archive -static -static-libgcc -static-libstdc++ -I$(HTSLIB_STATIC_DIR) $(CPPFLAGS)

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

LDLIBS = $(BOOST_LIBS) $(HTS_LIBS) -lz -lncurses -lpthread
HTSLIB_STATIC_DIR = $(HOME)/sw/bio/htslib/htslib
LDLIBS_STATIC = -L$(HTSLIB_STATIC_DIR) $(LDLIBS) -lbz2 -llzma -ltinfo -lrt

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

.PHONY: all checkdirs clean deb deb-static rpm install install-ataqv install-module install-scripts install-web test

all: checkdirs $(BUILD_DIR)/ataqv

static: checkdirs $(BUILD_DIR)/ataqv-static

dist: checkdirs $(BUILD_DIR)/ataqv
	make install DESTDIR=$(BUILD_DIR)/ataqv-$(VERSION) prefix=
	cd $(BUILD_DIR) && tar czf ataqv-$(VERSION).$(shell uname -m).$(shell uname -s).tar.gz ataqv-$(VERSION)

dist-static: checkdirs $(BUILD_DIR)/ataqv-static
	make install DESTDIR=$(BUILD_DIR)/ataqv-$(VERSION) prefix=
	install -m 0755 $(BUILD_DIR)/ataqv-static $(BUILD_DIR)/ataqv-$(VERSION)/bin/ataqv
	cd $(BUILD_DIR) && tar czf ataqv-$(VERSION).$(shell uname -m).$(shell uname -s).tar.gz ataqv-$(VERSION)

deb:
	(cd .. && tar czf ataqv_$(VERSION).orig.tar.gz --exclude .git --exclude build ataqv)
	debuild -uc -us

deb-static: deb static
	install -m 0755 $(BUILD_DIR)/ataqv-static debian/ataqv/usr/bin/ataqv
	sed -i '/Depends: /d' debian/ataqv/DEBIAN/control
	dh_builddeb

rpm: deb
	(cd .. && alien -r ataqv_$(VERSION)-1_amd64.deb)

rpm-static: deb-static
	(cd .. && alien -r ataqv_$(VERSION)-1_amd64.deb)

checkdirs: $(BUILD_DIR) $(TEST_DIR)

$(BUILD_DIR):
	@mkdir -p $@

$(TEST_DIR):
	@mkdir -p $@

$(BUILD_DIR)/ataqv: $(BUILD_DIR)/ataqv.o $(BUILD_DIR)/Features.o $(BUILD_DIR)/HTS.o $(BUILD_DIR)/IO.o $(BUILD_DIR)/Metrics.o $(BUILD_DIR)/Peaks.o $(BUILD_DIR)/Utils.o
	$(CXX) -o $@ $^ $(LDFLAGS) $(LDLIBS)

$(BUILD_DIR)/ataqv-static: $(CPP_DIR)/ataqv.cpp $(CPP_DIR)/Features.cpp $(CPP_DIR)/HTS.cpp $(CPP_DIR)/IO.cpp $(CPP_DIR)/Metrics.cpp $(CPP_DIR)/Peaks.cpp $(CPP_DIR)/Utils.cpp
	$(CXX) -o $@ $^ $(CXXFLAGS_STATIC) $(LDFLAGS) $(LDLIBS_STATIC)

$(BUILD_DIR)/%.o: $(CPP_DIR)/%.cpp $(SRC_HPP) $(CPP_DIR)/Version.hpp
	$(CXX) $(CXXFLAGS) -o $@ -c $<

$(CPP_DIR)/Version.hpp: Makefile
	@echo '//' >$@
	@echo '// Copyright 2015 Stephen Parker' >>$@
	@echo '//' >>$@
	@echo '// Licensed under Version 3 of the GPL or any later version' >>$@
	@echo '//' >>$@
	@echo >>$@
	@echo '#define VERSION "$(VERSION)"' >>$@

test: checkdirs $(CPP_DIR)/Version.hpp $(TEST_DIR)/run_ataqv_tests
	@cp testdata/* $(TEST_DIR)
	@cd $(TEST_DIR) && ./run_ataqv_tests -i
	@cd $(TEST_DIR) && lcov --no-external --quiet --capture --derive-func-data --directory $(CPP_DIR) --directory . --output-file ataqv.info && lcov --remove ataqv.info $(CPP_DIR)/catch.hpp --output-file ataqv.info && lcov --remove ataqv.info $(CPP_DIR)/json.hpp --output-file ataqv.info && genhtml ataqv.info -o ataqv

$(TEST_DIR)/run_ataqv_tests: $(TEST_DIR)/run_ataqv_tests.o $(TEST_DIR)/test_features.o $(TEST_DIR)/test_hts.o $(TEST_DIR)/test_io.o $(TEST_DIR)/test_metrics.o $(TEST_DIR)/test_peaks.o $(TEST_DIR)/test_utils.o $(TEST_DIR)/Features.o $(TEST_DIR)/HTS.o $(TEST_DIR)/IO.o $(TEST_DIR)/Metrics.o $(TEST_DIR)/Peaks.o $(TEST_DIR)/Utils.o
	$(CXX) -o $@ $^ $(LDFLAGS) --coverage $(LDLIBS)

$(TEST_DIR)/%.o: $(CPP_DIR)/%.cpp $(SRC_HPP)
	$(CXX)  $(CXXFLAGS_DEV) -fprofile-arcs -ftest-coverage -o $@ -c $<

clean:
	@rm -rf $(BUILD_DIR) $(TEST_DIR ) $(CPP_DIR)/Version.hpp
	@test -x dh_clean && dh_clean || true

install: checkdirs install-ataqv install-scripts install-web

install-ataqv: $(BUILD_DIR)/ataqv
	@echo "Installing to $(prefix)"
	strip $(BUILD_DIR)/ataqv
	install -d -m 0755 $(DESTDIR)$(prefix)/bin
	install -m 0755 build/ataqv $(DESTDIR)$(prefix)/bin

install-scripts: $(SCRIPTS)
	install -d -m 0755 $(DESTDIR)$(prefix)/bin
	for f in $^; do sed -e 's/{{VERSION}}/$(VERSION)/g' $$f > $(BUILD_DIR)/$$(basename $$f); install -m 0755 $(BUILD_DIR)/$$(basename $$f) $(DESTDIR)$(prefix)/bin; done

install-web: $(WEB_DIR)
	install -d -m 0755 $(DESTDIR)$(prefix)/share/ataqv/web
	cp -a $^/* $(DESTDIR)$(prefix)/share/ataqv/web
	find $(DESTDIR)$(prefix)/share/ataqv/web -type d -exec chmod 755 {} \;
	find $(DESTDIR)$(prefix)/share/ataqv/web -type f -exec chmod 644 {} \;

ifdef MODULEFILE_PATH

# template for environment module file
define MODULEFILE
#%Module1.0
set           version          $(VERSION)
set           app              ataqv
set           modroot          $(MODULES_ROOT)/$$app/$$version
setenv        ATAQV_MODULE     $$modroot

prepend-path  PATH             $$modroot/bin

conflict      ataqv

module-whatis "ATAC-seq QC toolkit, version $$version."
module-whatis "URL:  https://github.com/ParkerLab/ataqv"
module-whatis "Installation directory: $$modroot"
endef
export MODULEFILE

install-module: install
	mkdir -p `dirname $(DESTDIR)$(MODULEFILE_PATH)`
	echo "$$MODULEFILE" > $(DESTDIR)$(MODULEFILE_PATH)

endif
