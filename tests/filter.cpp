#include "display.cpp"
#include "extension.cpp"
#include <cmath>
#include <cstdio>
#include <cstdlib>
using namespace opxy;
using namespace opxy::extensions::master_dj_filter;
static void require(bool ok, const char *why) {
    if (!ok) {
        std::fprintf(stderr, "%s\n", why);
        std::exit(1);
    }
}
int main() {
    i32 l[64], r[64], parameters[8] = {0x40000000};
    filter.reset();
    filter.prepare(parameters);
    for (u32 i = 0; i < 64; ++i) {
        l[i] = static_cast<i32>(i * 19876) - 1234567;
        r[i] = -l[i];
    }
    filter.process(l, r);
    for (u32 i = 0; i < 64; ++i)
        require(l[i] == static_cast<i32>(i * 19876) - 1234567 && r[i] == -l[i], "exact neutral");
    DisplayState state = {};
    Frame frame = {};
    draw::Input input = {};
    input.elapsed = 17;
    display_reset(state);
    for (u32 sweep = 0; sweep < 1024; ++sweep) {
        parameters[0] = static_cast<i32>(sweep << 21);
        filter.prepare(parameters);
        for (u32 i = 0; i < 64; ++i) {
            l[i] = static_cast<i32>(std::sin(i * .21) * 0x10000000);
            r[i] = 0;
        }
        filter.process(l, r);
        for (u32 i = 0; i < 64; ++i)
            require(r[i] == 0, "stereo independence");
        input.slots[0] = parameters[0];
        display(state, input, frame);
        require(draw::valid(frame), "shared frame bounds");
        require(frame.nodes[6].shade < 16, "shared palette");
        for (u32 path = 0; path < 4; ++path) {
            i64 area = 0;
            for (u32 j = 0; j < 32; ++j) {
                const auto a = frame.paths[path].points[j], b = frame.paths[path].points[(j + 1) % 32];
                area += static_cast<i64>(a.x) * b.y - static_cast<i64>(b.x) * a.y;
            }
            require(area != 0, "Hidden and visible paths retain nondegenerate geometry");
        }
    }
    draw::parameter_reset(state, 0);
    input.slots[0] = 0x40000000;
    const i32 before = state.visual.value;
    display(state, input, frame);
    require(state.visual.value < before && state.visual.value > extensions::room::neutral, "smooth reset");
    for (u32 i = 0; i < 16; ++i)
        display(state, input, frame);
    require(state.visual.value == extensions::room::neutral, "reset ends at neutral");
    const u32 key=frame.paths[0].key;
    display(state,input,frame);
    require(frame.paths[0].key==key,"Stationary frames retain geometry keys");
    return 0;
}
