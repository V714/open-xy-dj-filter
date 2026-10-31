#include "master_filter.cpp"
#include "master_filter_tables.cpp"
#include <opxy/extension.hpp>
#include <opxy/master_filter.hpp>
namespace opxy {
namespace extensions {
namespace master_dj_filter {
namespace {
Processor processor;
void reset() { processor.reset(); }
void prepare(Parameters parameters) { processor.prepare(static_cast<u32>(parameters[0]) >> 21); }
void process(i32 *left, i32 *right) { processor.process_dj(left, right); }
} // namespace
extern constexpr Hooks hooks = Hooks();
extern const master::Filter filter = {"dj filter", reset, prepare, process};
} // namespace master_dj_filter
} // namespace extensions
} // namespace opxy
