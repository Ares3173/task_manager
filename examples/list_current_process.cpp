// Demonstrates the read-only query API by inspecting the running process
// itself. No arguments required.

#include "task_manager/process/process.hpp"

#include <print>
#include <string_view>

using task_manager::arch_t;
using task_manager::errc;
using task_manager::process;
using task_manager::to_string;

namespace {

std::string_view arch_name( arch_t a ) noexcept {
	switch ( a ) {
	case arch_t::x86:
		return "x86";
	case arch_t::x64:
		return "x64";
	case arch_t::arm64:
		return "arm64";
	}
	return "?";
}

} // namespace

int main() {
	auto p = process::current();
	if ( !p ) {
		std::println( stderr, "process::current() failed: {}", to_string( p.error() ) );
		return 1;
	}

	std::println( "pid:    {}", static_cast<unsigned>( p->pid() ) );
	std::println( "arch:   {}", arch_name( p->architecture() ) );

	if ( auto n = p->name() ) {
		std::println( "name:   {}", *n );
	} else {
		std::println( "name:   <{}>", to_string( n.error() ) );
	}

	if ( auto path = p->image_path() ) {
		std::println( "path:   {}", path->string() );
	} else {
		std::println( "path:   <{}>", to_string( path.error() ) );
	}

	if ( auto parent = p->parent_pid() ) {
		std::println( "parent: {}", static_cast<unsigned>( *parent ) );
	} else {
		std::println( "parent: <{}>", to_string( parent.error() ) );
	}

	return 0;
}
