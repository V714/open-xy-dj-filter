#include <opxy/extension.hpp>
#include <opxy/master_filter.hpp>
#include "master_filter.cpp"
#include "master_filter_tables.cpp"
#include "master_filter_room.hpp"
namespace opxy { namespace extensions { namespace dj {
namespace {
Processor processor;
room::Visual visual;
i32 shown = -1;
u32 project_serial, reset_serial, turn_serial;
void reset() {
    processor.reset();
}
void prepare(u32 sweep) {
    processor.prepare(sweep);
}
void process(i32 *left, i32 *right) {
    processor.process_dj(left, right);
}
void frame(const master::Scene &scene, const master::Input &input) {
    if (project_serial != input.project_serial) {
        visual.project();
        visual.follow(input.sweep);
    } else if (reset_serial != input.reset_serial && input.sweep == master::neutral) {
        visual.reset();
    } else if (turn_serial != input.turn_serial) {
        visual.follow(input.sweep);
    }
    project_serial = input.project_serial;
    turn_serial = input.turn_serial;
    reset_serial = input.reset_serial;
    visual.step(input.sweep, input.elapsed);
    if (shown == visual.value)
        return;
    shown = visual.value;
    const room::Pose pose = room::pose(visual.value);
    for (u32 i = 0; i < room::shape_count; ++i) {
        master::Point points[master::max_points];
        for (u32 j = 0; j < room::shapes[i].count; ++j) {
            const room::Point point = room::point(i, j, pose);
            points[j] = {point.x, point.y};
        }
        scene.path(scene.context, i, points, room::shapes[i].count, room::visible(i, pose));
    }
    scene.gray(scene.context, pose.outer_gray);
}
const master::Path paths[] = {{"door-panel", 4, true},    {"door-panel-inset", 4, false},
                              {"handle-outer", 32, true}, {"handle-stem", 2, false},
                              {"blind-panel", 4, true},   {"window-blind-bottom-bar", 2, false},
                              {"door-edge", 2, false}};
const master::Panel panel = {paths, 7, "outer-color", "gray-", 237, frame};
} // namespace
extern constexpr Hooks hooks = Hooks();
extern const master::Filter filter = {"dj filter", &panel, reset, prepare, process};
}}} // namespace opxy::extensions::dj
