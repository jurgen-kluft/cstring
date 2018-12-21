#include "xbase/x_allocator.h"
#include "xbase/x_runes.h"
#include "xstring/x_string.h"
#include "xunittest/xunittest.h"

using namespace xcore;

	class test_utf32_runes_allocator : public utf32::alloc
	{
		xalloc* m_allocator;
	public:
		test_utf32_runes_allocator (xalloc* allocator) : m_allocator(allocator) {}

		virtual utf32::runes allocate(s32 len, s32 cap)
		{
			if (len > cap)
				cap = len;

			utf32::runes r;
			r.m_str			 = (utf32::prune)m_allocator->allocate(cap * sizeof(utf32::rune), sizeof(void*));
			r.m_end			 = r.m_str + len;
			r.m_eos			 = r.m_str + cap - 1;
			r.m_end[0]		 = '\0';
			r.m_end[cap - 1] = '\0';
			return r;
		}

		virtual void deallocate(utf32::runes& r)
		{
			if (r.m_str != nullptr)
			{
				m_allocator->deallocate(r.m_str);
				r = utf32::runes();
			}
		}

		XCORE_CLASS_PLACEMENT_NEW_DELETE
	};

	extern xcore::xalloc* gTestAllocator;
	test_utf32_runes_allocator* gTestUtf32Allocator = nullptr;

