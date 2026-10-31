#pragma once
// Minimal test harness (no dependencies). TEST(name) { CHECK(expr); CHECK_NEAR(a, b, eps); }
#include <cmath>
#include <cstdio>
#include <functional>
#include <string>
#include <vector>

namespace t {
struct Case { const char* name; std::function<void()> fn; };
inline std::vector<Case>& cases() { static std::vector<Case> c; return c; }
inline int& failures() { static int f = 0; return f; }
struct Reg { Reg(const char* n, std::function<void()> f) { cases().push_back({n, std::move(f)}); } };
}  // namespace t

#define TEST_CAT2(a, b) a##b
#define TEST_CAT(a, b) TEST_CAT2(a, b)
#define TEST(name) \
    static void name(); \
    static t::Reg TEST_CAT(reg_, name)(#name, name); \
    static void name()
#define CHECK(expr) \
    do { if (!(expr)) { ++t::failures(); std::printf("  FAIL %s:%d: %s\n", __FILE__, __LINE__, #expr); } } while (0)
#define CHECK_NEAR(a, b, eps) \
    do { const double va_ = (a), vb_ = (b); if (std::fabs(va_ - vb_) > (eps)) { ++t::failures(); \
        std::printf("  FAIL %s:%d: %s = %g, expected %g\n", __FILE__, __LINE__, #a, va_, vb_); } } while (0)
