// /include/task_manager/process/process.hpp
#pragma once
#include "task_manager/detail/unique_handle.hpp"
#include "task_manager/error.hpp"
#include "task_manager/process/modules.hpp"
#include "task_manager/types.hpp"

#include <cstdint>
#include <expected>
#include <filesystem>
#include <string_view>


namespace task_manager {
class process {
  public:
	class suspension; // Defined below
	class frozen;     // Defined below

	enum class exit_code : std::uint32_t;

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
	bool is_alive() const noexcept;
	arch_t architecture() const noexcept;
	auto name() const -> std::expected<std::string, errc>;
	auto image_path() const -> std::expected<std::filesystem::path, errc>;
	auto parent_pid() const -> std::expected<pid_t, errc>;

	// auto threads() const -> std::expected<std::vector<thread>, errc>;
	// auto modules() const -> std::expected<std::vector<loaded_module>, errc>;

	// Modules
	auto modules() const& -> std::expected<std::vector<loaded_module>, errc>;
	auto modules() const&& -> std::expected<std::vector<loaded_module>, errc> = delete;
	// auto memory_regions() const -> std::expected<std::vector<memory_region>, errc>;

	// Lifecycle
	auto terminate( unsigned exit_code = 0 ) -> std::expected<void, errc>;

	auto suspend() -> std::expected<void, errc>;
	auto resume() -> std::expected<void, errc>;
	auto suspend_scoped() -> std::expected<suspension, errc>;

	auto add_job_object() -> std::expected<void, errc>;

	auto freeze() -> std::expected<void, errc>;
	auto thaw() -> std::expected<void, errc>;
	auto freeze_scoped() -> std::expected<frozen, errc>;

	static auto terminate( pid_t pid, unsigned exit_code = 0 ) -> std::expected<void, errc>;
	static auto terminate( std::string_view name, unsigned exit_code = 0 )
	    -> std::expected<void, errc>;

  private:
	friend class loaded_module;
	process( detail::unique_handle handle, pid_t pid ) noexcept;

	auto resume_internal() noexcept -> std::expected<void, errc>;
	auto thaw_internal() noexcept -> std::expected<void, errc>;

	detail::unique_handle handle_;
	detail::unique_handle job_;
	std::filesystem::path path_;
	pid_t pid_{};
};

enum class process::exit_code : std::uint32_t {
	still_active = 0x00000103, // STATUS_PENDING

	access_violation      = 0xC0000005,
	datatype_misalignment = 0x80000002,
	breakpoint            = 0x80000003,
	single_step           = 0x80000004,
	array_bounds_exceeded = 0xC000008C,

	flt_denormal_operand  = 0xC000008D,
	flt_divide_by_zero    = 0xC000008E,
	flt_inexact_result    = 0xC000008F,
	flt_invalid_operation = 0xC0000090,
	flt_overflow          = 0xC0000091,
	flt_stack_check       = 0xC0000092,
	flt_underflow         = 0xC0000093,

	int_divide_by_zero = 0xC0000094,
	int_overflow       = 0xC0000095,

	priv_instruction         = 0xC0000096,
	in_page_error            = 0xC0000006,
	illegal_instruction      = 0xC000001D,
	noncontinuable_exception = 0xC0000025,
	stack_overflow           = 0xC00000FD,
	invalid_disposition      = 0xC0000026,
	guard_page               = 0x80000001, // STATUS_GUARD_PAGE_VIOLATION
	invalid_handle           = 0xC0000008,
	possible_deadlock        = 0xC0000194,

	control_c_exit = 0xC000013A,
};

constexpr bool operator==( process::exit_code lhs, std::uint32_t rhs ) noexcept {
	return static_cast<std::uint32_t>( lhs ) == rhs;
}

constexpr std::strong_ordering operator<=>( process::exit_code lhs, std::uint32_t rhs ) noexcept {
	return static_cast<std::uint32_t>( lhs ) <=> rhs;
}

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

class [[nodiscard]] process::frozen {
  public:
	frozen( const frozen& )            = delete;
	frozen& operator=( const frozen& ) = delete;

	frozen( frozen&& other ) noexcept : owner_( std::exchange( other.owner_, nullptr ) ) {}

	frozen& operator=( frozen&& other ) noexcept {
		if ( this != &other ) {
			if ( owner_ )
				( void )owner_->thaw_internal(); // best-effort
			owner_ = std::exchange( other.owner_, nullptr );
		}
		return *this;
	}

	~frozen() {
		if ( owner_ )
			( void )owner_->thaw_internal(); // best-effort, errors swallowed
	}

	auto thaw() -> std::expected<void, errc> {
		if ( !owner_ )
			return std::unexpected{ errc::not_engaged };
		auto* p = std::exchange( owner_, nullptr );
		return p->thaw_internal();
	}

	void release() noexcept { owner_ = nullptr; }

	bool engaged() const noexcept { return owner_ != nullptr; }
	explicit operator bool() const noexcept { return engaged(); }

  private:
	friend class process;
	explicit frozen( process* p ) noexcept : owner_( p ) {}
	process* owner_ = nullptr; // non-owning back-pointer
};

} // namespace task_manager
