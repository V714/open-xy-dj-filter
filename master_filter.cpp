#include "master_filter.hpp"
#include <opxy/q31.hpp>
using namespace opxy;
namespace opxy { namespace extensions { namespace dj {
namespace {
using opxy::q31::wide;
i32 clamp(i64 value, i32 limit) {
    return value > limit ? limit : value < -static_cast<i64>(limit) ? -limit : static_cast<i32>(value);
}
i32 smooth(u32 n, u32 range) {
    if (n >= range)
        return 32768;
    const i32 t = static_cast<i32>(n * 32768u / range);
    const i32 square = static_cast<i32>(wide(t, t) / 32768);
    return static_cast<i32>(wide(square, 98304 - 2 * t) / 32768);
}
} // namespace
void Ramp::reset(i32 n) {
    value = target = n;
    step = remainder = error = sign = 0;
    remaining = 0;
}
void Ramp::set(i32 n) {
    if (n == target)
        return;
    target = n;
    const i32 delta = target - value;
    step = delta / static_cast<i32>(ramp_frames);
    const i32 rest = delta % static_cast<i32>(ramp_frames);
    sign = rest < 0 ? -1 : 1;
    remainder = rest < 0 ? -rest : rest;
    error = 0;
    remaining = ramp_frames;
}
i32 Ramp::tick() {
    if (remaining) {
        value += step;
        error += remainder;
        if (error >= static_cast<i32>(ramp_frames)) {
            value += sign;
            error -= ramp_frames;
        }
        if (!--remaining)
            value = target;
    }
    return value;
}
void Svf::reset() {
    s1 = s2 = 0;
}
i32 Svf::tick(i32 input, const i32 (&c)[5], bool high) {
    // Closed-form trapezoidal SVF; state updates use the same old states.
    // Q27 and 64-bit products avoid clipping between integrators.
    const i32 x = input / 16, v3 = x - s2;
    const i32 v1 = clamp((wide(c[0], s1) + wide(c[1], v3)) / (1LL << 29), 1 << 29);
    const i32 v2 = clamp(static_cast<i64>(s2) + (wide(c[1], s1) + wide(c[2], v3)) / (1LL << 29), 1 << 29);
    s1 = clamp(2LL * v1 - s1, 1 << 29);
    s2 = clamp(2LL * v2 - s2, 1 << 29);
    const i32 y = high ? clamp(static_cast<i64>(x) - wide(c[3], v1) / (1LL << 29) - v2, 1 << 30) : v2;
    const i64 result = (wide(y, c[4]) / (1LL << 29)) * 16;
    return result > 2147483647      ? 2147483647
           : result < -2147483648LL ? static_cast<i32>(-2147483647 - 1)
                                    : static_cast<i32>(result);
}
void Processor::reset() {
    for (u32 kind = 0; kind < 2; ++kind) {
        for (u32 channel = 0; channel < 2; ++channel)
            filters[kind][channel].reset();
        const Coefficients &c = coefficients[kind][0];
        const i32 values[5] = {c.a1, c.a2, c.a3, c.k, c.compensation};
        for (u32 i = 0; i < 5; ++i)
            current[kind][i].reset(values[i]);
    }
    low.reset(0);
    high.reset(0);
}
void Processor::prepare(u32 sweep) {
    const u32 d[2] = {sweep < 512 ? 512 - sweep : 0, sweep > 512 ? (sweep - 512) * 512 / 511 : 0};
    for (u32 kind = 0; kind < 2; ++kind) {
        const Coefficients &c = coefficients[kind][d[kind]];
        const i32 values[5] = {c.a1, c.a2, c.a3, c.k, c.compensation};
        for (u32 i = 0; i < 5; ++i)
            current[kind][i].set(values[i]);
    }
    low.set(smooth(d[0], 16));
    high.set(smooth(d[1], 16));
}
void Processor::process_dj(i32 *left, i32 *right) {
    // Exact neutral is free of recursive DSP. Retain histories for later sweeps.
    if (!low.value && !low.target && !high.value && !high.target)
        return;
    for (u32 i = 0; i < block_frames; ++i) {
        const bool run_l = low.value || low.target, run_h = high.value || high.target;
        const i32 l = low.tick(), raw_h = high.tick();
        const i32 h = raw_h < 32768 - l ? raw_h : 32768 - l;
        i32 c[2][5];
        for (u32 k = 0; k < 2; ++k)
            if (k ? run_h : run_l)
                for (u32 j = 0; j < 5; ++j)
                    c[k][j] = current[k][j].tick();
        i32 *samples[2] = {left + i, right + i};
        for (u32 ch = 0; ch < 2; ++ch) {
            const i32 dry = *samples[ch];
            const i32 lp = run_l ? filters[0][ch].tick(dry, c[0], false) : dry;
            const i32 hp = run_h ? filters[1][ch].tick(dry, c[1], true) : dry;
            const i32 filtered =
                static_cast<i32>((wide(dry, 32768 - l - h) + wide(lp, l) + wide(hp, h)) / 32768);
            *samples[ch] = filtered;
        }
    }
}
}}} // namespace opxy::extensions::dj
