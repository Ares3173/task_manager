// /include/task_manager/process/modules.hpp
#pragma once
#include "task_manager/error.hpp"
#include "task_manager/types.hpp"

#include <cstddef>
#include <expected>
#include <filesystem>
#include <span>
#include <string>
#include <string_view>

#ifdef TASK_MANAGER_WITH_PE_PARSER
#include <pe_parser/pe_parser.hpp>
#endif

namespace task_manager {

class process; // defined in process.hpp; loaded_module holds a non-owning ptr to one.

class loaded_module {
  public:
	[[nodiscard]] std::string_view name() const noexcept { return name_; }
	[[nodiscard]] const std::filesystem::path& path() const noexcept { return path_; }
	[[nodiscard]] address_t base() const noexcept { return base_; }
	[[nodiscard]] std::size_t size() const noexcept { return size_; }
	[[nodiscard]] address_t entry_point() const noexcept { return entry_point_; }

	[[nodiscard]] bool contains( address_t addr ) const noexcept;

	auto read( std::size_t offset, std::span<std::byte> dst ) const
	    -> std::expected<std::size_t, errc>;

#ifdef TASK_MANAGER_WITH_PE_PARSER
	auto headers() const -> std::expected<pe_parser::headers, errc>;
	auto sections() const -> std::expected<std::vector<pe_parser::section>, errc>;
	auto exports() const -> std::expected<std::vector<pe_parser::export_e>, errc>;
	auto imports() const -> std::expected<std::vector<pe_parser::import_e>, errc>;
	auto find_export( std::string_view name ) const -> std::expected<address_t, errc>;
#endif

	loaded_module()                                      = default;
	loaded_module( const loaded_module& )                = default;
	loaded_module( loaded_module&& ) noexcept            = default;
	loaded_module& operator=( const loaded_module& )     = default;
	loaded_module& operator=( loaded_module&& ) noexcept = default;
	~loaded_module()                                     = default;

  private:
	friend class process;

	loaded_module(
	    const process* proc, std::filesystem::path path, std::string name, address_t base,
	    std::size_t size, address_t entry ) noexcept
	    : proc_( proc ), path_( std::move( path ) ), name_( std::move( name ) ), base_( base ),
	      size_( size ), entry_point_( entry ) {}

	const process* proc_ = nullptr; // non-owning; must outlive *this
	std::filesystem::path path_;
	std::string name_;
	address_t base_{};
	std::size_t size_{};
	address_t entry_point_{};
};

} // namespace task_manager
