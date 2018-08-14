
#include <cstddef>
#include <cassert>
#include <algorithm>
#include <iostream>
#include <sstream>
#include <stack>
#include <regex>

#include <boost/core/demangle.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>

#include "tables.hh"
#include "hdf5_util.hh"

using namespace tables;


//-------------------------------------
//
// bindings
//
//-------------------------------------

output_binding::output_binding(output_file* f, output_table* t)
: file(f), table(t), 
	in_file_list(f->tables.insert(f->tables.end(),this)),
	in_table_list(t->files.insert(t->files.end(), this)),
	enabled(true)
{ }

void output_binding::unbind_all(output_binding::list& L)
{
	while(! L.empty()) {
		output_binding* b = L.front();
		delete b;
	}
}

output_binding::~output_binding()
{
	file->tables.erase(in_file_list);
	table->files.erase(in_table_list);
}


output_binding* output_binding::find(list& L, output_file* f)
{
	auto found = std::find_if(L.begin(), L.end(), 
		[=](output_binding* b){ return b->file==f; });
	if(found==L.end())
		return nullptr;
	else
		return *found;
}

output_binding* output_binding::find(list& L, output_table* t)
{
	auto found = std::find_if(L.begin(), L.end(), 
		[=](output_binding* b){ return b->table==t; });
	if(found==L.end())
		return nullptr;
	else
		return *found;
}


//--------------------------------------------
//
// Column items 
// 
//--------------------------------------------


column_item::column_item(column_group* p, const string& n) 
: _parent(nullptr), _name(n)
{ 
	if(_name.empty())
		throw std::runtime_error("Column items cannot have empty name");
	if(p)
		p->add_item(this);
}	

column_item::~column_item()
{ 
	if(_parent) {
		_parent->remove_item(this);
	}	
}

void column_item::_check_unlocked()
{
	output_table* owner = table();
	if(owner && owner->is_locked()) 
		throw std::logic_error("cannot modify item owned by locked output_table");
}

output_table* column_item::table() 
{
	return (_parent == nullptr) ? nullptr : _parent->table();
}


void column_item::visit(const std::function<void(column_item*)>& f)
{
	f(this);
}

static void output_name(std::ostream& stream, column_item* item, size_t level, const string& sep)
{
	if(item->parent() && !item->parent()->is_table())
		output_name(stream, item->parent(), level+1, sep);
	stream << item->name();
	if(level>0)
		stream << sep;
}

string column_item::path_name(const string& sep)
{
	std::ostringstream s;
	output_name(s, this, 0, sep);
	return s.str();
}


//----------------------------------------------
//
// Column groups
//
//----------------------------------------------



column_group::column_group(column_group* p, const string& n)
	: column_item(p,n), _dirty(false)
{ }	

column_group::~column_group()
{
	_check_unlocked();
	for(auto c : _children) {
		// dissociate with column
		if(c) remove_item(c);
	}	
}

void column_group::_mark_dirty()
{
	if(_dirty)
		return;
	if(_parent)
		_parent->_mark_dirty();
	_dirty = true;	
}


void column_group::_mark_dirty_columns()
{
	output_table* tab = table();
	if(tab)
		tab->_dirty_columns = true;
}

void column_group::_cleanup()
{
	if(!_dirty) return; 	// we are clean
	size_t pos=0;
	for(size_t i=0; i< _children.size(); i++) {
		// loop invariants: 
		//  (1) pos <= i
		//  (2) the range [0:pos) contains nonnulls
		//  (3) every initial nonnull in [0:i) is in [0:pos)

		if(_children[i]) {
			// move current child to pos if needed
			if(pos < i) {
				assert(_children[pos]==nullptr); 
				_children[pos] = _children[i];

				assert(_children[pos]->_index == i); 
				_children[pos]->_index = pos; 
			}
			// clean up a child that is a group
			column_group* _child = dynamic_cast<column_group*>(_children[pos]);
			if(_child)
				_child->_cleanup();
			// advance pos
			pos++;
		}
	}
	assert(pos <= _children.size()); // since we were dirty!
	if(pos < _children.size())
		_children.resize(pos);		// prune the vector
	_dirty = false; 			// we are clean!
}


