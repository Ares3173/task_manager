// /src/process/process.cpp
#include "task_manager/process/process.hpp"

#include "task_manager/detail/nt_api.hpp"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <cwctype>
#include <span>
#include <utility>

namespace task_manager {
namespace {

std::wstring widen_ascii( std::string_view s ) {
	return std::wstring( s.begin(), s.end() );
}

std::string narrow_ascii( std::wstring_view w ) {
	std::string out;
	out.reserve( w.size() );
	for ( wchar_t c : w )
		out.push_back( static_cast<char>( c & 0xFF ) );
	return out;
}

bool iequals_w( std::wstring_view a, std::wstring_view b ) noexcept {
	if ( a.size() != b.size() )
		return false;
	for ( std::size_t i = 0; i < a.size(); ++i ) {
		if ( std::towlower( a[ i ] ) != std::towlower( b[ i ] ) )
			return false;
	}
	return true;
}

#pragma pack( push, 8 )

struct unicode_string_64 {
	std::uint16_t length;         // bytes (not chars)
	std::uint16_t maximum_length; // bytes
	std::uint32_t padding;
	std::uint64_t buffer; // PWSTR in target process
};
static_assert( sizeof( unicode_string_64 ) == 16 );

struct list_entry_64 {
	std::uint64_t flink;
	std::uint64_t blink;
};
static_assert( sizeof( list_entry_64 ) == 16 );

struct peb_64_partial {
	std::uint8_t inherited_address_space;
	std::uint8_t read_image_file_exec_options;
	std::uint8_t being_debugged;
	std::uint8_t bit_field;
	std::uint8_t padding[ 4 ];
	std::uint64_t mutant;
	std::uint64_t image_base_address;
	std::uint64_t ldr; // PPEB_LDR_DATA
	                   // ... other fields not needed
};
static_assert( offsetof( peb_64_partial, ldr ) == 0x18 );

struct peb_ldr_data_64_partial {
	std::uint32_t length;
	std::uint8_t initialized;
	std::uint8_t padding1[ 3 ];
	std::uint64_t ss_handle;
	list_entry_64 in_load_order_module_list;
	list_entry_64 in_memory_order_module_list;
	list_entry_64 in_init_order_module_list;
};
static_assert( offsetof( peb_ldr_data_64_partial, in_load_order_module_list ) == 0x10 );

struct ldr_data_table_entry_64_partial {
	list_entry_64 in_load_order_links;
	list_entry_64 in_memory_order_links;
	list_entry_64 in_init_order_links;
	std::uint64_t dll_base;
	std::uint64_t entry_point;
	std::uint32_t size_of_image;
	std::uint8_t padding[ 4 ];
	unicode_string_64 full_dll_name;
	unicode_string_64 base_dll_name;
};
static_assert( offsetof( ldr_data_table_entry_64_partial, dll_base ) == 0x30 );
static_assert( offsetof( ldr_data_table_entry_64_partial, base_dll_name ) == 0x58 );

#pragma pack( pop )

template <typename T> std::expected<T, errc> read_struct( void* handle, address_t addr ) {
	T t{};
	auto r = detail::nt::read_virtual_memory(
	    handle, addr, std::span<std::byte>{ reinterpret_cast<std::byte*>( &t ), sizeof( T ) } );
	if ( !r )
		return std::unexpected{ r.error() };
	if ( *r != sizeof( T ) )
		return std::unexpected{ errc::bad_address };
	return t;
}

std::wstring read_unicode_string( void* handle, const unicode_string_64& us ) {
	if ( us.buffer == 0 || us.length == 0 )
		return {};
	std::wstring out( us.length / sizeof( wchar_t ), L'\0' );
	auto r = detail::nt::read_virtual_memory(
	    handle, static_cast<address_t>( us.buffer ),
	    std::span<std::byte>{ reinterpret_cast<std::byte*>( out.data() ), us.length } );
	if ( !r )
		return {};
	if ( *r < us.length )
		out.resize( *r / sizeof( wchar_t ) );
	return out;
}

struct raw_module {
	std::filesystem::path path;
	std::string name;
	address_t base;
	std::size_t size;
	address_t entry;
};

auto walk_loader_modules( void* handle, arch_t arch, address_t peb_base )
    -> std::expected<std::vector<raw_module>, errc> {
	if ( arch == arch_t::x86 )
		return std::unexpected{ errc::not_supported };

	auto peb = read_struct<peb_64_partial>( handle, peb_base );
	if ( !peb )
		return std::unexpected{ peb.error() };
	if ( peb->ldr == 0 )
		return std::unexpected{ errc::bad_address };

	auto ldr = read_struct<peb_ldr_data_64_partial>( handle, static_cast<address_t>( peb->ldr ) );
	if ( !ldr )
		return std::unexpected{ ldr.error() };

	const std::uint64_t list_head =
	    peb->ldr + offsetof( peb_ldr_data_64_partial, in_load_order_module_list );

	std::vector<raw_module> out;
	std::uint64_t cursor = ldr->in_load_order_module_list.flink;
	for ( int safety = 0; cursor != list_head && safety < 4096; ++safety ) {
		auto entry = read_struct<ldr_data_table_entry_64_partial>(
		    handle, static_cast<address_t>( cursor ) );
		if ( !entry )
			return std::unexpected{ entry.error() };

		// Some Windows versions terminate with a zero-base sentinel entry.
		if ( entry->dll_base == 0 ) {
			cursor = entry->in_load_order_links.flink;
			continue;
		}

		auto name_w = read_unicode_string( handle, entry->base_dll_name );
		auto path_w = read_unicode_string( handle, entry->full_dll_name );

		out.push_back(
		    raw_module{
		        .path  = std::filesystem::path{ path_w },
		        .name  = narrow_ascii( name_w ),
		        .base  = static_cast<address_t>( entry->dll_base ),
		        .size  = static_cast<std::size_t>( entry->size_of_image ),
		        .entry = static_cast<address_t>( entry->entry_point ) } );

		cursor = entry->in_load_order_links.flink;
	}

	return out;
}

} // namespace

process::process( detail::unique_handle handle, pid_t pid ) noexcept
    : handle_( std::move( handle ) ), pid_( pid ) {}

auto process::open( pid_t pid, access_rights rights ) -> std::expected<process, errc> {
	auto h = detail::nt::open_process( pid, rights );
	if ( !h )
		return std::unexpected{ h.error() };
	return process{ std::move( *h ), pid };
}

auto process::open( std::string_view name, access_rights rights ) -> std::expected<process, errc> {
	auto procs = detail::nt::query_system_processes();
	if ( !procs )
		return std::unexpected{ procs.error() };

	const auto wname = widen_ascii( name );
	for ( const auto& entry : *procs ) {
		if ( iequals_w( entry.image_name, wname ) )
			return open( entry.pid, rights );
	}
	return std::unexpected{ errc::invalid_pid };
}

auto process::create( std::filesystem::path /*path*/ ) -> std::expected<process, errc> {
	// Process creation has a much larger surface (NtCreateUserProcess +
	// RTL_USER_PROCESS_PARAMETERS, or CreateProcessW with a STARTUPINFO
	// shape). Deferred to a follow-up task.
	return std::unexpected{ errc::not_supported };
}

auto process::current() noexcept -> std::expected<process, errc> {
	return open( detail::nt::current_process_id(), access_rights::all_access );
}

bool process::is_alive() const noexcept {
	auto query_handle = detail::nt::open_process( pid_, access_rights::query_limited_info );
	if ( !query_handle )
		return false; // can't open — probably exited (could be access_denied without
		              // SeDebugPrivilege, but no better signal available here)

	auto info = detail::nt::query_process_basic_info( query_handle->get() );
	return info && info->exit_status == exit_code::still_active;
}

arch_t process::architecture() const noexcept {
	auto wow64 = detail::nt::query_process_is_wow64( handle_.get() );
	if ( wow64 && *wow64 )
		return arch_t::x86;
	return arch_t::x64;
}

auto process::name() const -> std::expected<std::string, errc> {
	auto path = image_path();
	if ( !path )
		return std::unexpected{ path.error() };
	return path->filename().string();
}

auto process::image_path() const -> std::expected<std::filesystem::path, errc> {
	if ( !handle_ )
		return std::unexpected{ errc::invalid_handle };
	return detail::nt::query_process_image_path_win32( handle_.get() );
}

auto process::parent_pid() const -> std::expected<pid_t, errc> {
	if ( !handle_ )
		return std::unexpected{ errc::invalid_handle };
	auto info = detail::nt::query_process_basic_info( handle_.get() );
	if ( !info )
		return std::unexpected{ info.error() };
	return info->parent_pid;
}

auto process::modules() const& -> std::expected<std::vector<loaded_module>, errc> {
	if ( !handle_ )
		return std::unexpected{ errc::invalid_handle };

	auto info = detail::nt::query_process_basic_info( handle_.get() );
	if ( !info )
		return std::unexpected{ info.error() };

	auto raw = walk_loader_modules( handle_.get(), architecture(), info->peb_base );
	if ( !raw )
		return std::unexpected{ raw.error() };

	std::vector<loaded_module> out;
	out.reserve( raw->size() );
	for ( auto& r : *raw ) {
		out.push_back(
		    loaded_module{
		        this, std::move( r.path ), std::move( r.name ), r.base, r.size, r.entry } );
	}
	return out;
}

auto process::terminate( unsigned exit_code ) -> std::expected<void, errc> {
	if ( !handle_ )
		return std::unexpected{ errc::invalid_handle };
	return detail::nt::terminate_process( handle_.get(), exit_code );
}

auto process::suspend() -> std::expected<void, errc> {
	if ( !handle_ )
		return std::unexpected{ errc::invalid_handle };
	return detail::nt::suspend_process( handle_.get() );
}

auto process::resume() -> std::expected<void, errc> {
	if ( !handle_ )
		return std::unexpected{ errc::invalid_handle };
	return detail::nt::resume_process( handle_.get() );
}

auto process::resume_internal() noexcept -> std::expected<void, errc> {
	if ( !handle_ )
		return std::unexpected{ errc::invalid_handle };
	return detail::nt::resume_process( handle_.get() );
}

auto process::suspend_scoped() -> std::expected<suspension, errc> {
	auto s = suspend();
	if ( !s )
		return std::unexpected{ s.error() };
	return suspension{ this };
}

auto process::add_job_object() -> std::expected<void, errc> {
	if ( !handle_ )
		return std::unexpected{ errc::invalid_handle };
	if ( job_ )
		return {}; // already added to a job object
	auto job = detail::nt::create_job_object();
	if ( !job )
		return std::unexpected{ job.error() };
	job_ = std::move( job.value() );
	return detail::nt::add_process_to_job_object( job_.get(), handle_.get() );
}

auto process::freeze() -> std::expected<void, errc> {
	if ( !handle_ )
		return std::unexpected{ errc::invalid_handle };
	if ( !job_ ) {
		auto add_result = add_job_object();
		if ( !add_result )
			return std::unexpected{ add_result.error() };
	}
	return detail::nt::set_job_object_freeze_info( job_.get(), true );
}

auto process::thaw() -> std::expected<void, errc> {
	if ( !handle_ )
		return std::unexpected{ errc::invalid_handle };
	if ( !job_ ) {
		auto add_result = add_job_object();
		if ( !add_result )
			return std::unexpected{ add_result.error() };
	}
	return detail::nt::set_job_object_freeze_info( job_.get(), false );
}

auto process::thaw_internal() noexcept -> std::expected<void, errc> {
	if ( !handle_ )
		return std::unexpected{ errc::invalid_handle };
	if ( !job_ ) {
		auto add_result = add_job_object();
		if ( !add_result )
			return std::unexpected{ add_result.error() };
	}
	return detail::nt::set_job_object_freeze_info( job_.get(), false );
}

auto process::freeze_scoped() -> std::expected<frozen, errc> {
	auto f = freeze();
	if ( !f )
		return std::unexpected{ f.error() };
	return frozen{ this };
}

auto process::terminate( pid_t pid, unsigned exit_code ) -> std::expected<void, errc> {
	auto p = open( pid, access_rights::terminate );
	if ( !p )
		return std::unexpected{ p.error() };
	return p->terminate( exit_code );
}

auto process::terminate( std::string_view name, unsigned exit_code ) -> std::expected<void, errc> {
	auto procs = detail::nt::query_system_processes();
	if ( !procs )
		return std::unexpected{ procs.error() };

	const auto wname = widen_ascii( name );
	bool found       = false;
	errc first_err{};
	for ( const auto& entry : *procs ) {
		if ( !iequals_w( entry.image_name, wname ) )
			continue;
		found  = true;
		auto r = terminate( entry.pid, exit_code );
		if ( !r && first_err == errc{} )
			first_err = r.error();
	}
	if ( !found )
		return std::unexpected{ errc::invalid_pid };
	if ( first_err != errc{} )
		return std::unexpected{ first_err };
	return {};
}

} // namespace task_manager
