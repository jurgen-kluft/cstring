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

		UNITTEST_TEST(test_select)
		{
			xstring str("This is an ASCII string converted to UTF-32");

			xstring c1 = select(str, xstring("ASCII"));
			CHECK_FALSE(c1.is_empty());
			CHECK_EQUAL(c1.size(), 5);
			CHECK_TRUE(c1[0] == 'A');
			CHECK_TRUE(c1[1] == 'S');
			CHECK_TRUE(c1[2] == 'C');
			CHECK_TRUE(c1[3] == 'I');
			CHECK_TRUE(c1[4] == 'I');
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

		UNITTEST_TEST(test_isUpper)
		{
			xstring str1("THIS IS AN UPPERCASE STRING WITH NUMBERS 1234");
			CHECK_TRUE(isUpper(str1));
			xstring str2("this is a lowercase string with numbers 1234");
			CHECK_TRUE(!isUpper(str2));
		}

		UNITTEST_TEST(test_isLower)
		{
			xstring str1("THIS IS AN UPPERCASE STRING WITH NUMBERS 1234");
			CHECK_TRUE(!isLower(str1));
			xstring str2("this is a lowercase string with numbers 1234");
			CHECK_TRUE(isLower(str2));
		}

		UNITTEST_TEST(test_isCapitalized)
		{
			xstring str1("This Is A Capitalized String With Numbers 1234");
			CHECK_TRUE(isCapitalized(str1));
			xstring str2("this is a lowercase string with numbers 1234");
			CHECK_TRUE(!isCapitalized(str2));
		}

		UNITTEST_TEST(test_isQuoted)
		{
			xstring str1("\"a quoted piece of text\"");
			CHECK_TRUE(isQuoted(str1));
			xstring str2("just a piece of text");
			CHECK_TRUE(!isQuoted(str2));
		}

		UNITTEST_TEST(test_isQuoted2)
		{
			xstring str1("$a quoted piece of text$");
			CHECK_TRUE(isQuoted(str1, '$'));
			xstring str2("just a piece of text");
			CHECK_TRUE(!isQuoted(str2, '$'));
		}

		UNITTEST_TEST(test_isDelimited)
		{
			xstring str1("[a delimited piece of text]");
			CHECK_TRUE(isDelimited(str1, '[', ']'));
			xstring str2("just a piece of text");
			CHECK_TRUE(!isDelimited(str2, '[', ']'));
		}

		UNITTEST_TEST(test_firstChar)
		{
			xstring str1("First character");
			CHECK_EQUAL(firstChar(str1), 'F');
			CHECK_NOT_EQUAL(firstChar(str1), 'G');
		}

		UNITTEST_TEST(test_lastChar)
		{
			xstring str1("Last character");
			CHECK_EQUAL(lastChar(str1), 'r');
			CHECK_NOT_EQUAL(lastChar(str1), 's');
		}

		UNITTEST_TEST(test_startsWith)
		{
			xstring str1("Last character");
			CHECK_TRUE(startsWith(str1, xstring("Last")));
			CHECK_FALSE(startsWith(str1, xstring("First")));
		}

		UNITTEST_TEST(test_endsWith)
		{
			xstring str1("Last character");
			CHECK_TRUE(endsWith(str1, xstring("character")));
			CHECK_FALSE(endsWith(str1, xstring("first")));
		}

		UNITTEST_TEST(test_find)
		{
			xstring str1("This is a piece of text to find something in");
			
			xstring c1 = find(str1, 'p');
			CHECK_FALSE(c1.is_empty());
			CHECK_EQUAL(c1.size(), 1);
			CHECK_EQUAL(c1[0], 'p');
		}
	}
}
UNITTEST_SUITE_END
