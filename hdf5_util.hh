
#pragma once

#include <string>
#include <vector>
#include <H5Cpp.h>
#include <H5LTpublic.h>
#include <typeinfo>

#include "tables.hh"

using std::string;
using namespace tables;


// A simple macro to check the result of HDF5 C-API functions
template <typename T>
inline T __H5_CHECK(T rc, const char* msg)
{
	if(rc < 0)
		throw std::runtime_error(msg);
	return rc;
}
#define H5_CHECK(cmd) (__H5_CHECK((cmd), #cmd  ))

	
//
// Return true if a dataset by the given name exists in 
// the provided location
//
inline bool hdf5_exists(hid_t locid, const string& name)
{
	//return H5Lexists(locid, name.c_str(), H5P_DEFAULT);
	return H5_CHECK(H5LTfind_dataset(locid, name.c_str()));
}

/*
	The handler type is exposed here for testing purposes
 */
struct output_hdf5::table_handler
{
	output_table& table;
	std::vector<size_t> colpos;
	size_t size;
	size_t align;
	H5::CompType type;
	H5::DataSet dataset;


	table_handler(output_table& _table);
	void make_row(char* buffer);
	void create_dataset(const H5::Group& loc);
	void append_row();
	~table_handler();
};

/*
	This table is actually defined in output.cc
 */
extern std::map<type_index, H5::DataType> __pred_type_map; 

/*
	Return the typed value of an attribute, for 
	scalar arithmetic attributes.
 */
template <typename T>
T get_value(H5::Attribute attr)
{
	using namespace H5;
	using std::vector;

	static_assert(std::is_arithmetic<T>::value, 
		"Only arithmetic types are supported");

	if(__pred_type_map.find(typeid(T)) == __pred_type_map.end()) {
		throw std::logic_error("get_value<T> called for unsupported type");
	}

	// our memory type
	DataType mtype = __pred_type_map[typeid(T)];

	// Get the space
	DataSpace dspc = attr.getSpace();
	if(dspc.isSimple()) 
		throw std::runtime_error("expected a scalar dataspace for scalar value");

	// the return buffer
	T retval;

	// do the read
	attr.read(mtype, &retval);

	// done
	return retval;
}


/*
	Return the typed value of an attribute, for 
	array arithmetic attributes.
 */
template <typename T>
std::vector<T> get_array(H5::Attribute attr)
{
	using namespace H5;
	using std::vector;

	static_assert(std::is_arithmetic<T>::value, 
		"Only arithmetic types are supported");

	if(__pred_type_map.find(typeid(T)) == __pred_type_map.end()) {
		throw std::logic_error("get_array<T> called for unsupported type");
	}

	// our memory type
	DataType mtype = __pred_type_map[typeid(T)];

	// Get the space
	DataSpace dspc = attr.getSpace();
	if(! dspc.isSimple()) 
		throw std::runtime_error("expected a simple dataspace for array");

	// the return buffer
	vector<T> retval( dspc.getSimpleExtentNpoints() );

	// do the read
	attr.read(mtype, retval.data());

	// done
	return retval;
}


