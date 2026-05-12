#include "task_manager/types.hpp"

#include <catch2/catch_test_macros.hpp>

#include <type_traits>
#include <utility>

using task_manager::access_rights;
using task_manager::arch_t;
using task_manager::pid_t;
using task_manager::tid_t;

TEST_CASE("strong-typed ids do not implicitly convert", "[unit][types]") {
    STATIC_REQUIRE_FALSE(std::is_convertible_v<pid_t, tid_t>);
    STATIC_REQUIRE_FALSE(std::is_convertible_v<tid_t, pid_t>);
    STATIC_REQUIRE_FALSE(std::is_convertible_v<pid_t, std::uint32_t>);
    STATIC_REQUIRE(std::is_same_v<std::underlying_type_t<pid_t>, std::uint32_t>);
}

TEST_CASE("access_rights bitwise operators behave as a flag set", "[unit][types]") {
    constexpr auto rw = access_rights::vm_read | access_rights::vm_write;

    STATIC_REQUIRE((rw & access_rights::vm_read)  == access_rights::vm_read);
    STATIC_REQUIRE((rw & access_rights::vm_write) == access_rights::vm_write);
    STATIC_REQUIRE((rw & access_rights::terminate) == access_rights::none);

    // Composing further preserves earlier flags.
    constexpr auto rwt = rw | access_rights::terminate;
    STATIC_REQUIRE((rwt & access_rights::vm_read)   == access_rights::vm_read);
    STATIC_REQUIRE((rwt & access_rights::terminate) == access_rights::terminate);
}

TEST_CASE("arch_t enumerators are distinct", "[unit][types]") {
    STATIC_REQUIRE(arch_t::x86 != arch_t::x64);
    STATIC_REQUIRE(arch_t::x64 != arch_t::arm64);
    STATIC_REQUIRE(arch_t::x86 != arch_t::arm64);
}