void column_group::add_item(column_item* col)
{
	// col must not br a table!
	if(col->is_table())
		throw std::runtime_error("Cannot add a table to a group");

	_check_unlocked();
	//_cleanup();

	if(col->_parent)
		throw std::runtime_error("column already added to a table");
	if(_item_names.count(col->name())!=0) {
		throw std::runtime_error(std::string("a column item by this name already exists: ")+col->name());
	}
	col->_parent = this;
	col->_index = _children.size();
	_children.push_back(col);
	_item_names[col->name()] = col;
	_mark_dirty_columns();
}


void column_group::remove_item(column_item* col)
{
	_check_unlocked();
	if(col->_parent != this) 
		throw std::invalid_argument(
			"column_group::remove(col) column not bound to this table");
	assert(_children[col->_index]==col);
	_children[col->_index] = nullptr;
	_item_names.erase(col->name());
	assert(_item_names.find(col->name())==_item_names.end());
	col->_parent = nullptr;
	_mark_dirty();
}


void column_group::visit(column_item::visitor f)
{
	f(this);
	for(auto c : _children)
		if(c!=nullptr)
			c->visit(f);
}


column_item* column_group::get_item(const std::vector<string>& inames)
{
	column_item* ret=this;
	for(const string&  iname : inames) {
		column_group* grp = dynamic_cast<column_group*>(ret);
		if(! grp)
			throw std::runtime_error("item not found");
		ret = grp->_item_names.at(iname);
	}
	assert(ret != nullptr);
	return ret;	
}

column_item* column_group::get_item(const string& path)
{
	std::vector<string> inames;
	boost::split(inames, path, boost::is_any_of("/"));
	return get_item(inames);
}


const std::vector<column_item*>& column_group::items()
{
	_cleanup();
	return _children;
}


void column_group::add(basic_column& col)
{
	add_item(&col);
}

void column_group::remove(basic_column& col)
{
	remove_item(&col);
}


void column_group::add(columns& cols)
{
	add_item(&cols);
}

void column_group::remove(columns& cols)
{
	remove_item(&cols);
}

void column_group::add(column_item_list cl)
{
	for(auto& c: cl) add_item(c);
}

void column_group::remove(column_item_list cl)
{
	for(auto& c: cl) remove_item(c);
}




//-------------------------------------
//
// basic_column
//
//-------------------------------------


basic_column::basic_column(column_group* _grp, const string& _name, 
	const string& f, 
	const type_info& _t, size_t _s, size_t _a)
: 	column_item(_grp, _name), 
	_format(f), 
	_type(_t), _size(_s), _align(_a)
 { }


basic_column::~basic_column()
{
}

void basic_column::set(double val)
{
	using namespace std::string_literals;
	throw std::invalid_argument("wrong column type: "s+name()
		+" is not arithmetic");
}

void basic_column::set(const string&)
{
	using namespace std::string_literals;
	throw std::invalid_argument("wrong column type"+name()
		+" is not textual");
}



//-------------------------------------
//
// Tables (result_table + timeseries)
//
//-------------------------------------

static std::unordered_map<string, output_table*>& __table_registry()
{
	static std::unordered_map<string, output_table*> foo;
	return foo;
}

static std::unordered_set<output_table*>& __all_tables()
{
	static std::unordered_set<output_table*> foo;
	return foo;
}

output_table* output_table::get(const string& name)
{
	auto iter = __table_registry().find(name);
	return (iter!=__table_registry().end()) ? iter->second : nullptr;
}


const output_table::registry& output_table::all()
{
	return __all_tables();
}


