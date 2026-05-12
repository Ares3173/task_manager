// /include/task_manager/error.hpp
#pragma once
#include <cstdint>
#include <string_view>
#include <system_error>

namespace task_manager {

enum class errc : std::uint8_t {
	ok = 0,
	access_denied,       // need higher privileges
	invalid_pid,         // no process with that ID
	process_exited,      // died between open and operation
	invalid_handle,      // handle was null or already closed
	insufficient_buffer, // common NT API pattern: retry with larger buffer
	not_supported,       // protected process, OS version mismatch
	bad_address,         // memory read/write hit an unmapped page
	not_engaged,
	unknown,
};

std::string_view to_string( errc e ) noexcept;

const std::error_category& task_manager_category() noexcept;

inline std::error_code make_error_code( errc e ) noexcept {
	return { static_cast<int>( e ), task_manager_category() };
}

} // namespace task_manager

template <> struct std::is_error_code_enum<task_manager::errc> : std::true_type {};
