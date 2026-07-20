/**
 * @file ConfigLoader.hpp
 * @brief Loads simulation configuration from YAML (flat) or JSON (nested) files.
 *
 * ConfigLoader detects the file format from the extension:
 *   .yaml / .yml  → flat YAML key:value parser (no external dependency)
 *   .json         → nlohmann/json (fetched via CMake FetchContent)
 *
 * The resulting Config or ExperimentConfig is fully validated before returning.
 *
 * @author  EMBI Simulator Project
 * @version 1.0.0
 */

#pragma once

#include "core/Config.hpp"

#include <string>

namespace embi {

/**
 * @class ConfigLoader
 * @brief Factory that deserialises Config / ExperimentConfig from disk.
 *
 * @par Supported formats
 *
 * **YAML (flat)** — single-scheduler, single-workload:
 * @code{.yaml}
 * scheduler:    embi
 * workload:     poisson
 * ticks:        1000000
 * num_processes: 64
 * arrival_rate: 0.5
 * seed:         42
 * M:            10.0
 * alpha:        0.1
 * beta:         0.1
 * output_dir:   output
 * @endcode
 *
 * **JSON (complex sweep)**:
 * @code{.json}
 * {
 *   "schedulers":    ["embi", "maxweight", "rr"],
 *   "workloads":     ["poisson", "bursty"],
 *   "seeds":         [42, 123],
 *   "arrival_rates": [0.3, 0.5],
 *   "ticks":         1000000,
 *   "num_processes": 64,
 *   "M":             10.0
 * }
 * @endcode
 */
class ConfigLoader {
public:
    ConfigLoader()                               = delete;
    ConfigLoader(const ConfigLoader&)            = delete;
    ConfigLoader& operator=(const ConfigLoader&) = delete;

    // ─── Single-run config ────────────────────────────────────────────────────

    /**
     * @brief Loads a single Config from a YAML or JSON file.
     *
     * Detects format from file extension. YAML files produce a flat Config.
     * JSON files are expected to have scalar (non-array) scheduler/workload
     * values; arrays are ignored.
     *
     * @param path  Absolute or relative path to the config file.
     * @return Validated Config object.
     * @throws std::runtime_error  if the file cannot be opened.
     * @throws std::invalid_argument if the config fails validation.
     * @complexity O(file size)
     */
    [[nodiscard]] static Config load(const std::string& path);

    // ─── Experiment sweep config ──────────────────────────────────────────────

    /**
     * @brief Loads an ExperimentConfig from a JSON file (for parameter sweeps).
     *
     * Only JSON is supported for experiment configs, as YAML is used for flat
     * single-run profiles.
     *
     * @param path  Absolute or relative path to the JSON config file.
     * @return ExperimentConfig with all sweep axes populated.
     * @throws std::runtime_error  if the file cannot be opened.
     * @throws std::invalid_argument if any value fails validation.
     * @complexity O(file size)
     */
    [[nodiscard]] static ExperimentConfig loadExperiment(const std::string& path);

private:
    // ─── Internal parsers ─────────────────────────────────────────────────────

    /// Parses a flat YAML file into a Config.
    [[nodiscard]] static Config loadYAML(const std::string& path);

    /// Parses a JSON file into a Config (scalar fields only).
    [[nodiscard]] static Config loadJSON(const std::string& path);

    /// Parses a JSON file into an ExperimentConfig (array fields supported).
    [[nodiscard]] static ExperimentConfig loadExperimentJSON(const std::string& path);
};

} // namespace embi
