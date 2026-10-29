#include "extension.cpp"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <limits>
using namespace opxy;
using namespace opxy::extensions::dj;
static void require(bool ok, const char *why) {
    if (!ok) {
        std::fprintf(stderr, "%s\n", why);
        std::exit(1);
    }
}
static void coeff(u32 hp, u32 d, i32 (&c)[5]) {
    const Coefficients &t = coefficients[hp][d];
    c[0] = t.a1;
    c[1] = t.a2;
    c[2] = t.a3;
    c[3] = t.k;
    c[4] = t.compensation;
}
static double amplitude(u32 hp, u32 d, double f) {
    Svf s = {};
    i32 c[5];
    coeff(hp, d, c);
    double in = 0, out = 0;
    for (u32 i = 0; i < 44100; ++i) {
        const i32 x = static_cast<i32>(std::sin(6.283185307179586 * f * i / 44100) * 0x10000000);
        const i32 y = s.tick(x, c, hp != 0);
        if (i > 22050) {
            in += static_cast<double>(x) * x;
            out += static_cast<double>(y) * y;
        }
    }
    return std::sqrt(out / in);
}
static void settle(Processor &p, u32 sweep, i32 (&l)[64], i32 (&r)[64], i32 x) {
    for (u32 b = 0; b < 120; ++b) {
        p.prepare(sweep);
        for (u32 i = 0; i < 64; ++i) {
            l[i] = x;
            r[i] = 0;
        }
        p.process_dj(l, r);
    }
}
int main() {
    // Independent unquantized double-precision reference for every target.
    for (u32 hp = 0; hp < 2; ++hp)
        for (u32 d = 0; d < 513; ++d) {
            i32 c[5];
            coeff(hp, d, c);
            double a[5];
            for (u32 i = 0; i < 5; ++i)
                a[i] = c[i] / 536870912.0;
            const double trace = 2 * a[0] - 2 * a[2], det = (2 * a[0] - 1) * (1 - 2 * a[2]) + 4 * a[1] * a[1];
            require(1 - trace + det > 0 && 1 + trace + det > 0 && 1 - det > 0,
                    "Stable quantized state matrix");
            const double t = d < 51 ? d / 51.0 : 1,
                         q = std::sqrt(.5) + (1.25 - std::sqrt(.5)) * t * t * (3 - 2 * t);
            const double f = hp ? 20 * std::pow(250, d / 512.0)
                                : 20000 * std::pow(250, -static_cast<double>(d) / 512),
                         g = std::tan(3.141592653589793 * f / 44100), k = 1 / q;
            const double a1 = 1 / (1 + g * (g + k)), a2 = g * a1, a3 = g * a2,
                         comp = q <= std::sqrt(.5) ? 1 : std::sqrt(1 - 1 / (4 * q * q)) / q;
            Svf filter = {};
            double s1 = 0, s2 = 0;
            unsigned seed = 123;
            for (u32 i = 0; i < 2048; ++i) {
                seed = 1664525 * seed + 1013904223;
                const i32 x = static_cast<i32>(seed >> 3) - 0x10000000;
                const double v3 = x - s2, v1 = a1 * s1 + a2 * v3, v2 = s2 + a2 * s1 + a3 * v3;
                s1 = 2 * v1 - s1;
                s2 = 2 * v2 - s2;
                const double expected = (hp ? x - k * v1 - v2 : v2) * comp;
                require(std::fabs(filter.tick(x, c, hp != 0) - expected) < 100000,
                        "Fixed-point vs double reference");
            }
        }
    // Ramped coefficients must remain stable even between distant cutoffs.
    for (u32 hp = 0; hp < 2; ++hp)
        for (u32 x = 0; x < 513; ++x)
            for (u32 y = 0; y < 513; y += 8)
                for (u32 fraction = 0; fraction <= 16; ++fraction) {
                    const Coefficients &a = coefficients[hp][x], &b = coefficients[hp][y];
                    const double w = fraction / 16.0, one = 536870912.0;
                    const double a1 = (a.a1 * (1 - w) + b.a1 * w) / one,
                                 a2 = (a.a2 * (1 - w) + b.a2 * w) / one,
                                 a3 = (a.a3 * (1 - w) + b.a3 * w) / one;
                    const double trace = 2 * a1 - 2 * a3, det = (2 * a1 - 1) * (1 - 2 * a3) + 4 * a2 * a2;
                    require(1 - trace + det > 0 && 1 + trace + det > 0 && 1 - det > 0,
                            "Stable coefficient transitions");
                }
    require(amplitude(0, 512, 80) > .70 && amplitude(0, 512, 80) < 1.01, "LP endpoint retains audible bass");
    require(amplitude(1, 512, 5000) > .70 && amplitude(1, 512, 5000) < 1.01,
            "HP endpoint retains audible treble");
    require(amplitude(0, 256, 100) > .70 && amplitude(0, 256, 10000) < .02, "LP response");
    require(amplitude(1, 256, 50) < .02 && amplitude(1, 256, 10000) > .70, "HP response");
    require(amplitude(0, 256, 1200) > amplitude(0, 256, 100) * 1.20, "Resonant edge");
    Processor p;
    p.reset();
    i32 l[64], r[64];
    p.prepare(512);
    for (u32 i = 0; i < 64; ++i) {
        l[i] = i & 1 ? std::numeric_limits<i32>::min() : std::numeric_limits<i32>::max();
        r[i] = 0;
    }
    p.process_dj(l, r);
    for (u32 i = 0; i < 64; ++i)
        require(l[i] == (i & 1 ? std::numeric_limits<i32>::min() : std::numeric_limits<i32>::max()) && !r[i],
                "Neutral bit-exact");
    require(!p.filters[0][0].s1 && !p.filters[1][0].s1, "Neutral skips DSP");
    // Both extremes remain filters: no gain stage can mute the residual band.
    settle(p, 0, l, r, 123456789);
    require(l[63] > 70000000, "LP endpoint retains DC");
    for (u32 b = 0; b < 120; ++b) {
        p.prepare(1023);
        for (u32 i = 0; i < 64; ++i) {
            l[i] = i & 1 ? -123456789 : 123456789;
            r[i] = 0;
        }
        p.process_dj(l, r);
    }
    require(std::abs(l[63]) > 70000000, "HP endpoint retains upper band");
    settle(p, 256, l, r, 12345);
    const i32 old = p.filters[1][0].s1;
    settle(p, 256, l, r, 23456);
    require(p.filters[1][0].s1 == old, "Inactive HP skips DSP");
    for (u32 b = 0; b < 1000; ++b) {
        p.prepare(b % 3 == 0 ? 0u : b % 3 == 1 ? 512u : 1023u);
        for (u32 i = 0; i < 64; ++i) {
            l[i] = i & 1 ? std::numeric_limits<i32>::min() : std::numeric_limits<i32>::max();
            r[i] = 0;
        }
        p.process_dj(l, r);
        for (u32 i = 0; i < 64; ++i)
            require(!r[i], "Rapid full-scale sweeps preserve stereo isolation");
    }
    // Silence after arbitrary transitions must decay rather than latch to a rail.
    settle(p, 256, l, r, 0);
    require(std::abs(l[63]) < 4096, "No growing oscillation or audible fixed-point limit cycle");
    settle(p, 512, l, r, 123456789);
    require(l[63] == 123456789, "Return to neutral");
    for (i32 target = -100000; target <= 100000; target += 12345) {
        Ramp ramp;
        ramp.reset(0);
        ramp.set(target);
        i32 previous = 0;
        for (u32 i = 0; i < ramp_frames; ++i) {
            const i32 v = ramp.tick();
            require(target < 0 ? v <= previous : v >= previous, "Monotone DDA");
            previous = v;
        }
        require(previous == target && !ramp.remaining, "DDA duration");
    }
}
