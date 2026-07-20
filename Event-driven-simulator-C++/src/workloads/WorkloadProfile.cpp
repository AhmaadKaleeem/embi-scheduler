/**
 * @file WorkloadProfile.cpp
 * @brief Implementation of WorkloadProfile factory.
 *
 * @author  EMBI Simulator Project
 * @version 1.0.0
 */

#include "workloads/WorkloadProfile.hpp"

#include "workloads/BurstyWorkload.hpp"
#include "workloads/HeavyTailWorkload.hpp"
#include "workloads/PoissonWorkload.hpp"
#include "workloads/UniformWorkload.hpp"

#include <stdexcept>
#include <unordered_map>

namespace embi {

// ─── Profile registry ─────────────────────────────────────────────────────────

namespace {

struct ProfileParams {
    std::string_view workload_type;
    // Bursty
    double on_rate{0.8};
    double off_rate{0.05};
    double p_on_off{0.1};
    double p_off_on{0.3};
    // Poisson
    double lambda{0.5};
    // HeavyTail
    double pareto_scale{1.0};
    double pareto_shape{1.5};
    // Uniform
    double uni_lo{0.5};
    double uni_hi{2.0};
};

// Profiles are defined as constexpr-friendly structs in a static table.
// Using a lookup map avoids if-else chains and keeps the code extensible.
const std::unordered_map<std::string, ProfileParams>& profileRegistry() {
    static const std::unordered_map<std::string, ProfileParams> kRegistry = {
        // High-arrival bursty: web traffic with ON/OFF surges
        {"web_server",          {"bursty",
                                  /* on_rate */ 0.80, /* off_rate */ 0.05,
                                  /* p_on_off */ 0.10, /* p_off_on */ 0.30}},

        // Moderate Poisson: OLTP database queries
        {"database",            {"poisson",
                                  {}, {}, {}, {},
                                  /* lambda */ 0.30}},

        // Heavy-tail: cloud VM workloads with occasional long inter-arrivals
        {"cloud_vm",            {"heavy_tail",
                                  {}, {}, {}, {},
                                  {},
                                  /* pareto_scale */ 1.0, /* pareto_shape */ 1.5}},

        // Regular embedded: tightly bounded inter-arrival times
        {"embedded",            {"uniform",
                                  {}, {}, {}, {},
                                  {},
                                  {}, {},
                                  /* uni_lo */ 0.5, /* uni_hi */ 2.0}},

        // Real-time: fast ON/OFF cycling with high burst rate
        {"real_time",           {"bursty",
                                  /* on_rate */ 0.60, /* off_rate */ 0.02,
                                  /* p_on_off */ 0.20, /* p_off_on */ 0.40}},

        // Interactive desktop: low utilisation, sporadic arrivals
        {"interactive_desktop", {"uniform",
                                  {}, {}, {}, {},
                                  {},
                                  {}, {},
                                  /* uni_lo */ 2.0, /* uni_hi */ 10.0}},
    };
    return kRegistry;
}

} // anonymous namespace

// ─── Public API ──────────────────────────────────────────────────────────────

std::unique_ptr<BaseWorkload>
WorkloadProfile::build(const std::string& profile_name, uint64_t seed) {
    const auto& registry = profileRegistry();
    auto it = registry.find(profile_name);
    if (it == registry.end()) {
        throw std::invalid_argument(
            "WorkloadProfile::build: unknown profile '" + profile_name +
            "'. Valid: web_server, database, cloud_vm, embedded, "
            "real_time, interactive_desktop");
    }

    const ProfileParams& p = it->second;

    if (p.workload_type == "bursty") {
        return std::make_unique<BurstyWorkload>(
            seed, p.on_rate, p.off_rate, p.p_on_off, p.p_off_on);
    }
    if (p.workload_type == "poisson") {
        return std::make_unique<PoissonWorkload>(seed, p.lambda);
    }
    if (p.workload_type == "heavy_tail") {
        return std::make_unique<HeavyTailWorkload>(seed, p.pareto_scale, p.pareto_shape);
    }
    if (p.workload_type == "uniform") {
        return std::make_unique<UniformWorkload>(seed, p.uni_lo, p.uni_hi);
    }

    // Should never reach here if ProfileParams is correctly populated
    throw std::logic_error("WorkloadProfile::build: unhandled workload_type '"
                           + std::string(p.workload_type) + "'");
}

std::string_view
WorkloadProfile::workloadName(const std::string& profile_name) {
    const auto& registry = profileRegistry();
    auto it = registry.find(profile_name);
    if (it == registry.end()) {
        throw std::invalid_argument(
            "WorkloadProfile::workloadName: unknown profile '" + profile_name + "'");
    }
    return it->second.workload_type;
}

bool WorkloadProfile::isKnownProfile(const std::string& profile_name) noexcept {
    return profileRegistry().count(profile_name) > 0;
}

} // namespace embi
