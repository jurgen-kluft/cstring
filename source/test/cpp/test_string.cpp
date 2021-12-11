#include "xbase/x_allocator.h"
#include "xbase/x_runes.h"
#include "xstring/x_string.h"
#include "xunittest/xunittest.h"

using namespace xcore;

using string_t = xstring;

UNITTEST_SUITE_BEGIN(test_xstring)
{
    UNITTEST_FIXTURE(main)
    {
        UNITTEST_FIXTURE_SETUP() {  }
        UNITTEST_FIXTURE_TEARDOWN() {  }

        UNITTEST_TEST(test_index_op)
        {
            string_t str;
            CHECK_TRUE(str.is_empty());
            CHECK_EQUAL(str.size(), 0);
            CHECK_TRUE(str[0] == '\0');
            CHECK_TRUE(str[1] == '\0');
            CHECK_TRUE(str[1000] == '\0');
        }

        UNITTEST_TEST(test_view)
        {
            string_t str;
            CHECK_TRUE(str.is_empty());
            CHECK_EQUAL(str.size(), 0);
            CHECK_TRUE(str[0] == '\0');

            string_t v1 = str(2);
            CHECK_TRUE(v1.is_empty());
            CHECK_EQUAL(v1.size(), 0);
            CHECK_TRUE(v1[0] == '\0');
        }

        UNITTEST_TEST(test_view2)
        {
            string_t str;
            CHECK_TRUE(str.is_empty());
            CHECK_EQUAL(str.size(), 0);
            CHECK_TRUE(str[0] == '\0');

            string_t c1 = str(4, 10);
            CHECK_TRUE(c1.is_empty());
            CHECK_EQUAL(c1.size(), 0);
            CHECK_TRUE(c1[0] == '\0');
        }

        UNITTEST_TEST(test_copy_con)
        {
            string_t str;
            CHECK_TRUE(str.is_empty());
            CHECK_EQUAL(str.size(), 0);
            CHECK_TRUE(str[0] == '\0');

            string_t c1(str);
            CHECK_TRUE(c1.is_empty());
            CHECK_EQUAL(c1.size(), 0);
            CHECK_TRUE(c1[0] == '\0');
        }

        UNITTEST_TEST(test_select)
        {
            string_t str("This is an ASCII string converted to UTF-32");
            string_t ascii("ASCII");

            string_t c1 = find(str, ascii);
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
            string_t str( "This is an ASCII string converted to UTF-32");
            string_t ascii( "ASCII");

            string_t c1 = selectUntil(str, ascii);
            CHECK_FALSE(c1.is_empty());
            CHECK_EQUAL(c1.size(), 11);
            CHECK_TRUE(c1[0] == 'T');
        }

        UNITTEST_TEST(test_selectUntilIncluded)
        {
            string_t str( "This is an ASCII string converted to UTF-32");

            string_t c1 = selectUntilIncluded(str, string_t( "ASCII"));
            CHECK_FALSE(c1.is_empty());
            CHECK_EQUAL(c1.size(), 16);
            CHECK_TRUE(c1[11] == 'A');
            CHECK_TRUE(c1[12] == 'S');
            CHECK_TRUE(c1[13] == 'C');
            CHECK_TRUE(c1[14] == 'I');
            CHECK_TRUE(c1[15] == 'I');
        }

        UNITTEST_TEST(test_isUpper)
        {
            string_t str1( "THIS IS AN UPPERCASE STRING WITH NUMBERS 1234");
            CHECK_TRUE(isUpper(str1));
            string_t str2( "this is a lowercase string with numbers 1234");
            CHECK_TRUE(!isUpper(str2));
        }

        UNITTEST_TEST(test_isLower)
        {
            string_t str1( "THIS IS AN UPPERCASE STRING WITH NUMBERS 1234");
            CHECK_TRUE(!isLower(str1));
            string_t str2( "this is a lowercase string with numbers 1234");
            CHECK_TRUE(isLower(str2));
        }

        UNITTEST_TEST(test_isCapitalized)
        {
            string_t str1("This Is A Capitalized String With Numbers 1234");
            CHECK_TRUE(isCapitalized(str1));
            string_t str2("this is a lowercase string with numbers 1234");
            CHECK_TRUE(!isCapitalized(str2));
        }

        UNITTEST_TEST(test_isQuoted)
        {
            string_t str1("\"a quoted piece of text\"");
            CHECK_TRUE(isQuoted(str1));
            string_t str2("just a piece of text");
            CHECK_TRUE(!isQuoted(str2));
        }

        UNITTEST_TEST(test_isQuoted2)
        {
            string_t str1("$a quoted piece of text$");
            CHECK_TRUE(isQuoted(str1, '$'));
            string_t str2("just a piece of text");
            CHECK_TRUE(!isQuoted(str2, '$'));
        }

        UNITTEST_TEST(test_isDelimited)
        {
            string_t str1("[a delimited piece of text]");
            CHECK_TRUE(isDelimited(str1, '[', ']'));
            string_t str2("just a piece of text");
            CHECK_TRUE(!isDelimited(str2, '[', ']'));
        }

        UNITTEST_TEST(test_firstChar)
        {
            string_t str1("First character");
            CHECK_EQUAL(firstChar(str1), 'F');
            CHECK_NOT_EQUAL(firstChar(str1), 'G');
        }

        UNITTEST_TEST(test_lastChar)
        {
            string_t str1("Last character");
            CHECK_EQUAL(lastChar(str1), 'r');
            CHECK_NOT_EQUAL(lastChar(str1), 's');
        }

        UNITTEST_TEST(test_startsWith)
        {
            string_t str1("Last character");
            CHECK_TRUE(startsWith(str1, string_t("Last")));
            CHECK_FALSE(startsWith(str1, string_t("First")));
        }

        UNITTEST_TEST(test_endsWith)
        {
            string_t str1("Last character");
            CHECK_TRUE(endsWith(str1, string_t("character")));
            CHECK_FALSE(endsWith(str1, string_t("first")));
        }

        UNITTEST_TEST(test_find)
        {
            string_t str1("This is a piece of text to find something in");

            string_t c1 = find(str1, 'p');
            CHECK_FALSE(c1.is_empty());
            CHECK_EQUAL(1, c1.size());
            CHECK_EQUAL('p', c1[0]);
        }

        UNITTEST_TEST(test_insert)
        {
            string_t str1("This is text to change something in");
            CHECK_EQUAL(str1.size(), 35);

            // First some views
            string_t v1 = find(str1, string_t("text"));
            string_t v2 = find(str1, string_t(" in"));
            CHECK_EQUAL(v1.size(), 4);
            CHECK_EQUAL(v2.size(), 3);

            // Now change the string so that it will resize
            insert(str1, v1, string_t("modified "));
            CHECK_EQUAL(35 + 9, str1.size());

            CHECK_EQUAL(4, v1.size());
            CHECK_EQUAL(3, v2.size());

            CHECK_EQUAL('t', v1[0]);
            CHECK_EQUAL('e', v1[1]);
            CHECK_EQUAL('x', v1[2]);
            CHECK_EQUAL('t', v1[3]);

            CHECK_TRUE(v2[0] == ' ');
            CHECK_TRUE(v2[1] == 'i');
            CHECK_TRUE(v2[2] == 'n');

            string_t str2("This is modified text to change something in");
            CHECK_EQUAL(str2.size(), str1.size());
            CHECK_TRUE(str1 == str2);
        }

        UNITTEST_TEST(test_find_remove)
        {
            string_t str1("This is text to remove something from");
            CHECK_EQUAL(37, str1.size());

            // First some views
            string_t v1 = find(str1, string_t("remove"));
            string_t v2 = find(str1, string_t("from"));
            CHECK_EQUAL(v1.size(), 6);
            CHECK_EQUAL(v2.size(), 4);

            // Now change the string so that it will resize
            string_t strr(" to remove something from");
            CHECK_EQUAL(25, strr.size());
            find_remove(str1, strr);
            CHECK_EQUAL(37 - 25, str1.size());

            CHECK_TRUE(v1.is_empty());
            CHECK_TRUE(v2.is_empty());

            string_t str2("This is text");
            CHECK_EQUAL(str2.size(), str1.size());
            CHECK_TRUE(str1 == str2);
        }

        UNITTEST_TEST(test_find_remove_from_sub_string)
        {
        }

        UNITTEST_TEST(test_find_replace)
        {
            string_t thestr("This is text to change something in");
            CHECK_EQUAL(35, thestr.size());

            // First some views
            string_t v1 = find(thestr, string_t("change"));
            string_t v2 = find(thestr, string_t("in"));
            CHECK_EQUAL(v1.size(), 6);
            CHECK_EQUAL(v2.size(), 2);

            // Now change the string so that it will resize (smaller)
            string_t strr("fix");
            CHECK_EQUAL(3, strr.size());
            find_replace(thestr, v1, strr);
            CHECK_EQUAL(35 + (3-6), thestr.size());

            CHECK_EQUAL(v2.size(), 2);

            string_t result("This is text to fix something in");
            CHECK_EQUAL(result.size(), thestr.size());
            CHECK_TRUE(thestr == result);

            // Now change the string so that it will resize (larger)
            string_t strr2("rectify");
            CHECK_EQUAL(7, strr2.size());
            find_replace(thestr, strr, strr2);
            CHECK_EQUAL(32 + (7-3), thestr.size());

            result = ("This is text to rectify something in");
            CHECK_EQUAL(result.size(), thestr.size());
            CHECK_TRUE(thestr == result);

            CHECK_EQUAL(v2.size(), 2);
        }

        UNITTEST_TEST(test_remove_any)
        {
            string_t str1("This is text to #change $something &in");
            CHECK_EQUAL(38, str1.size());

            string_t thing = find(str1, "thing");
            CHECK_EQUAL(5, thing.size());

            // Now change the string so that it will resize
            string_t strr("#$&");
            CHECK_EQUAL(3, strr.size());
            remove_any(str1, strr);
            CHECK_EQUAL(38 - 3, str1.size());

            CHECK_EQUAL(5, thing.size());
            CHECK_EQUAL('t', thing[0]);
            CHECK_EQUAL('h', thing[1]);
            CHECK_EQUAL('i', thing[2]);
            CHECK_EQUAL('n', thing[3]);
            CHECK_EQUAL('g', thing[4]);

            string_t str2("This is text to change something in");
            CHECK_EQUAL(str2.size(), str1.size());
            CHECK_TRUE(str1 == str2);
        }
    }
}
UNITTEST_SUITE_END
