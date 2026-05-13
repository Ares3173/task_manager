// /src/process/modules.cpp
#include "task_manager/process/modules.hpp"

#include "task_manager/detail/nt_api.hpp"
#include "task_manager/process/process.hpp"

#include <utility>

namespace task_manager {

bool loaded_module::contains( address_t addr ) const noexcept {
	const auto a = std::to_underlying( addr );
	const auto b = std::to_underlying( base_ );
	return a >= b && ( a - b ) < size_;
}

auto loaded_module::read( std::size_t offset, std::span<std::byte> dst ) const
    -> std::expected<std::size_t, errc> {
	if ( !proc_ || !proc_->handle_ )
		return std::unexpected{ errc::invalid_handle };
	if ( offset >= size_ )
		return std::unexpected{ errc::bad_address };

	const std::size_t available = size_ - offset;
	if ( dst.size() > available )
		dst = dst.first( available );

	const auto addr = static_cast<address_t>(
	    std::to_underlying( base_ ) + static_cast<std::uintptr_t>( offset ) );
	return detail::nt::read_virtual_memory( proc_->handle_.get(), addr, dst );
}

} // namespace task_manager
