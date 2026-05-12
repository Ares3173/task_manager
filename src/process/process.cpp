// /src/process/process.cpp
#include "task_manager/process/process.hpp"

#include "task_manager/detail/nt_api.hpp"

#include <cwctype>
#include <utility>

namespace task_manager {
namespace {

// Process names are virtually always ASCII filenames ("notepad.exe"). Widen
// without codepage conversion so we can compare against UNICODE_STRING image
// names from NtQuerySystemInformation.
std::wstring widen_ascii( std::string_view s ) {
	return std::wstring( s.begin(), s.end() );
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

} // namespace

// ─── Construction ──────────────────────────────────────────────────────

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

// ─── Info ──────────────────────────────────────────────────────────────

bool process::is_open() const noexcept {
	return static_cast<bool>( handle_ );
}

arch_t process::architecture() const noexcept {
	// Best-effort: WOW64 flag distinguishes x86 from native on 64-bit hosts.
	// ARM64 detection requires ProcessImageInformation (machine field) and
	// is deferred until needed.
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

// ─── Lifecycle ─────────────────────────────────────────────────────────

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
