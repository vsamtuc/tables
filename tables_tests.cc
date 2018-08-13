/* Generated file, do not edit */

#ifndef CXXTEST_RUNNING
#define CXXTEST_RUNNING
#endif

#define _CXXTEST_HAVE_STD
#define _CXXTEST_HAVE_EH
#include <cxxtest/TestListener.h>
#include <cxxtest/TestTracker.h>
#include <cxxtest/TestRunner.h>
#include <cxxtest/RealDescriptions.h>
#include <cxxtest/TestMain.h>
#include <cxxtest/ErrorPrinter.h>

int main( int argc, char *argv[] ) {
 int status;
    CxxTest::ErrorPrinter tmp;
    CxxTest::RealWorldDescription::_worldName = "cxxtest";
    status = CxxTest::Main< CxxTest::ErrorPrinter >( tmp, argc, argv );
    return status;
}
bool suite_tables__OutputTestSuite_init = false;
#include "tables_tests.hh"

static tables::OutputTestSuite suite_tables__OutputTestSuite;

static CxxTest::List Tests_tables__OutputTestSuite = { 0, 0 };
CxxTest::StaticSuiteDescription suiteDescription_tables__OutputTestSuite( "tables_tests.hh", 47, "tables::OutputTestSuite", suite_tables__OutputTestSuite, Tests_tables__OutputTestSuite );

static class TestDescription_suite_tables__OutputTestSuite_test_column : public CxxTest::RealTestDescription {
public:
 TestDescription_suite_tables__OutputTestSuite_test_column() : CxxTest::RealTestDescription( Tests_tables__OutputTestSuite, suiteDescription_tables__OutputTestSuite, 63, "test_column" ) {}
 void runTest() { suite_tables__OutputTestSuite.test_column(); }
} testDescription_suite_tables__OutputTestSuite_test_column;

static class TestDescription_suite_tables__OutputTestSuite_test_output : public CxxTest::RealTestDescription {
public:
 TestDescription_suite_tables__OutputTestSuite_test_output() : CxxTest::RealTestDescription( Tests_tables__OutputTestSuite, suiteDescription_tables__OutputTestSuite, 72, "test_output" ) {}
 void runTest() { suite_tables__OutputTestSuite.test_output(); }
} testDescription_suite_tables__OutputTestSuite_test_output;

static class TestDescription_suite_tables__OutputTestSuite_test_output_file : public CxxTest::RealTestDescription {
public:
 TestDescription_suite_tables__OutputTestSuite_test_output_file() : CxxTest::RealTestDescription( Tests_tables__OutputTestSuite, suiteDescription_tables__OutputTestSuite, 107, "test_output_file" ) {}
 void runTest() { suite_tables__OutputTestSuite.test_output_file(); }
} testDescription_suite_tables__OutputTestSuite_test_output_file;

static class TestDescription_suite_tables__OutputTestSuite_test_bind : public CxxTest::RealTestDescription {
public:
 TestDescription_suite_tables__OutputTestSuite_test_bind() : CxxTest::RealTestDescription( Tests_tables__OutputTestSuite, suiteDescription_tables__OutputTestSuite, 142, "test_bind" ) {}
 void runTest() { suite_tables__OutputTestSuite.test_bind(); }
} testDescription_suite_tables__OutputTestSuite_test_bind;

static class TestDescription_suite_tables__OutputTestSuite_test_output_hdf5_table_handler_schema : public CxxTest::RealTestDescription {
public:
 TestDescription_suite_tables__OutputTestSuite_test_output_hdf5_table_handler_schema() : CxxTest::RealTestDescription( Tests_tables__OutputTestSuite, suiteDescription_tables__OutputTestSuite, 239, "test_output_hdf5_table_handler_schema" ) {}
 void runTest() { suite_tables__OutputTestSuite.test_output_hdf5_table_handler_schema(); }
} testDescription_suite_tables__OutputTestSuite_test_output_hdf5_table_handler_schema;

