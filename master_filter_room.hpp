// GUI-only room geometry and presentation state; no audio or stock calls.
#pragma once
#include "master_filter_room_data.hpp"
namespace opxy { namespace extensions { namespace room {
const i32 neutral = 50 * 1024;
const u32 reset_ticks = 123; // 120ms rounded to stock Q10 seconds.
inline i32 position(u32 sweep) {
    if (sweep > 1023)
        sweep = 1023;
    return sweep <= 512 ? static_cast<i32>(sweep * neutral / 512)
                        : neutral + static_cast<i32>((sweep - 512) * neutral / 511);
}
struct Visual {
    i32 value, start;
    u32 elapsed, last;
    bool initialized, resetting;
    void follow(u32 sweep) {
        value = position(sweep);
        last = sweep;
        initialized = true;
        resetting = false;
    }
    void reset() {
        if (initialized) {
            start = value;
            elapsed = 0;
            last = 512;
            resetting = value != neutral;
        }
    }
    void project() { initialized = resetting = false; }
    void step(u32 sweep, i32 delta) {
        if (!initialized) {
            follow(sweep);
            return;
        }
        if (!resetting) {
            if (sweep != last)
                follow(sweep);
            return;
        }
        if (sweep != 512) {
            follow(sweep);
            return;
        }
        if (delta <= 0)
            return;
        elapsed += static_cast<u32>(delta) > reset_ticks ? reset_ticks : static_cast<u32>(delta);
        if (elapsed >= reset_ticks) {
            value = neutral;
            resetting = false;
            return;
        }
        const i32 t = static_cast<i32>(elapsed * 1024 / reset_ticks);
        const i32 easing = (t * t / 1024) * (3072 - 2 * t) / 1024;
        value = start + (neutral - start) * easing / 1024;
    }
};
struct Pose {
    i32 low, high, a, b, coverage;
    u32 outer_gray;
};
inline Pose pose(i32 value) {
    if (value < 0)
        value = 0;
    if (value > 100 * 1024)
        value = 100 * 1024;
    const i32 low = value < neutral ? value * 1024 / neutral : 1024;
    const i32 high = value > neutral ? (value - neutral) * 1024 / neutral : 0;
    const u32 index = static_cast<u32>(low) / 4, next = index < 256 ? index + 1 : index;
    const i32 fraction = low % 4;
    return Pose{low,
                high,
                (door[index][0] + (door[next][0] - door[index][0]) * fraction / 4) / 64,
                (door[index][1] + (door[next][1] - door[index][1]) * fraction / 4) / 64,
                1024 - low,
                static_cast<u32>((236 * high + 512) / 1024)};
}
// Hide sub-pixel filled polygons instead of passing singular geometry to stock.
inline bool visible(u32 shape, const Pose &p) {
    if (shape < 4)
        return p.a <= -door_min_projection || p.a >= door_min_projection;
    if (shape == 4)
        return p.coverage >= blind_min_coverage; // One pixel of blind height.
    if (shape == 6)
        return p.a > -door_min_projection && p.a < door_min_projection;
    return true;
}
inline Point point(u32 shape, u32 index, const Pose &p) {
    const Point q = shapes[shape].points[index];
    if (shape == 6)
        return Point{door_origin.x, door_origin.y + (index ? p.b * door_width / 1024 + door_height : 0)};
    if (shape < 4)
        return Point{door_origin.x + p.a * q.x / 1024, door_origin.y + p.b * q.x / 1024 + q.y};
    return Point{blind_origin.x + q.x, blind_origin.y + blind_skew * q.x / 1024 + p.coverage * q.y / 1024};
}
}}} // namespace opxy::extensions::room
