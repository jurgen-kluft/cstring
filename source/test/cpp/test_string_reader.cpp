#include "xbase/x_allocator.h"
#include "xbase/x_runes.h"
#include "xstring/x_string_reader.h"
#include "xunittest/xunittest.h"

using namespace xcore;

extern xcore::xalloc* gTestAllocator;

UNITTEST_SUITE_BEGIN(test_string_reader)
{
	UNITTEST_FIXTURE(main)
	{
		UNITTEST_FIXTURE_SETUP() 
		{
		}
		UNITTEST_FIXTURE_TEARDOWN() 
		{
		}

		UNITTEST_TEST(test_ascii)
		{
			string_reader reader("this is a string");
			CHECK_TRUE(reader.valid());
		}
		UNITTEST_TEST(test_peek)
		{
			string_reader reader("this is a string");
			CHECK_TRUE(reader.valid());
			CHECK_EQUAL('t', reader.peek());
		}
		UNITTEST_TEST(test_read)
		{
			string_reader reader("this is a string");
			CHECK_TRUE(reader.valid());
			CHECK_EQUAL('t', reader.read());
			CHECK_EQUAL('h', reader.read());
			CHECK_EQUAL('i', reader.read());
			CHECK_EQUAL('s', reader.read());
			CHECK_TRUE(reader.valid());
		}
		UNITTEST_TEST(test_skip)
		{
			string_reader reader("this is a string");
			CHECK_TRUE(reader.valid());
			reader.skip();
			reader.skip();
			reader.skip();
			reader.skip();
			reader.skip();
			CHECK_EQUAL('i', reader.read());
			CHECK_EQUAL('s', reader.read());
			CHECK_EQUAL(' ', reader.read());
			CHECK_EQUAL('a', reader.read());
			CHECK_TRUE(reader.valid());
		}
		UNITTEST_TEST(test_selection)
		{
			string_reader reader("the main string, with a substring, and the end.");
			CHECK_TRUE(reader.valid());
			for (s32 i=0; i<15; ++i)
				reader.skip();
			CHECK_EQUAL(',', reader.read());
			CHECK_EQUAL(' ', reader.read());

			string_reader substart(reader);
			for (s32 i=0; i<16; ++i)
				reader.skip();
			string_reader substr(substart, reader);

			CHECK_EQUAL(',', reader.read());

			CHECK_EQUAL('w', substr.read());
			CHECK_EQUAL('i', substr.read());
			CHECK_EQUAL('t', substr.read());
			CHECK_EQUAL('h', substr.read());
			CHECK_EQUAL(' ', substr.read());
			CHECK_EQUAL('a', substr.read());
			CHECK_EQUAL(' ', substr.read());
			CHECK_EQUAL('s', substr.read());
			CHECK_EQUAL('u', substr.read());
			CHECK_EQUAL('b', substr.read());
			

		}

	}
}
UNITTEST_SUITE_END
