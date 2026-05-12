// /include/task_manager/process/process.hpp
#pragma once
#include "task_manager/detail/unique_handle.hpp"
#include "task_manager/error.hpp"
#include "task_manager/types.hpp"

#include <cstdint>
#include <expected>
#include <filesystem>
#include <string_view>

namespace task_manager {
class process {
  public:
	class suspension; // Defined below

	// Construct
	static auto open( pid_t pid, access_rights rights ) -> std::expected<process, errc>;
	static auto open( std::string_view name, access_rights rights ) -> std::expected<process, errc>;
	static auto create( std::filesystem::path path ) -> std::expected<process, errc>;
	static auto current() noexcept -> std::expected<process, errc>;

	process( const process& )                = delete;
	process& operator=( const process& )     = delete;
	process( process&& ) noexcept            = default;
	process& operator=( process&& ) noexcept = default;
	~process()                               = default;

	// Info
	pid_t pid() const noexcept { return pid_; }
	bool is_open() const noexcept;
	arch_t architecture() const noexcept;
	auto name() const -> std::expected<std::string, errc>;
	auto image_path() const -> std::expected<std::filesystem::path, errc>;
	auto parent_pid() const -> std::expected<pid_t, errc>;

	// Children
	// auto threads() const -> std::expected<std::vector<thread>, errc>;
	// auto modules() const -> std::expected<std::vector<loaded_module>, errc>;
	// auto memory_regions() const -> std::expected<std::vector<memory_region>, errc>;

	// Lifecycle
	auto terminate( unsigned exit_code = 0 ) -> std::expected<void, errc>;
	auto suspend() -> std::expected<void, errc>;
	auto resume() -> std::expected<void, errc>;
	auto suspend_scoped() -> std::expected<suspension, errc>;

	static auto terminate( pid_t pid, unsigned exit_code = 0 ) -> std::expected<void, errc>;
	static auto terminate( std::string_view name, unsigned exit_code = 0 )
	    -> std::expected<void, errc>;

  private:
	process( detail::unique_handle handle, pid_t pid ) noexcept;

	auto resume_internal() noexcept -> std::expected<void, errc>;

	detail::unique_handle handle_;
	std::filesystem::path path_;
	pid_t pid_{};
};

class [[nodiscard]] process::suspension {
  public:
	suspension( const suspension& )            = delete;
	suspension& operator=( const suspension& ) = delete;

	suspension( suspension&& other ) noexcept : owner_( std::exchange( other.owner_, nullptr ) ) {}

	suspension& operator=( suspension&& other ) noexcept {
		if ( this != &other ) {
			if ( owner_ )
				( void )owner_->resume_internal(); // best-effort
			owner_ = std::exchange( other.owner_, nullptr );
		}
		return *this;
	}

	~suspension() {
		if ( owner_ )
			( void )owner_->resume_internal(); // best-effort, errors swallowed
	}

	auto resume() -> std::expected<void, errc> {
		if ( !owner_ )
			return std::unexpected{ errc::not_engaged };
		auto* p = std::exchange( owner_, nullptr );
		return p->resume_internal();
	}

	void release() noexcept { owner_ = nullptr; }

	bool engaged() const noexcept { return owner_ != nullptr; }
	explicit operator bool() const noexcept { return engaged(); }

  private:
	friend class process;
	explicit suspension( process* p ) noexcept : owner_( p ) {}
	process* owner_ = nullptr; // non-owning back-pointer
};

} // namespace task_manager
