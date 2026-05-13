// /include/task_manager/detail/nt_api.hpp
#pragma once
#include "task_manager/detail/nt_types.hpp"
#include "task_manager/detail/unique_handle.hpp"
#include "task_manager/error.hpp"
#include "task_manager/types.hpp"

#include <cstddef>
#include <cstdint>
#include <expected>
#include <filesystem>
#include <span>
#include <string>
#include <vector>

namespace task_manager::detail::nt {

// ─── Lifecycle ─────────────────────────────────────────────────────────
auto open_process( pid_t pid, access_rights rights ) -> std::expected<unique_handle, errc>;

auto terminate_process( void* handle, unsigned exit_code ) -> std::expected<void, errc>;

auto suspend_process( void* handle ) -> std::expected<void, errc>;
auto resume_process( void* handle ) -> std::expected<void, errc>;

auto close( void* handle ) -> std::expected<void, errc>;

// Job Object

auto create_job_object() -> std::expected<unique_handle, errc>;
auto add_process_to_job_object( void* job, void* process ) -> std::expected<void, errc>;
auto set_job_object_information(
    void* job, JOBOBJECTINFOCLASS info_class, void* info, unsigned long info_len )
    -> std::expected<void, errc>;

auto set_job_object_freeze_info( void* job, bool freeze ) -> std::expected<void, errc>;

auto read_virtual_memory( void* handle, address_t addr, std::span<std::byte> dst )
    -> std::expected<std::size_t, errc>;

struct process_basic_info {
	pid_t pid;
	pid_t parent_pid;
	address_t peb_base;
	std::uint32_t exit_status;
};

struct object_basic_info {
	access_rights access;
	std::uint32_t attributes;
	std::uint32_t handle_count;
	std::uint32_t pointer_count;
};

auto query_process_basic_info( void* handle ) -> std::expected<process_basic_info, errc>;
auto query_handle_basic_info( void* handle ) -> std::expected<object_basic_info, errc>;

auto query_process_image_path_win32( void* handle ) -> std::expected<std::filesystem::path, errc>;

auto query_process_is_wow64( void* handle ) -> std::expected<bool, errc>;

auto query_handle_access_rights( void* handle ) -> std::expected<access_rights, errc>;

// Pid of the calling process. Cheap; never fails.
auto current_process_id() noexcept -> pid_t;

struct system_process_entry {
	pid_t pid;
	pid_t parent_pid;
	std::wstring image_name;
};

auto query_system_processes() -> std::expected<std::vector<system_process_entry>, errc>;

} // namespace task_manager::detail::nt
