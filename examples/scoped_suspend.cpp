// Demonstrates RAII process suspension. Opens the pid given on the
// command line, holds it suspended for N seconds, then lets the
// suspension RAII guard resume it on scope exit.
//
//   scoped_suspend <pid> [seconds=3]

#include "task_manager/process/process.hpp"
#include "task_manager/types.hpp"

#include <charconv>
#include <chrono>
#include <cstdio>
#include <string_view>
#include <thread>

using task_manager::access_rights;
using task_manager::pid_t;
using task_manager::process;
using task_manager::to_string;

namespace {

bool parse_uint(std::string_view sv, unsigned& out) {
    auto [ptr, ec] = std::from_chars(sv.data(), sv.data() + sv.size(), out);
    return ec == std::errc{} && ptr == sv.data() + sv.size();
}

} // namespace

int main(int argc, char** argv) {
    if (argc < 2) {
        std::fprintf(stderr, "usage: %s <pid> [seconds=3]\n", argv[0]);
        return 2;
    }

    unsigned raw_pid = 0;
    if (!parse_uint(argv[1], raw_pid)) {
        std::fprintf(stderr, "invalid pid: %s\n", argv[1]);
        return 2;
    }

    unsigned seconds = 3;
    if (argc >= 3 && !parse_uint(argv[2], seconds)) {
        std::fprintf(stderr, "invalid seconds: %s\n", argv[2]);
        return 2;
    }

    // suspend / resume require PROCESS_SUSPEND_RESUME (0x0800), which the
    // current access_rights enum doesn't name individually — request
    // all_access until a dedicated bit is added.
    auto p = process::open(pid_t{raw_pid}, access_rights::all_access);
    if (!p) {
        auto msg = to_string(p.error());
        std::fprintf(stderr, "open(%u) -> %.*s\n", raw_pid,
                     static_cast<int>(msg.size()), msg.data());
        return 1;
    }

    auto guard = p->suspend_scoped();
    if (!guard) {
        auto msg = to_string(guard.error());
        std::fprintf(stderr, "suspend_scoped -> %.*s\n",
                     static_cast<int>(msg.size()), msg.data());
        return 1;
    }

    std::printf("suspended pid %u for %u second(s)...\n", raw_pid, seconds);
    std::this_thread::sleep_for(std::chrono::seconds(seconds));
    std::printf("resuming on scope exit\n");
    // `guard` is dropped here; its destructor calls resume_internal().
    return 0;
}
