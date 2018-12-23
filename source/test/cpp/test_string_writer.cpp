#include "xbase/x_allocator.h"
#include "xbase/x_runes.h"
#include "xstring/x_string_reader.h"
#include "xstring/x_string_writer.h"
#include "xunittest/xunittest.h"

using namespace xcore;

extern xcore::xalloc* gTestAllocator;

UNITTEST_SUITE_BEGIN(test_string_writer)
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

			ascii::runez<256> buffer;
			string_writer writer(buffer);
			writer.write(reader);
			string_reader reader2(writer);
			CHECK_TRUE(reader == reader2);
		}

	}
}
UNITTEST_SUITE_END
