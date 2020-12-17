#include "xbase/x_base.h"
#include "xbase/x_allocator.h"
#include "xbase/x_console.h"

#include "xstring/x_string.h"

#include "xunittest/xunittest.h"
#include "xunittest/private/ut_ReportAssert.h"

UNITTEST_SUITE_LIST(xStringUnitTest);

UNITTEST_SUITE_DECLARE(xStringUnitTest, test_xstring);
UNITTEST_SUITE_DECLARE(xStringUnitTest, test_string_reader);
UNITTEST_SUITE_DECLARE(xStringUnitTest, test_string_writer);

namespace xcore
{
	class UnitTestAssertHandler : public xcore::asserthandler_t
	{
	public:
		UnitTestAssertHandler()
		{
			NumberOfAsserts = 0;
		}

		virtual xcore::bool	handle_assert(u32& flags, const char* fileName, s32 lineNumber, const char* exprString, const char* messageString)
		{
			UnitTest::reportAssert(exprString, fileName, lineNumber);
			NumberOfAsserts++;
			return false;
		}

		xcore::s32		NumberOfAsserts;
	};

	class UnitTestAllocator : public UnitTest::Allocator
	{
		alloc_t*			mAllocator;
	public:
						UnitTestAllocator(alloc_t* allocator)				{ mAllocator = allocator; }
		virtual void*	Allocate(size_t size)								{ return mAllocator->allocate(size, 4); }
		virtual size_t	Deallocate(void* ptr)								{ return mAllocator->deallocate(ptr); }
	};

	class TestAllocator : public alloc_t
	{
		alloc_t*				mAllocator;
	public:
							TestAllocator(alloc_t* allocator) : mAllocator(allocator) { }

		virtual const char*	name() const										{ return "xcore unittest test heap allocator"; }

		virtual void*		v_allocate(u32 size, u32 alignment)
		{
			UnitTest::IncNumAllocations();
			return mAllocator->allocate(size, alignment);
		}

		virtual u32 		v_deallocate(void* mem)
		{
			UnitTest::DecNumAllocations();
			return mAllocator->deallocate(mem);
		}

		virtual void		v_release()
		{
			mAllocator->release();
			mAllocator = NULL;
		}
	};
}

xcore::alloc_t* gTestAllocator = NULL;
xcore::UnitTestAssertHandler gAssertHandler;


bool gRunUnitTest(UnitTest::TestReporter& reporter)
{
	xbase::x_Init();
#ifdef TARGET_DEBUG
	xcore::asserthandler_t::sRegisterHandler(&gAssertHandler);
#endif

	xcore::TestAllocator testAllocator(xcore::alloc_t::get_system());
	gTestAllocator = &testAllocator;

	xcore::UnitTestAllocator unittestAllocator(gTestAllocator);
	UnitTest::SetAllocator(&unittestAllocator);

	xcore::console->write("Configuration: ");
	xcore::console->writeLine(TARGET_FULL_DESCR_STR);

	int r = UNITTEST_SUITE_RUN(reporter, xStringUnitTest);

	gTestAllocator->release();

	UnitTest::SetAllocator(NULL);

	xbase::x_Exit();
	return r==0;
}