output_table::output_table(const string& _name, table_flavor _f)
: column_group(nullptr, _name), _dirty_columns(false), en(true), _locked(false), _flavor(_f)
{
	if(__table_registry().count(_name)>0)
		throw std::runtime_error("A table of name `"+_name+"' is already registered");
	__table_registry()[_name] = this;
	__all_tables().insert(this);	
}


output_table::~output_table()
{
	output_binding::unbind_all(files);
	__table_registry().erase(this->name());
	__all_tables().erase(this);
}


output_table* output_table::table() 
{
	return this;
}


void output_table::_cleanup()
{
	if(_dirty) {
		_dirty_columns = true; // trigger the recomputation of columns
		column_group::_cleanup();
	}
	assert(! _dirty);
	if(_dirty_columns) {
		columns.clear();

		visit([&](column_item* item) {
			basic_column* col = dynamic_cast<basic_column*>(item);
			if(col)
				columns.push_back(col);
		});

		_dirty_columns = false;
	}
}



basic_column* output_table::operator[](const string& n)
{
	column_item* retitem = get_item(n);
	basic_column* ret = dynamic_cast<basic_column*>(retitem);
	if(ret==nullptr)
		throw std::out_of_range("column not in table");
	return ret;
}



void output_table::emit_row()
{
	if(files.empty())
		return;
	if(!_locked)
		throw std::logic_error("prolog() has not been called before emit_row()");
	// is the table enabled?
	if(!en) return;
	// ok, we are enabled
	for(auto b : bindings())
		if(b->enabled){
			// for every enabled binding
			b->file->output_row(*this);
		}
}

void output_table::prolog()
{
	// repack the table after possible column removals
	_cleanup();

	// do this for every bound file, enabled or not
	for(auto b : bindings())
		b->file->output_prolog(*this);

	// we are ready for business
	_locked = true;
}

void output_table::epilog()
{
	// changes are allowed
	_locked = false;

	// do this for every bound file, enabled or not
	for(auto b : bindings())
		b->file->output_epilog(*this);
}


void output_table::generate_schema(std::ostream& out)
{
	using std::endl;
	// output top-level
	out << "{" << endl;
	out << "\t\"name\": \"" << name() << "\"," << endl;
	out << "\t\"columns\": [";

	// output columns
	for(size_t i=0; i<size(); i++) {
		out << "\t\t{" << endl;

		basic_column* col = (*this)[i];
		out << "\t\t\t\"name\": \"" << col->path_name() << "\"," << endl;

		out << "\t\t\t\"path\": [";
		std::stack<column_item*> stk;
		column_item* item = col;
		while(item!=this) { stk.push(item); item=item->parent(); }
		while(! stk.empty()) {
			out << "\"" << stk.top()->name() << "\"";
			stk.pop();
			if(!stk.empty())
				out << ",";
		}
		out << "]," << endl;

		out << "\t\t\t\"type\": \"" << boost::core::demangle(col->type().name()) << "\"," << endl;
		out << "\t\t\t\"arithmetic\": " << (col->is_arithmetic() ? "true" : "false") << endl;

		out << "\t\t}";
		if( (i+1)<size() ) out << ",";
		out << endl;
	}

	out << "\t]" << endl;
	out << "}" << endl;
}


result_table::result_table(const string& _name)
	: output_table(_name, table_flavor::RESULTS)
{
	//using std::cerr;
	//using std::endl;
	//cerr << "result table " << name() << " created"<< endl;
}

result_table::result_table(const string& _name,
		column_item_list col)
	: output_table(_name, table_flavor::RESULTS)
{
	add(col);

	//using std::cerr;
	//using std::endl;
	//cerr << "result table " << name() << " created"<< endl;
}


result_table::~result_table()
{
	using std::cerr;
	using std::endl;
	//cerr << "result table " << name() << " destroyed"<< endl;
}


output_file::output_file()
{ }

output_file::~output_file()
{
	output_binding::unbind_all(tables);
}




