// /include/task_manager/detail/unique_handle.hpp
#pragma once
#include <utility>

namespace task_manager::detail {

// Move-only RAII wrapper around an NT HANDLE.
// Sentinel is nullptr (NT semantics). Destructor calls NtClose; defined
// out-of-line in unique_handle.cpp so this header stays free of <windows.h>.
class unique_handle {
  public:
	using native_handle = void*;

	unique_handle() noexcept = default;
	explicit unique_handle( native_handle h ) noexcept : handle_( h ) {}

	unique_handle( const unique_handle& )            = delete;
	unique_handle& operator=( const unique_handle& ) = delete;

	unique_handle( unique_handle&& other ) noexcept
	    : handle_( std::exchange( other.handle_, nullptr ) ) {}

	unique_handle& operator=( unique_handle&& other ) noexcept {
		if ( this != &other ) {
			reset( std::exchange( other.handle_, nullptr ) );
		}
		return *this;
	}

	~unique_handle() {
		if ( handle_ )
			close( handle_ );
	}

	[[nodiscard]] native_handle get() const noexcept { return handle_; }

	[[nodiscard]] native_handle release() noexcept { return std::exchange( handle_, nullptr ); }

	void reset( native_handle h = nullptr ) noexcept {
		if ( handle_ )
			close( handle_ );
		handle_ = h;
	}

	explicit operator bool() const noexcept { return handle_ != nullptr; }

  private:
	// Defined in unique_handle.cpp; calls NtClose.
	static void close( native_handle h ) noexcept;

	native_handle handle_ = nullptr;
};

} // namespace task_manager::detail
