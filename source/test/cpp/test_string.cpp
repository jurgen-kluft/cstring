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
			CHECK_EQUAL(str.size(), 0);
			CHECK_TRUE(str[0] == '\0');
			CHECK_TRUE(str[1] == '\0');
			CHECK_TRUE(str[1000] == '\0');
		}

		UNITTEST_TEST(test_view)
		{
			xstring str;
			CHECK_TRUE(str.is_empty());
			CHECK_EQUAL(str.size(), 0);
			CHECK_TRUE(str[0] == '\0');

			xstring v1 = str(2);
			CHECK_TRUE(v1.is_empty());
			CHECK_EQUAL(v1.size(), 0);
			CHECK_TRUE(v1[0] == '\0');
		}

		UNITTEST_TEST(test_view2)
		{
			xstring str;
			CHECK_TRUE(str.is_empty());
			CHECK_EQUAL(str.size(), 0);
			CHECK_TRUE(str[0] == '\0');

			xstring c1 = str(4, 10);
			CHECK_TRUE(c1.is_empty());
			CHECK_EQUAL(c1.size(), 0);
			CHECK_TRUE(c1[0] == '\0');
		}

		UNITTEST_TEST(test_copy_con)
		{
			xstring str;
			CHECK_TRUE(str.is_empty());
			CHECK_EQUAL(str.size(), 0);
			CHECK_TRUE(str[0] == '\0');

			xstring c1(str);
			CHECK_TRUE(c1.is_empty());
			CHECK_EQUAL(c1.size(), 0);
			CHECK_TRUE(c1[0] == '\0');
		}

		UNITTEST_TEST(test_selectUntil)
		{
			xstring str("This is an ASCII string converted to UTF-32");

			xstring c1 = selectUntil(str, xstring("ASCII"));
			CHECK_FALSE(c1.is_empty());
			CHECK_EQUAL(c1.size(), 11);
			CHECK_TRUE(c1[0] == 'T');
		}

		UNITTEST_TEST(test_selectUntilIncluded)
		{
			xstring str("This is an ASCII string converted to UTF-32");

			xstring c1 = selectUntilIncluded(str, xstring("ASCII"));
			CHECK_FALSE(c1.is_empty());
			CHECK_EQUAL(c1.size(), 16);
			CHECK_TRUE(c1[11] == 'A');
			CHECK_TRUE(c1[12] == 'S');
			CHECK_TRUE(c1[13] == 'C');
			CHECK_TRUE(c1[14] == 'I');
			CHECK_TRUE(c1[15] == 'I');
		}

		UNITTEST_TEST(test_narrowIfDelimited)
		{
			xstring str("<XML tag>");
			CHECK_TRUE(narrowIfDelimited(str, '<', '>'));
			CHECK_EQUAL(str.size(), 7);
			CHECK_TRUE(str[0] == 'X');
			CHECK_TRUE(str[6] == 'g');
		}
	}
}
UNITTEST_SUITE_END
