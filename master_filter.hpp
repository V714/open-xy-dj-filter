// DJ filter DSP; all state is private to the audio role.
#pragma once
#include <opxy/types.hpp>
namespace opxy { namespace extensions { namespace master_dj_filter {
const u32 block_frames = 64;
const u32 ramp_frames = 882; // 20 ms at 44.1 kHz.
// TPT-SVF coefficients and resonance peak compensation, all Q2.29.
struct Coefficients {
    i32 a1, a2, a3, k, compensation;
};
extern const Coefficients coefficients[2][513];
struct Svf {
    i32 s1, s2; // Q27, retaining headroom until the final output conversion.
    void reset();
    i32 tick(i32, const i32 (&)[5], bool);
};
// DDA ramp: division only when a target changes, never in the sample loop.
struct Ramp {
    i32 value, target, step, remainder, error, sign;
    u32 remaining;
    void reset(i32);
    void set(i32);
    i32 tick();
};
struct Processor {
    Svf filters[2][2];
    Ramp current[2][5], low, high;
    void reset();
    void prepare(u32 sweep);
    void process_dj(i32 *, i32 *);
};
}}} // namespace opxy::extensions::master_dj_filter
