// /include/task_manager/process/process.hpp
#pragma once
#include "task_manager/error.hpp"

#include <cstdint>
#include <expected>
#include <filesystem>
#include <string_view>

namespace task_manager {
class process {
  public:
	// Construct
	static auto open( std::uint32_t pid, std::uint32_t rights ) -> std::expected<process, errc>;
	static auto open( std::string_view name, std::uint32_t rights ) -> std::expected<process, errc>;
	static auto create( std::filesystem::path path ) -> std::expected<process, errc>;
	static auto current() noexcept -> std::expected<process, errc>;

	process( const process& )                = delete;
	process& operator=( const process& )     = delete;
	process( process&& ) noexcept            = default;
	process& operator=( process&& ) noexcept = default;
	~process()                               = default;

	// Info
	std::uint32_t pid() const noexcept { return pid_; }
	bool is_open() const noexcept;
	auto name() const -> std::expected<std::wstring, errc>;
	auto image_path() const -> std::expected<std::filesystem::path, errc>;
	auto parent_pid() const -> std::expected<std::uint32_t, errc>;

	// Children
	// auto threads() const -> std::expected<std::vector<thread>, errc>;
	// auto modules() const -> std::expected<std::vector<loaded_module>, errc>;
	// auto memory_regions() const -> std::expected<std::vector<memory_region>, errc>;

	// Lifecycle
	auto terminate( unsigned exit_code = 0 ) -> std::expected<void, errc>;
	static auto terminate( std::uint32_t pid, unsigned exit_code = 0 ) -> std::expected<void, errc>;
	static auto terminate( std::string_view name, unsigned exit_code = 0 )
	    -> std::expected<void, errc>;

  private:
	process( detail::unique_handle handle, std::uint32_t pid ) noexcept;

	detail::unique_handle handle_;
	std::filesystem::path path_;
	std::string_view name_;
	std::uint32_t pid_ = 0;
};
} // namespace task_manager
