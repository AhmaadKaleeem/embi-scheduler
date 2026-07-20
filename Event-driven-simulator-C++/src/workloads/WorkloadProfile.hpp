/**
 * @file WorkloadProfile.hpp
 * @brief Named workload presets for reproducible research scenarios.
 *
 * WorkloadProfile maps human-readable scenario names to concrete parameter
 * bundles, then instantiates the corresponding BaseWorkload implementation.
 *
 * This allows experiments to be described by intent ("web_server") rather
 * than by raw parameters, improving readability of experiment configs
 * and paper descriptions.
 *
 * @par Supported Profiles
 * | Name                | Distribution | λ (ON)  | Notes                        |
 * |---------------------|--------------|---------|------------------------------|
 * | web_server          | Bursty       | 0.80    | High arrival, short service  |
 * | database            | Poisson      | 0.30    | Moderate, variable service   |
 * | cloud_vm            | Heavy-Tail   | —       | Pareto α=1.5, rare bursts    |
 * | embedded            | Uniform      | —       | U(0.5, 2.0), regular ticks   |
 * | real_time           | Bursty       | 0.60    | ON/OFF with tight cycles     |
 * | interactive_desktop | Uniform      | —       | U(2.0, 10.0), low util       |
 *
 * @author  EMBI Simulator Project
 * @version 1.0.0
 */

#pragma once

#include "workloads/BaseWorkload.hpp"
#include "utils/Random.hpp"

#include <memory>
#include <string>
#include <string_view>

namespace embi {

/**
 * @class WorkloadProfile
 * @brief Factory for named workload presets.
 *
 * @par Example
 * @code
 * auto w = embi::WorkloadProfile::build("web_server", 42);
 * double inter = w->next();
 * @endcode
 */
class WorkloadProfile {
public:
    WorkloadProfile()                                = delete;
    WorkloadProfile(const WorkloadProfile&)          = delete;
    WorkloadProfile& operator=(const WorkloadProfile&) = delete;

    /**
     * @brief Instantiates the workload corresponding to the named profile.
     * @param profile_name  One of the supported profile names (case-sensitive).
     * @param seed          PRNG seed for the workload.
     * @return Owning pointer to the constructed BaseWorkload.
     * @throws std::invalid_argument if profile_name is not recognised.
     * @complexity O(1)
     */
    [[nodiscard]] static std::unique_ptr<BaseWorkload>
    build(const std::string& profile_name, uint64_t seed);

    /**
     * @brief Returns the canonical workload type name for a profile.
     *
     * E.g., WorkloadProfile::workloadName("web_server") == "bursty".
     *
     * @param profile_name  Profile name.
     * @return Workload type identifier.
     * @throws std::invalid_argument if profile_name is not recognised.
     * @complexity O(1)
     */
    [[nodiscard]] static std::string_view
    workloadName(const std::string& profile_name);

    /**
     * @brief Returns true if the given name is a recognised profile.
     * @param profile_name  Profile name to check.
     * @complexity O(1)
     */
    [[nodiscard]] static bool isKnownProfile(const std::string& profile_name) noexcept;
};

} // namespace embi
