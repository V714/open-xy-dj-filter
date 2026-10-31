#include "display.h"
namespace opxy {
namespace extensions {
namespace master_dj_filter {
namespace {
const i32 unit = draw::unit;
draw::Point room_point(u32 shape, u32 index, const room::Pose &pose) {
    const room::Point p = room::point(shape, index, pose);
    return {p.x, p.y};
}
void outline(Frame &frame, u32 slot, const draw::Point *points, u32 count, u32 key) {
    frame.paths[slot].key = key;
    // Even subdivision of each edge keeps the shared fixed topology distinct.
    for (u32 j = 0; j < 32; ++j) {
        const u32 edge = j * count / 32, next = (edge + 1) % count;
        const u32 start = (edge * 32 + count - 1) / count, end = ((edge + 1) * 32 + count - 1) / count;
        const i32 t = static_cast<i32>((j - start) * 1024 / (end - start));
        const i32 x = points[edge].x + (points[next].x - points[edge].x) * t / 1024;
        const i32 y = points[edge].y + (points[next].y - points[edge].y) * t / 1024;
        frame.paths[slot].points[j] = {(x - 240 * unit) / 2, (y - 110 * unit) / 2};
    }
}
void segment(Frame &frame, u32 node, draw::Point a, draw::Point b) {
    const i32 dx = b.x - a.x, dy = b.y - a.y;
    // Integer square root; GUI only, bounded to at most 24 iterations.
    u32 n = static_cast<u32>((dx / 32) * (dx / 32) + (dy / 32) * (dy / 32)), root = 0, bit = 1u << 30;
    while (bit > n)
        bit >>= 2;
    while (bit) {
        if (n >= root + bit) {
            n -= root + bit;
            root = (root >> 1) + bit;
        } else
            root >>= 1;
        bit >>= 2;
    }
    const i32 length = static_cast<i32>(root) * 32;
    if (length < 64) {
        draw::hide(frame, node);
        return;
    }
    draw::place(frame, node, a.x, a.y, length / 64, 0, draw::keep);
    draw::turn(frame, node,
               {draw::clamp(dx * 1024 / length, -1024, 1024), draw::clamp(dy * 1024 / length, -1024, 1024)});
}
} // namespace
void display_reset(DisplayState &state) { state = {}; }
void display_parameter_reset(DisplayState &state, u32 slot) {
    if (!slot)
        state.visual.reset();
}
void display(DisplayState &state, const draw::Input &input, Frame &frame) {
    const i32 previous = state.visual.value;
    state.visual.step(static_cast<u32>(input.slots[0]) >> 21, input.elapsed);
    if (state.key && state.visual.value == previous)
        return;
    const room::Pose pose = room::pose(state.visual.value);
    if (!++state.key)
        ++state.key;
    const u32 shapes[4] = {0, 1, 2, 3};
    const u32 shades[4] = {3, 6, 15, 3};
    for (u32 slot = 0; slot < 4; ++slot) {
        const u32 shape = shapes[slot];
        draw::Point p[32];
        u32 count = room::shapes[shape].count;
        const bool visible = room::visible(shape, pose);
        // Hidden frames keep valid geometry; stock must never receive a collapsed polygon.
        const room::Pose geometry = visible ? pose : room::pose(shape == 3 ? 0 : room::neutral);
        for (u32 j = 0; j < count; ++j)
            p[j] = room_point(shape, j, geometry);
        if (shape == 1) {
            // A thin closed ribbon replaces the open inset contour.
            draw::Point ribbon[8];
            for (u32 j = 0; j < 4; ++j) {
                ribbon[j] = {p[j].x - 180, p[j].y};
                ribbon[7 - j] = {p[j].x + 180, p[j].y};
            }
            for (u32 j = 0; j < 8; ++j)
                p[j] = ribbon[j];
            count = 8;
        }
        outline(frame, slot, p, count, state.key);
        draw::place(frame, slot, 240 * unit, 110 * unit, 2 * unit, shades[slot], draw::fill_and_stroke);
        if (!visible)
            draw::hide(frame, slot);
    }
    segment(frame, 4, room_point(4, 0, pose), room_point(4, 1, pose));
    draw::place(frame, 5, 0, 0, unit, 0, draw::keep);
    if (!room::visible(5, pose))
        draw::hide(frame, 5);
    draw::place(frame, 6, 0, 0, unit, (pose.outer_gray * 15 + 118) / 236, draw::fill_and_stroke);
    for (u32 i = 0; i < 4; ++i)
        frame.header[i][0] = 0;
}
} // namespace master_dj_filter
} // namespace extensions
} // namespace opxy
