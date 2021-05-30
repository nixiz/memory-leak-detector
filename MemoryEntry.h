#pragma once
#include "malloc_allocator.hpp"
#include <thread>
#include <string>
#include <iostream>
#include <unordered_set>


using malloc_string = std::basic_string<char, std::char_traits<char>, malloc_allocator_t<char>>;
struct entry_t
{
	entry_t(void* allocated_ptr_)
		: allocated_ptr(allocated_ptr_)
	{ }
	entry_t(void* allocated_ptr_,
					size_t size_in_bytes_,
					std::thread::id thread_id_,
					malloc_string stack_dump_)
		: allocated_ptr(allocated_ptr_)
		, size_in_bytes(size_in_bytes_)
		, thread_id(thread_id_)
		, stack_dump(stack_dump_)
	{ }
	void* allocated_ptr{};
	size_t size_in_bytes{};
	std::thread::id thread_id{};
	malloc_string stack_dump{};
};

inline std::ostream& operator << (std::ostream& os, const entry_t& entry)
{
	os << "thrd: " << entry.thread_id << "\t"
		<< "ptr: " << entry.allocated_ptr << "\n"
		<< "-------stack trace-------\n"
		<< entry.stack_dump << "\n";
	return os;
}

inline bool operator == (const entry_t& lhs, const entry_t& rhs) { return lhs.allocated_ptr == rhs.allocated_ptr; }

struct new_entry_hash_t : std::hash<void*>
{
	using base = std::hash<void*>;
	std::size_t operator()(const entry_t& entry) const { return base::operator()(entry.allocated_ptr); }
};

using entry_set_t = std::unordered_set<
	entry_t,
	new_entry_hash_t,
	std::equal_to<entry_t>,
	malloc_allocator_t<entry_t>
>;