#define RE_FNAME "[a-zA-X0-9 _:\'\\.\\-\\$]+"
#define RE_PATH "(/?(?:" RE_FNAME "/)*(?:" RE_FNAME "))"
#define RE_ID   "[a-zA-Z_][a-zA-Z0-9_]*"
#define RE_TYPE "(" RE_ID  "):"
#define RE_VAR  RE_ID "=" RE_PATH
#define RE_VARS RE_VAR "(?:," RE_VAR ")*" 
#define RE_URL  RE_TYPE RE_PATH "?(?:\\?(" RE_VARS "))?" 

bool tables::parse_url(const string& url, string& type, string& path, varmap& vars)
{
	using std::regex;
	using std::smatch;
	using std::regex_match;
	using std::sregex_token_iterator;
	
	regex re_url(RE_URL);
	smatch match;
	
	if(! regex_match(url, match, re_url))
		return false;

	type = match[1];
	path = match[2];
	string allvars = match[3];

	// split variables
	regex re_var( "(" RE_ID ")=(" RE_PATH ")" );
	regex re_comma(",");
	auto s = sregex_token_iterator(allvars.begin(), allvars.end(), re_comma, -1);
	auto s2 = sregex_token_iterator();
	for(; s!=s2; ++s) {
		string var =*s;
		smatch vmatch;
		regex_match(var, vmatch, re_var);
		string vname = vmatch[1];
		string vvalue = vmatch[2];
		if(! vname.empty()) vars[vname] = vvalue; 
	}

	return true;
}




//
// Helper namespace for open_file
//
namespace {

std::unordered_map<string, open_mode> open_mode_map {
	{"append", open_mode::append},
	{"truncate", open_mode::truncate}
};

std::unordered_map<string, text_format> text_format_map {
        { "cvstab", text_format::csvtab},
        { "csvrel", text_format::csvrel}
};

template <typename T>
T proc_enum_var(const string& var, 
		const std::map<string, string>& vars, 
		const std::unordered_map<string, T>& valmap, 
		T defval)
{
	if(vars.count(var)>0) {
		const string& val = vars.at(var);
		if(valmap.count(val)==0)
			throw std::runtime_error("Illegal value in URL: "+var+"="+val);
		return valmap.at(val);
	} else {
		return defval;
	}
}

}


output_file* tables::open_file(const string& url)
{
	string type;
	string path;
	varmap vars;

    if(! parse_url(url, type, path, vars))
		throw std::runtime_error("Malformed url `"+url+"'");

	open_mode   mode = proc_enum_var("open_mode", vars, open_mode_map, default_open_mode);
	text_format format = proc_enum_var("format", vars, text_format_map, default_text_format);

    if(type == "file")
        return new output_c_file(path, mode, format);
    else if (type == "hdf5")
        return new output_hdf5(path, mode);
	else if (type == "stdout")
        return &output_stdout;
    else if (type == "stderr")
        return &output_stderr;
    throw std::runtime_error("Unknown output_file type in URL: `"+type+"'");
}



//-------------------------------------
//
// C (STDIO) files
//
//-------------------------------------

//-------------------------------------
//
// Formatters
//
//-------------------------------------


formatter::formatter(output_c_file* of, output_table& tab)
	: ofile(of), table(tab)
{ }

formatter::~formatter()
{ }


struct csvtab_formatter : formatter
{
	using formatter::formatter;
	void prolog() override;
	void row() override;
	void epilog() override;
};

void csvtab_formatter::prolog() 
{
	/*
		Logic for prolog:
		When the file is seekable, check if we are at the beginning
		before we output a header.
	 */
	long int fpos = ftell(ofile->file());
	if(fpos == -1 || fpos == 0) {
		for(size_t col=0;col < table.size(); col++) {
			if(col) fputs(",", ofile->file());
			fputs(table[col]->name().c_str(), ofile->file());
		}
		fputs("\n", ofile->file());
	} 
}

void csvtab_formatter::row() 
{
	for(size_t col=0;col < table.size(); col++) {
		if(col) fputs(",", ofile->file());
		table[col]->emit(ofile->file());
	}
	fputs("\n", ofile->file());	
}

