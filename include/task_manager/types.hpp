// /include/task_manager/types.hpp
#pragma once
#include <cstdint>

namespace task_manager {

enum class pid_t : std::uint32_t {};
enum class tid_t : std::uint32_t {};
enum class address_t : std::uintptr_t {}; // for memory addresses in the target process
enum class arch_t { x86, x64, arm64 };

enum class access_rights : std::uint32_t {
	none          = 0,
	terminate     = 0x0001,
	create_thread = 0x0002,
	vm_operation  = 0x0008,
	vm_read       = 0x0010,
	vm_write      = 0x0020,
	query_info    = 0x0400,
	all_access    = 0x000F0000 | 0x00100000 | 0xFFFF,
};

constexpr access_rights operator|( access_rights a, access_rights b ) noexcept {
	return access_rights{ std::to_underlying( a ) | std::to_underlying( b ) };
}
constexpr access_rights operator&( access_rights a, access_rights b ) noexcept {
	return access_rights{ std::to_underlying( a ) & std::to_underlying( b ) };
}

} // namespace task_manager
