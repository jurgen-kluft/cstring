#include "cbase/c_base.h"
#include "cbase/c_allocator.h"
#include "cbase/c_console.h"
#include "cbase/c_context.h"

#include "cstring/c_string.h"

#include "cunittest/cunittest.h"
#include "cunittest/private/ut_ReportAssert.h"

UNITTEST_SUITE_LIST(xStringUnitTest);

UNITTEST_SUITE_DECLARE(xStringUnitTest, test_xstring);
//UNITTEST_SUITE_DECLARE(xStringUnitTest, test_string_reader);
//UNITTEST_SUITE_DECLARE(xStringUnitTest, test_string_writer);

namespace ncore
{
	class UnitTestAssertHandler : public ncore::asserthandler_t
	{
	public:
		UnitTestAssertHandler()
		{
			NumberOfAsserts = 0;
		}

		virtual bool	handle_assert(u32& flags, const char* fileName, s32 lineNumber, const char* exprString, const char* messageString)
		{
			UnitTest::reportAssert(exprString, fileName, lineNumber);
			NumberOfAsserts++;
			return false;
		}

		ncore::s32		NumberOfAsserts;
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
			mAllocator = nullptr;
		}
	};
}

ncore::UnitTestAssertHandler gAssertHandler;

bool gRunUnitTest(UnitTest::TestReporter& reporter)
{
	cbase::init();
	
#ifdef TARGET_DEBUG
	ncore::context_t::set_assert_handler(&gAssertHandler);
#endif

	ncore::alloc_t* system_alloc = ncore::alloc_t::get_system();
	ncore::TestAllocator testAllocator(system_alloc);
	ncore::context_t::set_system_alloc(&testAllocator);

	ncore::UnitTestAllocator unittestAllocator(&testAllocator);
	UnitTest::SetAllocator(&unittestAllocator);

	ncore::console->write("Configuration: ");
	ncore::console->writeLine(TARGET_FULL_DESCR_STR);

	int r = UNITTEST_SUITE_RUN(reporter, xStringUnitTest);

	UnitTest::SetAllocator(nullptr);
	ncore::context_t::set_system_alloc(system_alloc);

	cbase::exit();
	return r==0;
}

