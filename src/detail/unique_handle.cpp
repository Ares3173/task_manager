// /src/detail/unique_handle.cpp
#include "task_manager/detail/unique_handle.hpp"

#include "task_manager/detail/nt_api.hpp"

namespace task_manager::detail {

void unique_handle::close( native_handle h ) noexcept {
	( void )nt::close( h );
}

} // namespace task_manager::detail