void csvtab_formatter::epilog() 
{ }


struct csvrel_formatter : formatter
{
	using formatter::formatter;

	void prolog() override { }

	void row() override {
		fputs(table.name().c_str(), ofile->file());
		for(size_t col=0;col < table.size(); col++) {
			fputs(",", ofile->file());
			table[col]->emit(ofile->file());
		}
		fputs("\n", ofile->file());	
	}

	void epilog() override { }
};



formatter* formatter::create(output_c_file* f, output_table& t, text_format fmt)
{
	switch(fmt)
	{
	case text_format::csvtab:
		return new csvtab_formatter(f,t);
	case text_format::csvrel:
		return new csvrel_formatter(f,t);
	default:
		throw std::runtime_error("Unhandled text format");
	}
}

void formatter::destroy(formatter* fmt)
{
	delete fmt;
}

//-------------------------------
//
// A C-style file
//
//-------------------------------

void output_c_file::open(const string& fpath, open_mode mode)
{
	if(stream) 
		throw std::runtime_error("output file already open");
	stream = fopen(fpath.c_str(), (mode==open_mode::append?"a":"w"));
	if(!stream) 
		throw std::runtime_error("I/O error opening file");
	filepath = fpath;
	owner = true;
}

void output_c_file::open(FILE* _file, bool _owner)
{
	if(stream) 
		throw std::runtime_error("output file already open");
	stream = _file;
	owner = _owner;
}

void output_c_file::close()
{
	// handle the stream
	if((!stream)) return;
	if(owner) {
		if(fclose(stream)!=0)
			throw std::runtime_error("I/O error closing file");		
	} else {
		flush();
	}
	stream = nullptr;
	owner = false;
	filepath = string();
}

void output_c_file::flush()
{
	if(!stream)
		throw std::runtime_error("I/O error flushing closed file");
	if(fflush(stream)!=0)
		throw std::runtime_error("I/O error flushing file");
}


output_c_file::output_c_file(FILE* _stream, bool _owner, text_format f)
: stream(_stream), owner(_owner), fmt(f) { }


output_c_file::output_c_file(const string& _fpath, open_mode mode, text_format f)
: output_c_file(f)
{
	open(_fpath, mode);
}


output_c_file::~output_c_file()
{
	close();
	// remove any open formatters
	for(auto&& f : fmtr)
		formatter::destroy(f.second);
}


void output_c_file::output_prolog(output_table& table)
{
	// Create formatter for table
	if(fmtr.count(&table)>0) return;
	auto form = fmtr[&table] = formatter::create(this, table, fmt);
	form->prolog();
}

void output_c_file::output_row(output_table& table)
{
	fmtr.at(&table)->row();
}

void output_c_file::output_epilog(output_table& table)
{ 
	auto form = fmtr.at(&table);
	form->epilog();

	fmtr.erase(&table);
	formatter::destroy(form);
}




output_c_file tables::output_stdout(stdout, false);
output_c_file tables::output_stderr(stderr, false);


//-------------------------------------
//
// A memory file 
//
//-------------------------------------


output_mem_file::output_mem_file(text_format fmt)
	: output_c_file(fmt), state(0)
{
	state = new memstate();
	state->buffer = nullptr;
	state->len = 0;
	FILE* f = open_memstream(&state->buffer, &state->len);
	open(f, true);
}

output_mem_file::~output_mem_file()
{
	if(stream != nullptr) 
		close();
	if(state) {
		free(state->buffer);
		delete state;
	}

}

const char* output_mem_file::contents()
{
	fflush(stream);
	return state->buffer;
}

string output_mem_file::str()
{
	return string(contents());
}



//-------------------------------------
//
// Progress bar
//
//-------------------------------------



progress_bar::progress_bar(FILE* s, size_t b, const string& msg)
		: stream(s), message(msg), 
		N(0), i(0), ni(0), B(b), l(0), finished(false)
		{ }

