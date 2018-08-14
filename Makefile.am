
AUTOMAKE_OPTIONS = foreign

CXXFLAGS=
if DEBUG
AM_CXXFLAGS= -Wall -std=gnu++17 -g3
else
AM_CXXFLAGS= -Wall -std=gnu++17 -Ofast -DNDEBUG 
endif

AM_CXXFLAGS+= $(HDF5_CPPFLAGS) $(JSONCPP_CPPFLAGS)

lib_LIBRARIES= libtables.a
libtables_a_SOURCES=tables.cc

include_HEADERS= tables.hh

EXTRA_DIST= tables.hh tables_types.hh

#
# Testing
# 

TESTS= tables_tests

check_PROGRAMS= tables_tests

EXTRA_tables_tests_SOURCES= tables_tests.hh group_tests.hh 
tables_tests_SOURCES= tables_tests.cc 
tables_tests_CPPFLAGS= $(HDF5_CPPFLAGS)
tables_tests_LDADD= libtables.a $(JSONCPP_LIBS) -lhdf5_cpp -lhdf5_hl_cpp $(HDF5_LIBS) 
tables_tests_LDFLAGS= $(HDF5_LDFLAGS)

BUILT_SOURCES = tables_tests.cc
MAINTAINERCLEANFILES = tables_tests.cc

tables_tests.cc: $(EXTRA_tables_tests_SOURCES)
	@echo CXXFLAGS= $(CXXFLAGS)
	cxxtestgen --runner=ErrorPrinter -o $@ $^

# documentation
@DX_RULES@
