#include "test.hpp"

int main() {
    int failed_cases = 0;
    for (const auto& c : t::cases()) {
        const int before = t::failures();
        c.fn();
        const bool ok = t::failures() == before;
        if (!ok) ++failed_cases;
        std::printf("%s %s\n", ok ? "[ ok ]" : "[FAIL]", c.name);
    }
    std::printf("\n%zu tests, %d failed\n", t::cases().size(), failed_cases);
    return failed_cases == 0 ? 0 : 1;
}