void progress_bar::start(unsigned long long _N)
{	
	N = _N;
	i=0; ni=0; l=0;

	ni = nexti();
	size_t spc = B+1+message.size();
	for(size_t j=0; j< spc; j++) putchar(' ');
	fprintf(stream, "]\r%s[", message.c_str());
	fflush(stream);
	tick(0);
}

void progress_bar::adjustBar()
{
	if(i>N) i=N;
	while(i >= ni)  {
		++l; ni = nexti();
		if(l<=B) { fputc('#', stream); }
	}
	fflush(stream);
	if(l==B) {
		fprintf(stream,"\n");
		finished = true;
	}
}

void progress_bar::finish()
{
	if(finished) return;
	if(i<N)
		tick(N-i);
}


//-------------------------------------
//
// HDF5 files
//
//-------------------------------------



/*
 *
 * This is the code for the table_handler
 *
 */



std::map<type_index, H5::DataType> __pred_type_map = 
{
	{typeid(bool), H5::PredType::NATIVE_UCHAR},

	{typeid(char), H5::PredType::NATIVE_CHAR},
	{typeid(signed char), H5::PredType::NATIVE_SCHAR},	
	{typeid(short), H5::PredType::NATIVE_SHORT},
	{typeid(int), H5::PredType::NATIVE_INT},
	{typeid(long), H5::PredType::NATIVE_LONG},
	{typeid(long long), H5::PredType::NATIVE_LLONG},

	{typeid(unsigned char), H5::PredType::NATIVE_UCHAR},
	{typeid(unsigned short), H5::PredType::NATIVE_USHORT},
	{typeid(unsigned int), H5::PredType::NATIVE_UINT},
	{typeid(unsigned long), H5::PredType::NATIVE_ULONG},
	{typeid(unsigned long long), H5::PredType::NATIVE_ULLONG},

	{typeid(float), H5::PredType::NATIVE_FLOAT},
	{typeid(double), H5::PredType::NATIVE_DOUBLE},
	{typeid(long double), H5::PredType::NATIVE_LDOUBLE}
};


H5::DataType hdf_mapped_type(basic_column* col)
{
	// make it simple-minded for now
	using namespace std::string_literals;
	auto predit = __pred_type_map.find(col->type());
	if(predit!=__pred_type_map.end()) 
		return predit->second;
	else if(col->type() == typeid(string)) {
		return H5::StrType(0, col->size());
	} else {
		throw std::logic_error("HDF5 mapping for type '"s+
			col->type().name()+"' not known"s);
	}
}


inline static size_t __aligned(size_t pos, size_t al) {
	assert( (al&(al-1)) == 0); // al is a power of 2
	return al*((pos+al-1)/al);
}

void output_hdf5::table_handler::make_row(char* buffer) 
{
	size_t pos = 0;
	for(size_t i=0; i< table.size(); i++) {
		if(i>0) pos = __aligned(pos+table[i-1]->size(), table[i]->align());
		assert(pos == colpos[i]);
		table[i]->copy(buffer + pos);
	}
}

output_hdf5::table_handler::table_handler(output_table& _table) 
: table(_table), colpos(table.size(),0), size(0), align(1)
{
	// first compute the size of the whole
	// thing
	for(size_t i=0;i<table.size();i++) {
		align = std::max(align, table[i]->align());
		if(i>0) size = __aligned(size, table[i]->align());
		size += table[i]->size();
	}
	if(table.size()>0) size = __aligned(size, table[0]->align());
	// now, compute the type
	size_t pos = 0;
	type = H5::CompType(size);
	for(size_t i=0;i<table.size();i++) {
		basic_column* c = table[i];
		if(i>0) pos = __aligned(pos+table[i-1]->size(), table[i]->align());
		colpos[i] = pos;
		type.insertMember(c->name(), pos, hdf_mapped_type(c));
	}
}

