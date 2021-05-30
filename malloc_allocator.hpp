#pragma once
#include <memory>

template<typename T>
struct malloc_allocator_t : std::allocator<T>
{
	malloc_allocator_t() = default;

	template<class U>
	malloc_allocator_t(const malloc_allocator_t<U>&) noexcept {}

	T* allocate(std::size_t n)
	{
		T* ptr = (T*)std::malloc(n * sizeof(T));
		if (!ptr) throw std::bad_alloc();
		return ptr;
	}

	void deallocate(T* ptr, std::size_t) { std::free(ptr); }

	template<typename U>
	struct rebind { typedef malloc_allocator_t<U> other; };
};

/*
		void* raw = std::malloc(sizeof(entry_set_t));
		if (!raw) throw std::bad_alloc();
		entry_table = new (raw) entry_set_t;
*/

template <typename T, typename ...Args>
T* malloc_new(Args&& ...args)
{
	void* raw = std::malloc(sizeof(T));
	if (!raw) throw std::bad_alloc();
	return new(raw) T{std::forward<Args>(args)...};
}

template <typename T, typename ...Args>
void malloc_create(T** ptr, Args&& ...args)
{
	*ptr = malloc_new<T>(std::forward<Args>(args)...);
}


template <typename T>
void free_new(T* ptr) 
{
	if (!ptr) 
		return;
	ptr->~T();
	std::free(ptr);
}


