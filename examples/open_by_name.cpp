// Demonstrates process::open(name, …) and how errc surfaces through
// std::expected. Defaults to "explorer.exe" if no name is supplied.

#include "task_manager/process/process.hpp"
#include "task_manager/types.hpp"

#include <print>
#include <string_view>

using task_manager::access_rights;
using task_manager::process;
using task_manager::to_string;

int main( int argc, char** argv ) {
	std::string_view name = ( argc >= 2 ) ? argv[ 1 ] : "explorer.exe";

	auto p = process::open( name, access_rights::query_info );
	if ( !p ) {
		std::println( stderr, "open({}) -> {}", name, to_string( p.error() ) );
		return 1;
	}

	std::println( "found {}: pid={}", name, static_cast<unsigned>( p->pid() ) );

	if ( auto path = p->image_path() ) {
		std::println( "  path:   {}", path->string() );
	}
	if ( auto parent = p->parent_pid() ) {
		std::println( "  parent: {}", static_cast<unsigned>( *parent ) );
	}

	return 0;
}
