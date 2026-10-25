#include "geoviz.hpp"
#include <chrono>
#include <iomanip>
#include <iostream>
#include <sstream>
using namespace gv;
using Clock = std::chrono::steady_clock;
template <class F> double timed(F action) {
    auto begin = Clock::now();
    action();
    return std::chrono::duration<double, std::micro>(Clock::now() - begin).count();
}
int main() {
    std::cout << "workload,n,mode,index_build_us,checked_no_trace_us,reference_repeat_us,checked_"
                 "trace_us,trace_frames,trace_payload_bytes,operations\n";
    for (const std::string name : {"sparse", "dense", "degenerate"})
        for (int n : {16, 64, 128, 256}) {
            Scene s;
            for (int i = 0; i < n; ++i)
                s.addPoint(name == "degenerate" ? Point{1, 1}
                           : name == "dense"
                               ? Point{(i % 16) * 0.01, (i / 16) * 0.01}
                               : Point{double((i * 37) % 257), double((i * 73) % 263)});
            s.setRadius(name == "dense" ? 2 : 28);
            double index = timed([&] {
                KDTree tree(s.points());
                Stats stats;
                tree.radius(s.query(), s.radius(), stats);
            });
            for (Mode mode : {Mode::DBSCAN, Mode::IndexedDBSCAN, Mode::KDNearest, Mode::ClosestPair,
                              Mode::Hull}) {
                Result result, trace;
                double checked = timed([&] { result = Engine{}.execute(s, mode, false); });
                double verify = timed([&] { Engine::verify(s, result); });
                double recorded = timed([&] { trace = Engine{}.execute(s, mode, true); });
                std::size_t payload = 0;
                for (const auto &f : trace.trace().frames())
                    payload +=
                        sizeof(Frame) + f.explanation().size() +
                        sizeof(int) * (f.active().size() + f.queue().size() + f.labels().size() +
                                       f.kind().size() + f.parents().size() + f.chain().size()) +
                        sizeof(double) * (f.costs().size() + f.scores().size());
                std::cout << name << ',' << n << ',' << static_cast<int>(mode) << ',' << std::fixed
                          << std::setprecision(2) << index << ',' << checked << ',' << verify << ','
                          << recorded << ',' << trace.trace().frames().size() << ',' << payload
                          << ',' << result.stats().operations() << '\n';
                if (!result.checked() || result.answer() != trace.answer() ||
                    result.labels() != trace.labels())
                    return 1;
            }
        }
}