UNITTEST_SUITE_BEGIN(test_xstring)
{
	UNITTEST_FIXTURE(main)
	{
		UNITTEST_FIXTURE_SETUP() 
		{
			xheap heap(gTestAllocator);
			gTestUtf32Allocator = heap.construct<test_utf32_runes_allocator>(gTestAllocator);
		}
		UNITTEST_FIXTURE_TEARDOWN() 
		{
			xheap heap(gTestAllocator);
			heap.destruct<test_utf32_runes_allocator>(gTestUtf32Allocator);
		}

		UNITTEST_TEST(test_index_op)
		{
			xstring str(gTestUtf32Allocator);
			CHECK_TRUE(str.is_empty());
			CHECK_EQUAL(str.size(), 0);
			CHECK_TRUE(str[0] == '\0');
			CHECK_TRUE(str[1] == '\0');
			CHECK_TRUE(str[1000] == '\0');
		}

		UNITTEST_TEST(test_view)
		{
			xstring str(gTestUtf32Allocator);
			CHECK_TRUE(str.is_empty());
			CHECK_EQUAL(str.size(), 0);
			CHECK_TRUE(str[0] == '\0');

			xstring::view v1 = str(2);
			CHECK_TRUE(v1.is_empty());
			CHECK_EQUAL(v1.size(), 0);
			CHECK_TRUE(v1[0] == '\0');
		}

		UNITTEST_TEST(test_view2)
		{
			xstring str(gTestUtf32Allocator);
			CHECK_TRUE(str.is_empty());
			CHECK_EQUAL(str.size(), 0);
			CHECK_TRUE(str[0] == '\0');

			xstring::view c1 = str(4, 10);
			CHECK_TRUE(c1.is_empty());
			CHECK_EQUAL(c1.size(), 0);
			CHECK_TRUE(c1[0] == '\0');
		}

		UNITTEST_TEST(test_copy_con)
		{
			xstring str(gTestUtf32Allocator);
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
			xstring str(gTestUtf32Allocator, "This is an ASCII string converted to UTF-32");
			xstring ascii(gTestUtf32Allocator, "ASCII");

			xstring::view c1 = find(str.full(), ascii.full());
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
			xstring str(gTestUtf32Allocator, "This is an ASCII string converted to UTF-32");
			xstring ascii(gTestUtf32Allocator, "ASCII");

			xstring::view c1 = selectUntil(str.full(), ascii.full());
			CHECK_FALSE(c1.is_empty());
			CHECK_EQUAL(c1.size(), 11);
			CHECK_TRUE(c1[0] == 'T');
		}

		UNITTEST_TEST(test_selectUntilIncluded)
		{
			xstring str(gTestUtf32Allocator, "This is an ASCII string converted to UTF-32");

			xstring::view c1 = selectUntilIncluded(str, xstring(gTestUtf32Allocator, "ASCII"));
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
			xstring str1(gTestUtf32Allocator, "THIS IS AN UPPERCASE STRING WITH NUMBERS 1234");
			CHECK_TRUE(isUpper(str1));
			xstring str2(gTestUtf32Allocator, "this is a lowercase string with numbers 1234");
			CHECK_TRUE(!isUpper(str2));
		}

		UNITTEST_TEST(test_isLower)
		{
			xstring str1(gTestUtf32Allocator, "THIS IS AN UPPERCASE STRING WITH NUMBERS 1234");
			CHECK_TRUE(!isLower(str1));
			xstring str2(gTestUtf32Allocator, "this is a lowercase string with numbers 1234");
			CHECK_TRUE(isLower(str2));
		}

		UNITTEST_TEST(test_isCapitalized)
		{
			xstring str1(gTestUtf32Allocator, "This Is A Capitalized String With Numbers 1234");
			CHECK_TRUE(isCapitalized(str1));
			xstring str2(gTestUtf32Allocator, "this is a lowercase string with numbers 1234");
			CHECK_TRUE(!isCapitalized(str2));
		}

		UNITTEST_TEST(test_isQuoted)
		{
			xstring str1(gTestUtf32Allocator, "\"a quoted piece of text\"");
			CHECK_TRUE(isQuoted(str1));
			xstring str2(gTestUtf32Allocator, "just a piece of text");
			CHECK_TRUE(!isQuoted(str2));
		}

		UNITTEST_TEST(test_isQuoted2)
		{
			xstring str1(gTestUtf32Allocator, "$a quoted piece of text$");
			CHECK_TRUE(isQuoted(str1, '$'));
			xstring str2(gTestUtf32Allocator, "just a piece of text");
			CHECK_TRUE(!isQuoted(str2, '$'));
		}

		UNITTEST_TEST(test_isDelimited)
		{
			xstring str1(gTestUtf32Allocator, "[a delimited piece of text]");
			CHECK_TRUE(isDelimited(str1, '[', ']'));
			xstring str2(gTestUtf32Allocator, "just a piece of text");
			CHECK_TRUE(!isDelimited(str2, '[', ']'));
		}

		UNITTEST_TEST(test_firstChar)
		{
			xstring str1(gTestUtf32Allocator, "First character");
			CHECK_EQUAL(firstChar(str1), 'F');
			CHECK_NOT_EQUAL(firstChar(str1), 'G');
		}

		UNITTEST_TEST(test_lastChar)
		{
			xstring str1(gTestUtf32Allocator, "Last character");
			CHECK_EQUAL(lastChar(str1), 'r');
			CHECK_NOT_EQUAL(lastChar(str1), 's');
		}

		UNITTEST_TEST(test_startsWith)
		{
			xstring str1(gTestUtf32Allocator, "Last character");
			CHECK_TRUE(startsWith(str1, xstring(gTestUtf32Allocator, "Last")));
			CHECK_FALSE(startsWith(str1, xstring(gTestUtf32Allocator, "First")));
		}

		UNITTEST_TEST(test_endsWith)
		{
			xstring str1(gTestUtf32Allocator, "Last character");
			CHECK_TRUE(endsWith(str1, xstring(gTestUtf32Allocator, "character")));
			CHECK_FALSE(endsWith(str1, xstring(gTestUtf32Allocator, "first")));
		}

		UNITTEST_TEST(test_find)
		{
			xstring str1(gTestUtf32Allocator, "This is a piece of text to find something in");
			
			xstring::view c1 = find(str1.full(), 'p');
			CHECK_FALSE(c1.is_empty());
			CHECK_EQUAL(c1.size(), 1);
			CHECK_EQUAL(c1[0], 'p');
		}

		UNITTEST_TEST(test_insert)
		{
			xstring str1(gTestUtf32Allocator, "This is text to change something in");
			CHECK_EQUAL(str1.size(), 35);

			// First some views
			xstring::view v1 = find(str1.full(), xstring(gTestUtf32Allocator, "text"));
			xstring::view v2 = find(str1.full(), xstring(gTestUtf32Allocator, " in"));
			CHECK_EQUAL(v1.size(), 4);
			CHECK_EQUAL(v2.size(), 3);

			// Now change the string so that it will resize
			insert(str1, v1, xstring(gTestUtf32Allocator, "modified "));
			CHECK_EQUAL(35 + 9, str1.size());

			CHECK_EQUAL(4, v1.size());
			CHECK_EQUAL(3, v2.size());

			CHECK_TRUE(v1[0] == 't');
			CHECK_TRUE(v1[1] == 'e');
			CHECK_TRUE(v1[2] == 'x');
			CHECK_TRUE(v1[3] == 't');

			CHECK_TRUE(v2[0] == ' ');
			CHECK_TRUE(v2[1] == 'i');
			CHECK_TRUE(v2[2] == 'n');

			xstring str2(gTestUtf32Allocator, "This is modified text to change something in");
			CHECK_EQUAL(str2.size(), str1.size());
			CHECK_TRUE(str1 == str2);
		}

		UNITTEST_TEST(test_find_remove)
		{
			xstring str1(gTestUtf32Allocator, "This is text to remove something from");
			CHECK_EQUAL(37, str1.size());

			// First some views
			xstring::view v1 = find(str1.full(), xstring(gTestUtf32Allocator, "remove"));
			xstring::view v2 = find(str1.full(), xstring(gTestUtf32Allocator, "from"));
			CHECK_EQUAL(v1.size(), 6);
			CHECK_EQUAL(v2.size(), 4);

			// Now change the string so that it will resize
			xstring strr(gTestUtf32Allocator, " to remove something from");
			CHECK_EQUAL(25, strr.size());
			find_remove(str1, strr.full());
			CHECK_EQUAL(37 - 25, str1.size());

			CHECK_TRUE(v1.is_empty());
			CHECK_TRUE(v2.is_empty());

			xstring str2(gTestUtf32Allocator, "This is text");
			CHECK_EQUAL(str2.size(), str1.size());
			CHECK_TRUE(str1 == str2);
		}

		UNITTEST_TEST(test_find_replace)
		{
			xstring str1(gTestUtf32Allocator, "This is text to change something in");
			CHECK_EQUAL(35, str1.size());

			// First some views
			xstring::view v1 = find(str1.full(), xstring(gTestUtf32Allocator, "change"));
			xstring::view v2 = find(str1.full(), xstring(gTestUtf32Allocator, "in"));
			CHECK_EQUAL(v1.size(), 6);
			CHECK_EQUAL(v2.size(), 2);

			// Now change the string so that it will resize
			xstring strr(gTestUtf32Allocator, "fix");
			CHECK_EQUAL(3, strr.size());
			find_replace(str1, v1, strr);
			CHECK_EQUAL(35 - 3, str1.size());

			xstring str2(gTestUtf32Allocator, "This is text to fix something in");
			CHECK_EQUAL(str2.size(), str1.size());
			CHECK_TRUE(str1 == str2);
		}

		UNITTEST_TEST(test_remove_any)
		{
			xstring str1(gTestUtf32Allocator, "This is text to #change $something &in");
			CHECK_EQUAL(38, str1.size());

			// Now change the string so that it will resize
			xstring strr(gTestUtf32Allocator, "#$&");
			CHECK_EQUAL(3, strr.size());
			remove_any(str1, strr);
			CHECK_EQUAL(38 - 3, str1.size());

			xstring str2(gTestUtf32Allocator, "This is text to change something in");
			CHECK_EQUAL(str2.size(), str1.size());
			CHECK_TRUE(str1 == str2);
		}

	}
}
UNITTEST_SUITE_END
