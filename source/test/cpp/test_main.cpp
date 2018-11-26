#include "xbase/x_base.h"
#include "xbase/x_allocator.h"
#include "xbase/x_console.h"

#include "xstring/x_string.h"

#include "xunittest/xunittest.h"
#include "xunittest/private/ut_ReportAssert.h"

UNITTEST_SUITE_LIST(xStringUnitTest);

UNITTEST_SUITE_DECLARE(xStringUnitTest, test_xstring);

namespace xcore
{
	class UnitTestAssertHandler : public xcore::xasserthandler
	{
	public:
		UnitTestAssertHandler()
		{
			NumberOfAsserts = 0;
		}

		virtual xcore::xbool	handle_assert(u32& flags, const char* fileName, s32 lineNumber, const char* exprString, const char* messageString)
		{
			UnitTest::reportAssert(exprString, fileName, lineNumber);
			NumberOfAsserts++;
			return false;
		}

		xcore::s32		NumberOfAsserts;
	};

	class UnitTestAllocator : public UnitTest::Allocator
	{
		xalloc*			mAllocator;
	public:
						UnitTestAllocator(xalloc* allocator)				{ mAllocator = allocator; }
		virtual void*	Allocate(size_t size)								{ return mAllocator->allocate(size, 4); }
		virtual void	Deallocate(void* ptr)								{ mAllocator->deallocate(ptr); }
	};

	class TestAllocator : public xalloc
	{
		xalloc*				mAllocator;
	public:
							TestAllocator(xalloc* allocator) : mAllocator(allocator) { }

		virtual const char*	name() const										{ return "xcore unittest test heap allocator"; }

		virtual void*		allocate(xsize_t size, u32 alignment)
		{
			UnitTest::IncNumAllocations();
			return mAllocator->allocate(size, alignment);
		}

		virtual void		deallocate(void* mem)
		{
			UnitTest::DecNumAllocations();
			mAllocator->deallocate(mem);
		}

		virtual void		release()
		{
			mAllocator->release();
			mAllocator = NULL;
		}
	};
}

xcore::xalloc* gTestAllocator = NULL;
xcore::UnitTestAssertHandler gAssertHandler;


bool gRunUnitTest(UnitTest::TestReporter& reporter)
{
	xbase::x_Init();
#ifdef TARGET_DEBUG
	xcore::xasserthandler::sRegisterHandler(&gAssertHandler);
#endif

	xcore::TestAllocator testAllocator(xcore::xalloc::get_system());
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

