// /src/detail/nt_api.cpp
#include "task_manager/detail/nt_api.hpp"

#define WIN32_NO_STATUS
#include <windows.h>
#undef WIN32_NO_STATUS

#include <cstddef>
#include <ntstatus.h>
#include <utility>
#include <vector>
#include <winternl.h>

// ─── ntdll exports not in <winternl.h> ─────────────────────────────────
extern "C" {
NTSTATUS NTAPI NtClose( HANDLE Handle );

NTSTATUS NTAPI NtOpenProcess(
    PHANDLE ProcessHandle, ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes,
    CLIENT_ID* ClientId );

NTSTATUS NTAPI NtTerminateProcess( HANDLE ProcessHandle, NTSTATUS ExitStatus );
NTSTATUS NTAPI NtSuspendProcess( HANDLE ProcessHandle );
NTSTATUS NTAPI NtResumeProcess( HANDLE ProcessHandle );
}

namespace task_manager::detail::nt {
namespace {

// ─── NTSTATUS → errc ───────────────────────────────────────────────────
errc nt_to_errc( NTSTATUS s ) noexcept {
	switch ( s ) {
	case STATUS_SUCCESS:
		return errc::ok;
	case STATUS_ACCESS_DENIED:
		return errc::access_denied;
	case STATUS_INVALID_CID:
	case STATUS_INVALID_PARAMETER:
		return errc::invalid_pid;
	case STATUS_PROCESS_IS_TERMINATING:
		return errc::process_exited;
	case STATUS_INVALID_HANDLE:
		return errc::invalid_handle;
	case STATUS_INFO_LENGTH_MISMATCH:
	case STATUS_BUFFER_TOO_SMALL:
	case STATUS_BUFFER_OVERFLOW:
		return errc::insufficient_buffer;
	case STATUS_NOT_SUPPORTED:
	case STATUS_NOT_IMPLEMENTED:
		return errc::not_supported;
	case STATUS_ACCESS_VIOLATION:
		return errc::bad_address;
	default:
		return errc::unknown;
	}
}

HANDLE pid_to_handle( pid_t pid ) noexcept {
	return reinterpret_cast<HANDLE>( static_cast<std::uintptr_t>( std::to_underlying( pid ) ) );
}

pid_t handle_to_pid( HANDLE h ) noexcept {
	return static_cast<pid_t>(
	    static_cast<std::uint32_t>( reinterpret_cast<std::uintptr_t>( h ) ) );
}

// PROCESSINFOCLASS values not in <winternl.h>'s reduced enum.
constexpr auto kProcessImageFileNameWin32 = static_cast<PROCESSINFOCLASS>( 43 );

} // namespace

// ─── Lifecycle ─────────────────────────────────────────────────────────

auto open_process( pid_t pid, access_rights rights ) -> std::expected<unique_handle, errc> {
	OBJECT_ATTRIBUTES oa{};
	InitializeObjectAttributes( &oa, nullptr, 0, nullptr, nullptr );

	CLIENT_ID cid{};
	cid.UniqueProcess = pid_to_handle( pid );
	cid.UniqueThread  = nullptr;

	HANDLE h = nullptr;
	const NTSTATUS status =
	    NtOpenProcess( &h, static_cast<ACCESS_MASK>( std::to_underlying( rights ) ), &oa, &cid );

	if ( !NT_SUCCESS( status ) )
		return std::unexpected{ nt_to_errc( status ) };

	return unique_handle{ h };
}

auto terminate_process( void* handle, unsigned exit_code ) -> std::expected<void, errc> {
	const NTSTATUS status =
	    NtTerminateProcess( static_cast<HANDLE>( handle ), static_cast<NTSTATUS>( exit_code ) );
	if ( !NT_SUCCESS( status ) )
		return std::unexpected{ nt_to_errc( status ) };
	return {};
}

auto suspend_process( void* handle ) -> std::expected<void, errc> {
	const NTSTATUS status = NtSuspendProcess( static_cast<HANDLE>( handle ) );
	if ( !NT_SUCCESS( status ) )
		return std::unexpected{ nt_to_errc( status ) };
	return {};
}

auto resume_process( void* handle ) -> std::expected<void, errc> {
	const NTSTATUS status = NtResumeProcess( static_cast<HANDLE>( handle ) );
	if ( !NT_SUCCESS( status ) )
		return std::unexpected{ nt_to_errc( status ) };
	return {};
}

auto close_handle( void* handle ) -> std::expected<void, errc> {
	const NTSTATUS status = NtClose( static_cast<HANDLE>( handle ) );
	if ( !NT_SUCCESS( status ) )
		return std::unexpected{ nt_to_errc( status ) };
	return {};
}

// ─── Query ─────────────────────────────────────────────────────────────

auto query_process_basic_info( void* handle ) -> std::expected<process_basic_info, errc> {
	PROCESS_BASIC_INFORMATION pbi{};
	ULONG ret_len         = 0;
	const NTSTATUS status = NtQueryInformationProcess(
	    static_cast<HANDLE>( handle ), ProcessBasicInformation, &pbi, sizeof( pbi ), &ret_len );

	if ( !NT_SUCCESS( status ) )
		return std::unexpected{ nt_to_errc( status ) };

	// winternl.h aliases: Reserved1 = ExitStatus, Reserved3 = InheritedFromUniqueProcessId.
	process_basic_info out{};
	out.pid        = static_cast<pid_t>( pbi.UniqueProcessId );
	out.parent_pid = static_cast<pid_t>( reinterpret_cast<std::uintptr_t>( pbi.Reserved3 ) );
	out.peb_base = static_cast<address_t>( reinterpret_cast<std::uintptr_t>( pbi.PebBaseAddress ) );
	out.exit_status = static_cast<std::int32_t>( reinterpret_cast<std::intptr_t>( pbi.Reserved1 ) );
	return out;
}

auto query_process_image_path_win32( void* handle ) -> std::expected<std::filesystem::path, errc> {
	// First call probes the required size. Most paths comfortably fit in
	// a small starting buffer — try once, grow on length-mismatch.
	std::vector<std::byte> buf( sizeof( UNICODE_STRING ) + MAX_PATH * sizeof( WCHAR ) );
	ULONG ret_len   = 0;
	NTSTATUS status = NtQueryInformationProcess(
	    static_cast<HANDLE>( handle ), kProcessImageFileNameWin32, buf.data(),
	    static_cast<ULONG>( buf.size() ), &ret_len );

	if ( status == STATUS_INFO_LENGTH_MISMATCH || status == STATUS_BUFFER_TOO_SMALL ||
	     status == STATUS_BUFFER_OVERFLOW ) {
		buf.resize( ret_len );
		status = NtQueryInformationProcess(
		    static_cast<HANDLE>( handle ), kProcessImageFileNameWin32, buf.data(),
		    static_cast<ULONG>( buf.size() ), &ret_len );
	}

	if ( !NT_SUCCESS( status ) )
		return std::unexpected{ nt_to_errc( status ) };

	const auto* us = reinterpret_cast<const UNICODE_STRING*>( buf.data() );
	if ( !us->Buffer || us->Length == 0 )
		return std::filesystem::path{};

	const std::wstring_view sv{ us->Buffer, us->Length / sizeof( WCHAR ) };
	return std::filesystem::path{ sv };
}

auto query_process_is_wow64( void* handle ) -> std::expected<bool, errc> {
	ULONG_PTR wow64       = 0;
	ULONG ret_len         = 0;
	const NTSTATUS status = NtQueryInformationProcess(
	    static_cast<HANDLE>( handle ), ProcessWow64Information, &wow64, sizeof( wow64 ), &ret_len );
	if ( !NT_SUCCESS( status ) )
		return std::unexpected{ nt_to_errc( status ) };
	return wow64 != 0;
}

auto current_process_id() noexcept -> pid_t {
	return static_cast<pid_t>( ::GetCurrentProcessId() );
}

// ─── Enumeration ───────────────────────────────────────────────────────

auto query_system_processes() -> std::expected<std::vector<system_process_entry>, errc> {
	// Snapshot can be megabytes on a busy system. Start at 256 KiB and
	// double until it fits, capped to a sane retry count.
	std::vector<std::byte> buf( 256 * 1024 );
	ULONG ret_len   = 0;
	NTSTATUS status = STATUS_INFO_LENGTH_MISMATCH;

	for ( int attempt = 0; attempt < 8; ++attempt ) {
		status = NtQuerySystemInformation(
		    SystemProcessInformation, buf.data(), static_cast<ULONG>( buf.size() ), &ret_len );
		if ( status != STATUS_INFO_LENGTH_MISMATCH )
			break;
		const std::size_t grown = ret_len > buf.size() ? ret_len + 16 * 1024 : buf.size() * 2;
		buf.resize( grown );
	}

	if ( !NT_SUCCESS( status ) )
		return std::unexpected{ nt_to_errc( status ) };

	std::vector<system_process_entry> out;
	const std::byte* cursor = buf.data();
	while ( true ) {
		const auto* spi = reinterpret_cast<const SYSTEM_PROCESS_INFORMATION*>( cursor );

		system_process_entry e{};
		e.pid = static_cast<pid_t>( reinterpret_cast<std::uintptr_t>( spi->UniqueProcessId ) );
		// winternl.h aliases: Reserved2 = InheritedFromUniqueProcessId.
		e.parent_pid = static_cast<pid_t>( reinterpret_cast<std::uintptr_t>( spi->Reserved2 ) );
		if ( spi->ImageName.Buffer && spi->ImageName.Length > 0 ) {
			e.image_name.assign( spi->ImageName.Buffer, spi->ImageName.Length / sizeof( WCHAR ) );
		}
		out.push_back( std::move( e ) );

		if ( spi->NextEntryOffset == 0 )
			break;
		cursor += spi->NextEntryOffset;
	}
	return out;
}

} // namespace task_manager::detail::nt
