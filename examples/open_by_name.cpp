// Demonstrates process::open(name, …) and how errc surfaces through
// std::expected. Defaults to "explorer.exe" if no name is supplied.

#include "task_manager/process/process.hpp"
#include "task_manager/types.hpp"

#include <cstdio>
#include <string_view>

using task_manager::access_rights;
using task_manager::process;
using task_manager::to_string;

int main(int argc, char** argv) {
    std::string_view name = (argc >= 2) ? argv[1] : "explorer.exe";

    auto p = process::open(name, access_rights::query_info);
    if (!p) {
        auto msg = to_string(p.error());
        std::fprintf(stderr, "open(%.*s) -> %.*s\n",
                     static_cast<int>(name.size()), name.data(),
                     static_cast<int>(msg.size()), msg.data());
        return 1;
    }

    std::printf("found %.*s: pid=%u\n",
                static_cast<int>(name.size()), name.data(),
                static_cast<unsigned>(p->pid()));

    if (auto path = p->image_path()) {
        std::printf("  path:   %s\n", path->string().c_str());
    }
    if (auto parent = p->parent_pid()) {
        std::printf("  parent: %u\n", static_cast<unsigned>(*parent));
    }

    return 0;
}
