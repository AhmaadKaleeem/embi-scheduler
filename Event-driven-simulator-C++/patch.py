import os
import re

def patch_file(path, pattern, replacement):
    with open(path, 'r', encoding='utf-8') as f:
        content = f.read()
    content = content.replace(pattern, replacement)
    with open(path, 'w', encoding='utf-8') as f:
        f.write(content)

# 1. Process.hpp
process_hpp = "src/core/Process.hpp"
patch_file(process_hpp, 
"    // ─── Computed properties ─────────────────────────────────────────────────────", 
"""    // ─── Computed properties ─────────────────────────────────────────────────────

    /**
     * @brief Returns the arrival time of the Head-of-Line (HOL) job.
     * @return Arrival tick, or -1.0 if queue is empty.
     */
    double holArrivalTime() const noexcept {
        return job_arrival_queue_.empty() ? -1.0 : job_arrival_queue_.front();
    }
""")

# 2. FCFSScheduler.cpp
fcfs_cpp = "src/schedulers/FCFSScheduler.cpp"
patch_file(fcfs_cpp, 
"procs[i].first_arrival_time", 
"procs[i].holArrivalTime()")

# 3. Config.hpp
config_hpp = "src/core/Config.hpp"
patch_file(config_hpp, 
"    double arrival_rate{0.5};   ///< Mean job arrivals per tick per process.", 
"    double arrival_rate{0.5};   ///< Mean job arrivals per tick per process.\n    std::vector<double> arrival_rate_asymmetric; ///< Optional per-process rates.")
patch_file(config_hpp, 
"    std::vector<double>      arrival_rates{0.5};      ///< Arrival rates to sweep.",
"    std::vector<double>      arrival_rates{0.5};      ///< Arrival rates to sweep.\n    std::vector<double>      alphas;                  ///< Alphas to sweep.")

# 4. Config.cpp
config_cpp = "src/core/Config.cpp"
patch_file(config_cpp,
"return schedulers.size() * workloads.size() * seeds.size() * arrival_rates.size();",
"std::size_t a_len = alphas.empty() ? 1 : alphas.size();\n    return schedulers.size() * workloads.size() * seeds.size() * arrival_rates.size() * a_len;")
patch_file(config_cpp,
"for (double rate : arrival_rates) {",
"for (double rate : arrival_rates) {\n                    std::vector<double> sweep_a = alphas.empty() ? std::vector<double>{alpha} : alphas;\n                    for (double a : sweep_a) {")
patch_file(config_cpp,
"configs.push_back(c);",
"c.alpha = a;\n                        configs.push_back(c);\n                    }")
patch_file(config_cpp,
"std::to_string(rate)",
"std::to_string(rate) + (alphas.empty() ? \"\" : \"_alpha_\" + std::to_string(a))")

# 5. CLI.cpp
cli_cpp = "src/CLI.cpp"
patch_file(cli_cpp,
"        if (arg == \"--arrival-rate\") {",
"""        if (arg == "--arrival-rate-asymmetric") {
            expect_arg(arg.c_str(), ++i, argc);
            std::string val = argv[i];
            std::stringstream ss(val);
            std::string token;
            cfg.arrival_rate_asymmetric.clear();
            while (std::getline(ss, token, ',')) {
                cfg.arrival_rate_asymmetric.push_back(toDouble(arg.c_str(), token.c_str()));
            }
            continue;
        }
        if (arg == "--alpha-sweep") {
            expect_arg(arg.c_str(), ++i, argc);
            std::string val = argv[i];
            std::stringstream ss(val);
            std::string token;
            result.exp_config.alphas.clear();
            while (std::getline(ss, token, ',')) {
                result.exp_config.alphas.push_back(toDouble(arg.c_str(), token.c_str()));
            }
            result.experiment_mode = true;
            continue;
        }
        if (arg == "--arrival-rate") {""")

# 6. EventLoop.cpp
eventloop_cpp = "src/core/EventLoop.cpp"
patch_file(eventloop_cpp,
"""        processes_.emplace_back(i,
                                config.arrival_rate,
                                config.service_rate,
                                config.alpha,
                                config.beta);""",
"""        double rate = config.arrival_rate;
        if (!config.arrival_rate_asymmetric.empty()) {
            rate = config.arrival_rate_asymmetric[i % config.arrival_rate_asymmetric.size()];
        }
        processes_.emplace_back(i,
                                rate,
                                config.service_rate,
                                config.alpha,
                                config.beta);""")
patch_file(eventloop_cpp,
"""            while (next_arrival_tick_[i] <= tick_d) {
                event_queue_.push(makeArrivalEvent(tick_d, i));
                next_arrival_tick_[i] += workload_->next();
            }""",
"""            double rate = config_.arrival_rate;
            if (!config_.arrival_rate_asymmetric.empty()) {
                rate = config_.arrival_rate_asymmetric[i % config_.arrival_rate_asymmetric.size()];
            }
            double scale = (rate > 0.0) ? (config_.arrival_rate / rate) : 1.0;

            while (next_arrival_tick_[i] <= tick_d) {
                event_queue_.push(makeArrivalEvent(tick_d, i));
                next_arrival_tick_[i] += workload_->next() * scale;
            }""")

print("Patching complete.")
