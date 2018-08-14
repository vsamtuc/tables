#pragma once

#include <sstream>
#include <cxxtest/TestSuite.h>
#include <jsoncpp/json/json.h>

#include "tables.hh"
#include "hdf5_util.hh"


using namespace tables;

struct table_mixin3 : columns
{
	column<size_t>  foo  { "foo", "%zu" };
	column<string>  bar  { "bar", 32, "%s" };
	table_mixin3(const string& n="grp", column_group* host = nullptr) 
		: columns(n)
	{
		add({&foo, &bar});
		if(host)
			host->add_item(this);
	}
};

class GroupTestSuite : public CxxTest::TestSuite
{
public:

	void test_columns_constructor1()
	{
		columns cg("foo");

		TS_ASSERT_EQUALS(cg.name(), "foo");
		TS_ASSERT_EQUALS(cg.parent(), nullptr);
		TS_ASSERT_EQUALS(cg.table(), nullptr);
		TS_ASSERT_EQUALS(cg.items().size(), 0);
	}

	void test_columns_constructor2()
	{
		columns par("p");
		columns cg(par, "foo");

		TS_ASSERT_EQUALS(cg.name(), "foo");
		TS_ASSERT_EQUALS(cg.parent(), &par);
		TS_ASSERT_EQUALS(cg.table(), nullptr);
		TS_ASSERT_EQUALS(cg.items().size(), 0);
		TS_ASSERT_EQUALS(par.items().size(), 1);

	}

	void test_table_member()
	{
		columns par("p");
		columns cg(par, "foo");

		TS_ASSERT_EQUALS(cg.table(), nullptr);
		TS_ASSERT_EQUALS(par.table(), nullptr);
		{
			result_table tab("foo");
			tab.add(par);

			TS_ASSERT_EQUALS(cg.table(), &tab);
			TS_ASSERT_EQUALS(par.table(), &tab);
		}

		TS_ASSERT_EQUALS(cg.table(), nullptr);
		TS_ASSERT_EQUALS(par.table(), nullptr);
	}

	void test_is_methods()
	{
		column<int> col("foo", "%d");

		TS_ASSERT(col.is_column());
		TS_ASSERT(!col.is_columns());
		TS_ASSERT(!col.is_table());

		columns cols("bar");
		TS_ASSERT(!cols.is_column());
		TS_ASSERT(cols.is_columns());
		TS_ASSERT(!cols.is_table());

		result_table tab("tab");
		TS_ASSERT(!tab.is_column());
		TS_ASSERT(!tab.is_columns());
		TS_ASSERT(tab.is_table());
	}

	void test_visitor()
	{
		columns c1("foo");
		table_mixin3 grp("grp", &c1);

		typedef std::vector<column_item*> items_t;
		items_t items;

		auto collector = [&](column_item* item) {
			items.push_back(item);
		};

		grp.foo.visit(collector);
		TS_ASSERT_EQUALS(items, items_t {& grp.foo} );

		items.clear();
		grp.visit(collector);
		TS_ASSERT_EQUALS(items, (items_t {&grp, &grp.foo, &grp.bar}) );

		items.clear();
		c1.visit(collector);
		TS_ASSERT_EQUALS(items, (items_t {&c1, &grp, &grp.foo, &grp.bar}) );

		columns c2(c1, "bar2");
		columns c3(c1, "bar3");
		c1.remove(c2);

		items.clear();
		c1.visit(collector);
		TS_ASSERT_EQUALS(items, (items_t {&c1, &grp, &grp.foo, &grp.bar, &c3}) );
	}

	void test_cleanup()
	{
		result_table tab("tab");

		columns c1(tab, "foo");
		table_mixin3 grp("grp", &c1);

		columns c2(c1, "bar2");
		table_mixin3 grp2("grp", &c2);

		columns c3(c1, "bar3");

		TS_ASSERT_EQUALS(tab.size(), 4);

		c1.remove(c2);
		TS_ASSERT_EQUALS(tab.size(), 2);
	}

	void test_get_item()
	{
		result_table tab("tab");

		columns c1(tab, "foo");
		table_mixin3 grp("grp", &c1);

		columns c2(c1, "bar2");
		table_mixin3 grp2("grp", &c2);

		columns c3(c1, "bar3");

		TS_ASSERT_EQUALS(tab.get_item("foo"), &c1);
		TS_ASSERT_EQUALS(tab.get_item("foo/grp"), &grp);
		TS_ASSERT_EQUALS(tab.get_item("foo/grp/foo"), &grp.foo);
		TS_ASSERT_EQUALS(tab.get_item("foo/grp/bar"), &grp.bar);

		TS_ASSERT_EQUALS(tab.get_item("foo/bar2/grp"), &grp2);
		TS_ASSERT_EQUALS(tab.get_item("foo/bar2/grp/foo"), &grp2.foo);
		TS_ASSERT_EQUALS(tab.get_item("foo/bar2/grp/bar"), &grp2.bar);

		TS_ASSERT_EQUALS(tab.get_item("foo/bar3"), &c3);		
	}

	void test_path_name()
	{
		result_table tab("tab");

		columns c1(tab, "foo");
		table_mixin3 grp("grp", &c1);

		columns c2(c1, "bar2");
		table_mixin3 grp2("grp", &c2);

		columns c3(c1, "bar3");

		TS_ASSERT_EQUALS(string("foo"), c1.path_name());
		TS_ASSERT_EQUALS(string("foo/grp"), grp.path_name());
		TS_ASSERT_EQUALS(string("foo/grp/foo"), grp.foo.path_name());
		TS_ASSERT_EQUALS(string("foo/grp/bar"), grp.bar.path_name());

		TS_ASSERT_EQUALS(string("foo/bar2/grp"), grp2.path_name());
		TS_ASSERT_EQUALS(string("foo/bar2/grp/foo"), grp2.foo.path_name());
		TS_ASSERT_EQUALS(string("foo/bar2/grp/bar"), grp2.bar.path_name());

		TS_ASSERT_EQUALS(string("foo/bar3"), c3.path_name());		

		TS_ASSERT_EQUALS(string("foo::bar2::grp::foo"), grp2.foo.path_name("::"));
	}

};