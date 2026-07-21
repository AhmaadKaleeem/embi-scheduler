/**
 * @file Config.cpp
 * @brief Config validation and ExperimentConfig expansion.
 *
 * @author  EMBI Simulator Project
 * @version 1.0.0
 */

#include "core/Config.hpp"

#include <algorithm>
#include <cmath>
#include <sstream>
#include <stdexcept>

namespace embi {

// ─── Helpers ─────────────────────────────────────────────────────────────────

namespace {

/// Returns true if 'name' is in the null-terminated sentinel array.
/// Accepts both mutable and const-pointer arrays (constexpr-safe).
bool inTable(const char* const* table, std::size_t n, const std::string& name) noexcept {
    for (std::size_t i = 0; i < n; ++i) {
        if (name == table[i]) return true;
    }
    return false;
}

void require(bool cond, const char* msg) {
    if (!cond) throw std::invalid_argument(msg);
}

std::string asymSuffix(const std::vector<double>& rates) {
    if (rates.empty()) return {};

    std::ostringstream oss;
    oss << "_asym";
    for (double rate : rates) {
        oss << "_" << std::to_string(rate);
    }
    return oss.str();
}

} // namespace

// ─── Config::validate ────────────────────────────────────────────────────────

void Config::validate() const {
    require(ticks > 0,          "Config: ticks must be > 0");
    require(num_processes >= 1, "Config: num_processes must be >= 1");

    require(arrival_rate > 0.0 && arrival_rate <= 1.0,
            "Config: arrival_rate must be in (0, 1]");
    for (double rate : arrival_rate_asymmetric) {
        require(rate > 0.0 && rate <= 1.0,
                "Config: arrival_rate_asymmetric values must be in (0, 1]");
    }
    require(service_rate > 0.0,
            "Config: service_rate must be > 0");
    require(alpha > 0.0 && alpha < 1.0,
            "Config: alpha (EWMA arrival smoothing) must be in (0, 1)");
    require(beta > 0.0 && beta < 1.0,
            "Config: beta (EWMA service smoothing) must be in (0, 1)");
    require(M > 0.0,
            "Config: M (service-capacity bound) must be > 0");
    require(log_freq >= 1,
            "Config: log_freq must be >= 1");

    // Scheduler name
    if (!inTable(kValidSchedulers, std::size(kValidSchedulers), scheduler_name)) {
        std::string msg = "Config: unknown scheduler '" + scheduler_name +
                          "'. Valid options: embi, embi_unclipped, maxweight, cmu, rr, fcfs";
        throw std::invalid_argument(msg);
    }

    // Workload name (only required if no profile is set)
    if (!workload_profile.has_value()) {
        if (!inTable(kValidWorkloads, std::size(kValidWorkloads), workload_name)) {
            std::string msg = "Config: unknown workload '" + workload_name +
                              "'. Valid options: uniform, poisson, bursty, heavy_tail, trace";
            throw std::invalid_argument(msg);
        }
    } else {
        if (!inTable(kValidProfiles, std::size(kValidProfiles), workload_profile.value())) {
            std::string msg = "Config: unknown workload profile '" +
                              workload_profile.value() + "'";
            throw std::invalid_argument(msg);
        }
    }

    // Trace workload requires a trace file
    if (workload_name == "trace" && !trace_file.has_value()) {
        throw std::invalid_argument(
            "Config: workload 'trace' requires --trace-file to be specified");
    }

    // Uniform workload parameters
    require(uniform_lo < uniform_hi,
            "Config: uniform_lo must be < uniform_hi");
    require(uniform_lo > 0.0,
            "Config: uniform_lo must be > 0");

    // Pareto parameters
    require(pareto_shape > 0.0, "Config: pareto_shape must be > 0");
    require(pareto_scale > 0.0, "Config: pareto_scale must be > 0");

    // Bursty parameters
    require(burst_on_rate  > 0.0 && burst_on_rate  <= 1.0,
            "Config: burst_on_rate must be in (0, 1]");
    require(burst_off_rate >= 0.0 && burst_off_rate <= 1.0,
            "Config: burst_off_rate must be in [0, 1]");
    require(burst_p_on_off > 0.0 && burst_p_on_off < 1.0,
            "Config: burst_p_on_off must be in (0, 1)");
    require(burst_p_off_on > 0.0 && burst_p_off_on < 1.0,
            "Config: burst_p_off_on must be in (0, 1)");
}

// ─── Config::defaults ────────────────────────────────────────────────────────

Config Config::defaults() noexcept {
    return Config{};
}

// ─── ExperimentConfig::expand ─────────────────────────────────────────────────

std::vector<Config> ExperimentConfig::expand() const {
    std::vector<Config> configs;
    configs.reserve(totalRuns());

    const std::vector<double> alpha_sweep =
        alphas.empty() ? std::vector<double>{alpha} : alphas;
    const std::vector<std::vector<double>> asym_sweep =
        arrival_rate_asymmetric_sweep.empty()
            ? std::vector<std::vector<double>>{arrival_rate_asymmetric}
            : arrival_rate_asymmetric_sweep;
    const std::vector<double> eps_sweep =
        epsilon_totals.empty() ? std::vector<double>{0.0} : epsilon_totals;
    const std::vector<double> noise_sweep =
        lambda_noise_stddevs.empty() ? std::vector<double>{0.0} : lambda_noise_stddevs;

    for (const auto& sched : schedulers) {
        for (const auto& wload : workloads) {
            for (uint64_t seed_val : seeds) {
                for (double rate : arrival_rates) {
                    for (const auto& asym_rates : asym_sweep) {
                        for (double a : alpha_sweep) {
                            for (double em : eps_sweep) {
                                for (double nz : noise_sweep) {
                                    Config cfg;
                                    cfg.ticks           = ticks;
                                    if (warmup_ticks == 0) {
                                        cfg.warmup_ticks = std::max<uint64_t>(100000, ticks / 10);
                                    } else {
                                        cfg.warmup_ticks = warmup_ticks;
                                    }
                                    cfg.num_processes   = num_processes;
                                    cfg.context_switch_cost = context_switch_cost;
                                    cfg.git_commit_hash = git_commit_hash;
                                    cfg.binary_sha256   = binary_sha256;
                                    cfg.config_hash     = config_hash;
                                    cfg.seed            = seed_val;
                                    cfg.scheduler_name  = sched;
                                    cfg.workload_name   = wload;
                                    cfg.arrival_rate    = rate;
                                    cfg.service_rate    = service_rate;
                                    cfg.alpha           = a;
                                    cfg.beta            = beta;
                                    cfg.lambda_noise_stddev = nz;
                                    cfg.M               = M;
                                    cfg.epsilon_total   = em;
                                    cfg.pareto_shape    = pareto_shape;
                                    cfg.pareto_scale    = pareto_scale;
                                    cfg.burst_on_rate   = burst_on_rate;
                                    cfg.burst_off_rate  = burst_off_rate;
                                    cfg.burst_p_on_off  = burst_p_on_off;
                                cfg.burst_p_off_on  = burst_p_off_on;
                                cfg.num_locks       = num_locks;
                                cfg.lock_request_rate = lock_request_rate;
                                cfg.lock_hold_mean  = lock_hold_mean;
                                cfg.arrival_rate_asymmetric = asym_rates;
                                cfg.trace_file      = trace_file;
                                cfg.log_freq        = log_freq;
                                cfg.binary_log      = binary_log;
                                cfg.null_log        = null_log;

                                // Build per-run output directory:
                                // output_dir/scheduler/workload/seed_<s>_rate_<r>
                                std::ostringstream oss;
                                oss << output_dir << "/" << sched << "/" << wload
                                    << "/seed_" << seed_val
                                    << "_rate_" << std::to_string(rate)
                                    << asymSuffix(asym_rates)
                                    << (alphas.empty() ? "" : "_alpha_" + std::to_string(a))
                                    << (epsilon_totals.empty() ? "" : "_eps_" + std::to_string(em));
                                    cfg.output_dir = oss.str();

                                    cfg.validate();
                                    configs.push_back(std::move(cfg));
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    return configs;
}

std::size_t ExperimentConfig::totalRuns() const noexcept {
    std::size_t a_len = alphas.empty() ? 1 : alphas.size();
    std::size_t asym_len = arrival_rate_asymmetric_sweep.empty()
        ? 1
        : arrival_rate_asymmetric_sweep.size();
    std::size_t eps_len = epsilon_totals.empty() ? 1 : epsilon_totals.size();
    std::size_t noise_len = lambda_noise_stddevs.empty() ? 1 : lambda_noise_stddevs.size();
    return schedulers.size() * workloads.size() * seeds.size() *
           arrival_rates.size() * a_len * asym_len * eps_len * noise_len;
}

} // namespace embi
