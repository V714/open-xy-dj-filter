#pragma once
#include "master_filter_room.hpp"
#include <opxy/draw.hpp>
namespace opxy {
namespace extensions {
namespace master_dj_filter {
using Frame = draw::Frame<4, 32, 8, 0, 16>;
struct DisplayState {
    room::Visual visual;
    u32 key;
};
void display_reset(DisplayState &);
void display_parameter_reset(DisplayState &, u32);
void display(DisplayState &, const draw::Input &, Frame &);
} // namespace master_dj_filter
} // namespace extensions
} // namespace opxy
