#include "xbase/x_runes.h"
#include "xstring/x_string.h"
#include "xunittest/xunittest.h"

using namespace xcore;

UNITTEST_SUITE_BEGIN(test_xstring)
{
	UNITTEST_FIXTURE(main)
	{
		UNITTEST_FIXTURE_SETUP() {}
		UNITTEST_FIXTURE_TEARDOWN() {}

		UNITTEST_TEST(test_index_op)
		{
			xstring str;
			CHECK_TRUE(str.is_empty());
			CHECK_TRUE(str.size() == 0);
			CHECK_TRUE(str[0] == '\0');
			CHECK_TRUE(str[1] == '\0');
			CHECK_TRUE(str[1000] == '\0');
		}

		UNITTEST_TEST(test_view)
		{
			xstring str;
			CHECK_TRUE(str.is_empty());
			CHECK_TRUE(str.size() == 0);
			CHECK_TRUE(str[0] == '\0');

			xstring v1 = str(2);
			CHECK_TRUE(v1.is_empty());
			CHECK_TRUE(v1.size() == 0);
			CHECK_TRUE(v1[0] == '\0');
		}

		UNITTEST_TEST(test_copy_con)
		{
			xstring str;
			CHECK_TRUE(str.is_empty());
			CHECK_TRUE(str.size() == 0);
			CHECK_TRUE(str[0] == '\0');

			xstring c1(str);
			CHECK_TRUE(c1.is_empty());
			CHECK_TRUE(c1.size() == 0);
			CHECK_TRUE(c1[0] == '\0');
		}
	}
}
UNITTEST_SUITE_END
