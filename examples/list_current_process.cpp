// Demonstrates the read-only query API by inspecting the running process
// itself. No arguments required.

#include "task_manager/process/process.hpp"

#include <cstdio>
#include <string_view>

using task_manager::arch_t;
using task_manager::errc;
using task_manager::process;
using task_manager::to_string;

namespace {

std::string_view arch_name(arch_t a) noexcept {
    switch (a) {
    case arch_t::x86:   return "x86";
    case arch_t::x64:   return "x64";
    case arch_t::arm64: return "arm64";
    }
    return "?";
}

} // namespace

int main() {
    auto p = process::current();
    if (!p) {
        std::fprintf(stderr, "process::current() failed: %.*s\n",
                     static_cast<int>(to_string(p.error()).size()),
                     to_string(p.error()).data());
        return 1;
    }

    std::printf("pid:    %u\n", static_cast<unsigned>(p->pid()));
    std::printf("arch:   %.*s\n",
                static_cast<int>(arch_name(p->architecture()).size()),
                arch_name(p->architecture()).data());

    if (auto n = p->name()) {
        std::printf("name:   %s\n", n->c_str());
    } else {
        std::printf("name:   <%.*s>\n",
                    static_cast<int>(to_string(n.error()).size()),
                    to_string(n.error()).data());
    }

    if (auto path = p->image_path()) {
        std::printf("path:   %s\n", path->string().c_str());
    } else {
        std::printf("path:   <%.*s>\n",
                    static_cast<int>(to_string(path.error()).size()),
                    to_string(path.error()).data());
    }

    if (auto parent = p->parent_pid()) {
        std::printf("parent: %u\n", static_cast<unsigned>(*parent));
    } else {
        std::printf("parent: <%.*s>\n",
                    static_cast<int>(to_string(parent.error()).size()),
                    to_string(parent.error()).data());
    }

    return 0;
}
