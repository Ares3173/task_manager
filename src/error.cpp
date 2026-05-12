// /src/error.cpp
#include "task_manager/error.hpp"

namespace task_manager {

std::string_view to_string( errc e ) noexcept {
	switch ( e ) {
	case errc::ok:
		return "ok";
	case errc::access_denied:
		return "access_denied";
	case errc::invalid_pid:
		return "invalid_pid";
	case errc::process_exited:
		return "process_exited";
	case errc::invalid_handle:
		return "invalid_handle";
	case errc::insufficient_buffer:
		return "insufficient_buffer";
	case errc::not_supported:
		return "not_supported";
	case errc::bad_address:
		return "bad_address";
	case errc::unknown:
		return "unknown";
	}
	return "unknown";
}

namespace {
struct task_manager_category_t : std::error_category {
	[[nodiscard]] const char* name() const noexcept override { return "task_manager"; }
	[[nodiscard]] std::string message( int condition ) const override {
		return std::string{ to_string( static_cast<errc>( condition ) ) };
	}
};
} // anonymous namespace

const std::error_category& task_manager_category() noexcept {
	static const task_manager_category_t instance;
	return instance;
}

} // namespace task_manager