static class TestDescription_suite_tables__OutputTestSuite_test_output_hdf5_table_handler_data : public CxxTest::RealTestDescription {
public:
 TestDescription_suite_tables__OutputTestSuite_test_output_hdf5_table_handler_data() : CxxTest::RealTestDescription( Tests_tables__OutputTestSuite, suiteDescription_tables__OutputTestSuite, 303, "test_output_hdf5_table_handler_data" ) {}
 void runTest() { suite_tables__OutputTestSuite.test_output_hdf5_table_handler_data(); }
} testDescription_suite_tables__OutputTestSuite_test_output_hdf5_table_handler_data;

static class TestDescription_suite_tables__OutputTestSuite_test_output_hdf5_basic : public CxxTest::RealTestDescription {
public:
 TestDescription_suite_tables__OutputTestSuite_test_output_hdf5_basic() : CxxTest::RealTestDescription( Tests_tables__OutputTestSuite, suiteDescription_tables__OutputTestSuite, 325, "test_output_hdf5_basic" ) {}
 void runTest() { suite_tables__OutputTestSuite.test_output_hdf5_basic(); }
} testDescription_suite_tables__OutputTestSuite_test_output_hdf5_basic;

static class TestDescription_suite_tables__OutputTestSuite_test_output_hdf5_truncate : public CxxTest::RealTestDescription {
public:
 TestDescription_suite_tables__OutputTestSuite_test_output_hdf5_truncate() : CxxTest::RealTestDescription( Tests_tables__OutputTestSuite, suiteDescription_tables__OutputTestSuite, 373, "test_output_hdf5_truncate" ) {}
 void runTest() { suite_tables__OutputTestSuite.test_output_hdf5_truncate(); }
} testDescription_suite_tables__OutputTestSuite_test_output_hdf5_truncate;

static class TestDescription_suite_tables__OutputTestSuite_test_output_hdf5_append : public CxxTest::RealTestDescription {
public:
 TestDescription_suite_tables__OutputTestSuite_test_output_hdf5_append() : CxxTest::RealTestDescription( Tests_tables__OutputTestSuite, suiteDescription_tables__OutputTestSuite, 406, "test_output_hdf5_append" ) {}
 void runTest() { suite_tables__OutputTestSuite.test_output_hdf5_append(); }
} testDescription_suite_tables__OutputTestSuite_test_output_hdf5_append;

static class TestDescription_suite_tables__OutputTestSuite_test_settable : public CxxTest::RealTestDescription {
public:
 TestDescription_suite_tables__OutputTestSuite_test_settable() : CxxTest::RealTestDescription( Tests_tables__OutputTestSuite, suiteDescription_tables__OutputTestSuite, 441, "test_settable" ) {}
 void runTest() { suite_tables__OutputTestSuite.test_settable(); }
} testDescription_suite_tables__OutputTestSuite_test_settable;

static class TestDescription_suite_tables__OutputTestSuite_test_untyped : public CxxTest::RealTestDescription {
public:
 TestDescription_suite_tables__OutputTestSuite_test_untyped() : CxxTest::RealTestDescription( Tests_tables__OutputTestSuite, suiteDescription_tables__OutputTestSuite, 476, "test_untyped" ) {}
 void runTest() { suite_tables__OutputTestSuite.test_untyped(); }
} testDescription_suite_tables__OutputTestSuite_test_untyped;

static class TestDescription_suite_tables__OutputTestSuite_test_myresults : public CxxTest::RealTestDescription {
public:
 TestDescription_suite_tables__OutputTestSuite_test_myresults() : CxxTest::RealTestDescription( Tests_tables__OutputTestSuite, suiteDescription_tables__OutputTestSuite, 526, "test_myresults" ) {}
 void runTest() { suite_tables__OutputTestSuite.test_myresults(); }
} testDescription_suite_tables__OutputTestSuite_test_myresults;

#include <cxxtest/Root.cpp>
const char* CxxTest::RealWorldDescription::_worldName = "cxxtest";