void output_hdf5::table_handler::create_dataset(const H5::Group& loc)
{
	using namespace H5;
	// it does not! create it
	hsize_t zdim[] = { 0 };
	hsize_t cdim[] = { 16 };
	hsize_t mdim[] = { H5S_UNLIMITED };
	DataSpace dspace(1, zdim, mdim);
	DSetCreatPropList props;
	props.setChunk(1, cdim);

	dataset = loc.createDataSet(table.name(), 
			type, dspace, props);		
}

void output_hdf5::table_handler::append_row()
{
	using namespace H5;
	// Make the image of an object.
	// This need not be aligned as far as I can tell!!!!!
	char buffer[size];
	memset(buffer,0,size); // this should silence valgrind
	make_row(buffer);

	/*
	Note: the following function is not yet supported in the 
	hdf5 version of Ubuntu :-(

	H5DOappend(dataset.getId(), H5P_DEFAULT, 0, 1, type, buffer);

	So, we must do the append "manually"
	*/

	// extend the dataset by one row
	DataSpace tabspc = dataset.getSpace();
	assert(tabspc.getSimpleExtentNdims()==1);
	hsize_t ext[1];
	tabspc.getSimpleExtentDims(ext);
	ext[0]++;
	dataset.extend(ext);

	// create table space
	tabspc = dataset.getSpace();
	hsize_t dext[] = { 1 };
	ext[0]--;  // use the old extent
	tabspc.selectHyperslab(H5S_SELECT_SET, dext, ext);
	DataSpace memspc;  // scalar dataspace

	dataset.write(buffer, type, memspc, tabspc);
}

output_hdf5::table_handler::~table_handler() 
{
	dataset.close();
}


/*
 *
 * This is the code for the main object  
 *
 */

output_hdf5::~output_hdf5()
{
	H5_CHECK(H5Idec_ref(locid));
}


output_hdf5::output_hdf5(long int _locid, open_mode _mode)
: locid(_locid), mode(_mode)
{
	H5_CHECK(H5Iinc_ref(locid));
}

output_hdf5::output_hdf5(const H5::Group& _group, open_mode _mode)
: output_hdf5(_group.getId(), _mode)
{  }


output_hdf5::output_hdf5(const H5::H5File& _file, open_mode _mode)
: output_hdf5(_file.openGroup("/"), _mode)
{  }


output_hdf5::output_hdf5(const string& h5file, open_mode mode)
: output_hdf5(H5::H5File(h5file, H5F_ACC_TRUNC), mode)
{ }


output_hdf5::table_handler* output_hdf5::handler(output_table& table)
{
	auto it = _handler.find(&table);
	if(it==_handler.end()) {
		table_handler* sc = new table_handler(table);
		_handler[&table] = sc;
		return sc;
	} else
		return it->second;
}



void output_hdf5::output_prolog(output_table& table)
{
	using namespace H5;

	// construct the table or timeseries
	Group loc(locid);
	table_handler* th = handler(table);
	
	// check the open mode and work accordingly
	if(this->mode == open_mode::append) {
		// check if an object by the given name exists in the loc
		if(hdf5_exists(locid, table.name())) {
			DataSet dset = loc.openDataSet(table.name());

			// ok, the dataset exists, just check compatibility
			hid_t dset_type = H5_CHECK(H5Dget_type(dset.getId()));

			if(! (th->type == DataType(dset_type)))
				throw std::runtime_error("On appending to HDF table,"\
					" types are not compatible");

			th->dataset = dset;

		} else {
			th->create_dataset(loc);
		}
	} else {
		assert(mode==open_mode::truncate);
		// maybe it exists, erase it
		if(hdf5_exists(locid, table.name())) {
			loc.unlink(table.name());
		}
		th->create_dataset(loc);
	}


}


void output_hdf5::output_row(output_table& table)
{	
	using namespace H5;
	table_handler* th = handler(table);
	th->append_row();

}


void output_hdf5::output_epilog(output_table& table)
{
	// just delete the handler
	auto it = _handler.find(&table);
	if(it != _handler.end()) {
		delete it->second;
		_handler.erase(it);		
	}
}


