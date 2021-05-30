#pragma once
#include <boost/stacktrace/stacktrace.hpp>
#include "malloc_allocator.hpp"
#include "MemoryEntry.h"

namespace bs = boost::stacktrace;

inline auto/*malloc_string*/ get_stacktrace()
{
	using stacktrace = bs::basic_stacktrace<malloc_allocator_t<bs::frame>>;
	auto bt = stacktrace(4, 10);
	return bs::to_string(bt);
}


/*		
		//std::cout << bt << std::endl;
*/