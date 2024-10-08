#include "cbase/c_allocator.h"
#include "cbase/c_runes.h"
#include "cstring/c_string.h"
#include "cunittest/cunittest.h"

using namespace ncore;

UNITTEST_SUITE_BEGIN(test_string)
{
    UNITTEST_FIXTURE(main)
    {
        UNITTEST_ALLOCATOR;

        UNITTEST_FIXTURE_SETUP()
        {
            nstring_memory::init(Allocator, Allocator);
        }

        UNITTEST_FIXTURE_TEARDOWN() {}

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

            string_t v1 = str(0,2);
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

        UNITTEST_TEST(test_construct_1_from_ascii_string_destruct)
        {
            string_t str("This is an ASCII string converted to UTF-16");
            CHECK_FALSE(str.is_empty());
            CHECK_EQUAL(str.size(), 43);
            CHECK_TRUE(str[0] == 'T');
        }

        UNITTEST_TEST(test_construct_2_from_ascii_string_destruct)
        {
            string_t str("ASCII");
            CHECK_FALSE(str.is_empty());
            CHECK_EQUAL(str.size(), 5);
            CHECK_TRUE(str[0] == 'A');
        }

        UNITTEST_TEST(test_construct_3_from_ascii_string_destruct)
        {
            string_t str("This is an ASCII string converted to UTF-16");
            string_t ascii("ASCII");

            CHECK_FALSE(str.is_empty());
            CHECK_EQUAL(str.size(), 43);
            CHECK_TRUE(str[0] == 'T');

            CHECK_FALSE(ascii.is_empty());
            CHECK_EQUAL(ascii.size(), 5);
            CHECK_TRUE(ascii[0] == 'A');
        }

        UNITTEST_TEST(test_select)
        {
            string_t str("This is an ASCII string converted to UTF-16");
            string_t ascii("ASCII");

            string_t c1 = str.find(ascii);
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
            string_t str("This is an ASCII string converted to UTF-16");
            string_t ascii("ASCII");

            // Select until ASCII string will fail because 'ascii' is not a selection of 'str'
            string_t c1 = str.selectUntil(ascii);
            CHECK_TRUE(c1.is_empty());
            CHECK_EQUAL(c1.size(), 0);
            CHECK_TRUE(c1[0] == '\0');
        }

        UNITTEST_TEST(test_selectUntil_correct_use)
        {
            string_t str("This is an ASCII string converted to UTF-16");
            string_t ascii("ASCII");

            // Select until ASCII string will fail because 'ascii' is not a selection of 'str'
            string_t selection = str.find(ascii);
            string_t c1 = str.selectUntil(selection); // c1 should be "This is an "
            CHECK_FALSE(c1.is_empty());
            CHECK_EQUAL(c1.size(), 11);
            CHECK_TRUE(c1[0] == 'T');
        }

        UNITTEST_TEST(test_selectUntilIncluded)
        {
            string_t str("This is an ASCII string converted to UTF-16");
            string_t ascii("ASCII");

            string_t selection = str.find(ascii);
            string_t c1 = str.selectUntilIncluded(selection);
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
            string_t str1("THIS IS AN UPPERCASE STRING WITH NUMBERS 1234");
            CHECK_TRUE(str1.isUpper());
            string_t str2("this is a lowercase string with numbers 1234");
            CHECK_FALSE(str2.isUpper());
        }

        UNITTEST_TEST(test_isLower)
        {
            string_t str1("THIS IS AN UPPERCASE STRING WITH NUMBERS 1234");
            CHECK_FALSE(str1.isLower());
            string_t str2("this is a lowercase string with numbers 1234");
            CHECK_TRUE(str2.isLower());
        }

        UNITTEST_TEST(test_isCapitalized)
        {
            string_t str1("This Is A Capitalized String With Numbers 1234");
            CHECK_TRUE(str1.isCapitalized());
            string_t str2("this is a lowercase string with numbers 1234");
            CHECK_FALSE(str2.isCapitalized());
        }

        UNITTEST_TEST(test_isQuoted)
        {
            string_t str1("\"a quoted piece of text\"");
            CHECK_TRUE(str1.isQuoted());
            string_t str2("just a piece of text");
            CHECK_FALSE(str2.isQuoted());
        }

        UNITTEST_TEST(test_isQuoted2)
        {
            string_t str1("$a quoted piece of text$");
            CHECK_TRUE(str1.isQuoted('$'));
            string_t str2("just a piece of text");
            CHECK_FALSE(str2.isQuoted('$'));
        }

        UNITTEST_TEST(test_isDelimited)
        {
            string_t str1("[a delimited piece of text]");
            CHECK_TRUE(str1.isDelimited('[', ']'));
            string_t str2("just a piece of text");
            CHECK_FALSE(str2.isDelimited('[', ']'));
        }

        UNITTEST_TEST(test_firstChar)
        {
            string_t str1("First character");
            CHECK_EQUAL(str1.firstChar(), 'F');
            CHECK_NOT_EQUAL(str1.firstChar(), 'G');
        }

        UNITTEST_TEST(test_lastChar)
        {
            string_t str1("Last character");
            CHECK_EQUAL(str1.lastChar(), 'r');
            CHECK_NOT_EQUAL(str1.lastChar(), 's');
        }

        UNITTEST_TEST(test_startsWith)
        {
            string_t str1("Last character");
            string_t last("Last");
            string_t first("First");
            CHECK_TRUE(str1.startsWith(last));
            CHECK_FALSE(str1.startsWith(first));
        }

        UNITTEST_TEST(test_endsWith)
        {
            string_t str1("Last character");
            string_t chr("character");
            bool     t1 = str1.endsWith(chr);
            CHECK_TRUE(t1);
            string_t f("first");
            CHECK_FALSE(str1.endsWith(f));
        }

        UNITTEST_TEST(test_find)
        {
            string_t str1("This is a piece of text to find something in");

            string_t c1 = str1.find('p');
            CHECK_FALSE(c1.is_empty());
            CHECK_EQUAL(1, c1.size());
            CHECK_EQUAL('p', c1[0]);
        }

        UNITTEST_TEST(test_insert)
        {
            string_t str1("This is text to change something in");
            CHECK_EQUAL(str1.size(), 35);

            // First some views
            string_t v1 = str1.find("text");
            string_t v2 = str1.find(" in");
            CHECK_EQUAL(v1.size(), 4);
            CHECK_EQUAL(v2.size(), 3);

            // Now change the string so that it will resize
            string_t m("modified ");
            str1.insertBeforeSelection(v1, m);
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

            char const* constAsciiStr = "This is modified text to change something in";

            string_t str2(constAsciiStr);
            CHECK_EQUAL(str2.size(), str1.size());
            CHECK_TRUE(str1 == str2);

            char asciiStr[256];
            str1.toAscii(asciiStr, 256);
            CHECK_TRUE(ascii::compare(asciiStr, constAsciiStr) == 0);
        }

        UNITTEST_TEST(test_find_remove)
        {
            string_t str1("This is text to remove something from");
            CHECK_EQUAL(37, str1.size());

            // First some views
            string_t v1 = str1.find("remove");
            string_t v2 = str1.find("from");
            CHECK_EQUAL(v1.size(), 6);
            CHECK_EQUAL(v2.size(), 4);

            // Now change the string so that it will resize
            string_t strr(" to remove something from");
            CHECK_EQUAL(25, strr.size());
            str1.findRemove(strr);
            CHECK_EQUAL(37 - 25, str1.size());

            CHECK_TRUE(v1.is_empty());
            CHECK_TRUE(v2.is_empty());

            string_t str2("This is text");
            CHECK_EQUAL(str2.size(), str1.size());
            CHECK_TRUE(str1 == str2);
        }

        UNITTEST_TEST(test_find_remove_from_sub_string) {}

        UNITTEST_TEST(test_find_replace)
        {
            string_t thestr("This is text to change something in");
            CHECK_EQUAL(35, thestr.size());

            // First some views
            string_t v1 = thestr.find("change");
            string_t v2 = thestr.find("in");
            CHECK_EQUAL(v1.size(), 6);
            CHECK_EQUAL(v2.size(), 2);

            // Now change the string so that it will resize (smaller)
            string_t strr("fix");
            CHECK_EQUAL(3, strr.size());
            thestr.findReplace(v1, strr);
            CHECK_EQUAL(35 + (3 - 6), thestr.size());

            CHECK_EQUAL(v2.size(), 2);

            string_t result("This is text to fix something in");
            CHECK_EQUAL(result.size(), thestr.size());
            CHECK_TRUE(thestr == result);

            // Now change the string so that it will resize (larger)
            string_t strr2("rectify");
            CHECK_EQUAL(7, strr2.size());
            thestr.findReplace(strr, strr2);
            CHECK_EQUAL(32 + (7 - 3), thestr.size());

            result = ("This is text to rectify something in");
            CHECK_EQUAL(result.size(), thestr.size());
            CHECK_TRUE(thestr == result);

            CHECK_EQUAL(v2.size(), 2);
        }

        UNITTEST_TEST(test_remove_any)
        {
            string_t str1("This is text to #change $something &in");
            CHECK_EQUAL(38, str1.size());

            string_t thing = str1.find("thing");
            CHECK_EQUAL(5, thing.size());

            // Now change the string so that it will resize
            string_t strr("#$&");
            CHECK_EQUAL(3, strr.size());
            str1.removeAnyChar(strr);
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

        UNITTEST_TEST(test_remove_any2)
        {
            string_t str1("This is text to ##change $$something &&in");
            CHECK_EQUAL(41, str1.size());

            string_t thing = str1.find("thing");
            CHECK_EQUAL(5, thing.size());

            // Now change the string so that it will resize
            string_t strr("#$&");
            CHECK_EQUAL(3, strr.size());
            str1.removeAnyChar(strr);
            CHECK_EQUAL(41 - 6, str1.size());

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
