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
        const auto pose=extensions::room::pose(state.visual.value);
        const auto top=extensions::room::point(5,0,pose),bottom=extensions::room::point(5,1,pose);
        i32 actual_top=extensions::room::point(0,0,pose).y,actual_bottom=actual_top;
        for(u32 corner=1;corner<4;++corner) {
            const i32 y=extensions::room::point(0,corner,pose).y;
            if(y<actual_top)actual_top=y;
            if(y>actual_bottom)actual_bottom=y;
        }
        require(top.y==actual_top && bottom.y==actual_bottom,"Edge matches all projected door corners");
        if(extensions::room::visible(5,pose)) {
            require(!extensions::room::visible(0,pose),"Near-singular filled door stays hidden");
            require(frame.nodes[5].y==actual_top,"Edge top matches projected door top");
            const i32 rendered_bottom=frame.nodes[5].y+extensions::room::door_height*frame.nodes[5].scale/draw::unit;
            require(std::abs(rendered_bottom-actual_bottom)<=extensions::room::door_height/1024+2,
                    "Edge bottom matches projected height within Q10 scale rounding");
            require(frame.nodes[5].scale>0 && frame.nodes[5].scale<=2*draw::unit,
                    "Edge strip keeps a valid shared transform");
        }
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
