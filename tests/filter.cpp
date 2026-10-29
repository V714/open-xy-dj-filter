#include "extension.cpp"
#include <cstdio>
#include <cstdlib>
#include <cmath>
using namespace opxy;
static void require(bool ok, const char *why) {
    if (!ok) {
        std::fprintf(stderr, "%s\n", why);
        std::exit(1);
    }
}
namespace {
u32 path_count, gray_count;
bool path(void *, u32 index, const master::Point *points, u32 count, bool visible) {
    require(index < 7 && count >= 2 && count <= 32, "path topology");
    if (visible)
        for (u32 i = 0; i < count; ++i)
            require(points[i].x >= 0 && points[i].x <= 480 * 1024 && points[i].y >= 0 &&
                        points[i].y <= 222 * 1024,
                    "screen bounds");
    ++path_count;
    return true;
}
bool gray(void *, u32 value) {
    require(value <= 236, "gray range");
    ++gray_count;
    return true;
}
} // namespace
int main() {
    const auto &filter = extensions::dj::filter;
    i32 left[64], right[64];
    filter.reset();
    filter.prepare(512);
    for (u32 i = 0; i < 64; ++i) {
        left[i] = static_cast<i32>(i * 19876) - 1234567;
        right[i] = -left[i];
    }
    filter.process(left, right);
    for (u32 i = 0; i < 64; ++i)
        require(left[i] == static_cast<i32>(i * 19876) - 1234567 && right[i] == -left[i], "exact neutral");
    for (u32 sweep = 0; sweep < 1024; ++sweep) {
        filter.prepare(sweep);
        for (u32 i = 0; i < 64; ++i) {
            left[i] = static_cast<i32>(std::sin(i * .21) * 0x10000000);
            right[i] = 0;
        }
        filter.process(left, right);
        for (u32 i = 0; i < 64; ++i)
            require(right[i] == 0, "stereo independence");
        const master::Scene scene = {nullptr, path, gray};
        filter.panel->frame(scene, master::Input{sweep, 17, 1, 0, sweep});
    }
    require(path_count == 7 * 1024 && gray_count == 1024, "complete frames");
    const master::Scene scene = {nullptr, path, gray};
    filter.panel->frame(scene, master::Input{0, 17, 1, 0, 1024});
    filter.panel->frame(scene, master::Input{512, 17, 1, 1, 1024});
    for (u32 i = 0; i < 10; ++i)
        filter.panel->frame(scene, master::Input{512, 17, 1, 1, 1024});
    require(extensions::dj::visual.value == 50 * 1024, "smooth reset reaches neutral");
    return 0;
}
