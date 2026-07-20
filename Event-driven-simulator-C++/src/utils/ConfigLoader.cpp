/**
 * @file ConfigLoader.cpp
 * @brief Implementation of YAML (flat) and JSON config file parsers.
 *
 * YAML parser: hand-rolled line-by-line key:value reader (no external deps).
 * JSON parser: nlohmann/json via CMake FetchContent.
 *
 * @author  EMBI Simulator Project
 * @version 1.0.0
 */

#include "utils/ConfigLoader.hpp"
#include "utils/FileUtils.hpp"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>

namespace embi {

// ─── Helpers ─────────────────────────────────────────────────────────────────

namespace {

/// Trims leading/trailing whitespace from a string in-place.
std::string trim(const std::string& s) {
    const std::string ws = " \t\r\n";
    std::size_t start    = s.find_first_not_of(ws);
    if (start == std::string::npos) return {};
    std::size_t end = s.find_last_not_of(ws);
    return s.substr(start, end - start + 1);
}

/// Strips trailing comment (# ...) from a YAML value string.
std::string stripYAMLComment(const std::string& val) {
    auto pos = val.find('#');
    return (pos == std::string::npos) ? val : val.substr(0, pos);
}

/// Parses a flat YAML file into a string→string map of key:value pairs.
std::unordered_map<std::string, std::string> parseYAMLMap(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("ConfigLoader: cannot open '" + path + "'");
    }

    std::unordered_map<std::string, std::string> map;
    std::string line;

    while (std::getline(file, line)) {
        // Skip comments and blank lines
        std::string trimmed = trim(line);
        if (trimmed.empty() || trimmed[0] == '#') continue;

        auto colon = trimmed.find(':');
        if (colon == std::string::npos) continue;

        std::string key = trim(trimmed.substr(0, colon));
        std::string val = trim(stripYAMLComment(trimmed.substr(colon + 1)));

        if (!key.empty()) {
            map[key] = val;
        }
    }
    return map;
}

/// Safely converts string to double; throws on parse error.
double toDouble(const std::string& key, const std::string& val) {
    try {
        std::size_t pos;
        double result = std::stod(val, &pos);
        if (pos != val.size()) throw std::exception{};
        return result;
    } catch (...) {
        throw std::invalid_argument(
            "ConfigLoader: '" + key + "' has invalid numeric value '" + val + "'");
    }
}

/// Safely converts string to uint64_t; throws on parse error.
uint64_t toUint64(const std::string& key, const std::string& val) {
    try {
        std::size_t pos;
        uint64_t result = static_cast<uint64_t>(std::stoull(val, &pos));
        if (pos != val.size()) throw std::exception{};
        return result;
    } catch (...) {
        throw std::invalid_argument(
            "ConfigLoader: '" + key + "' has invalid integer value '" + val + "'");
    }
}

/// Safely converts string to size_t; throws on parse error.
std::size_t toSizeT(const std::string& key, const std::string& val) {
    return static_cast<std::size_t>(toUint64(key, val));
}

/// Converts "true" / "false" / "1" / "0" string to bool.
bool toBool(const std::string& key, const std::string& val) {
    if (val == "true" || val == "1" || val == "yes")  return true;
    if (val == "false" || val == "0" || val == "no")  return false;
    throw std::invalid_argument(
        "ConfigLoader: '" + key + "' has invalid boolean value '" + val + "'");
}

/// Applies a YAML key→value map to a Config struct.
void applyYAMLToConfig(const std::unordered_map<std::string, std::string>& m, Config& cfg) {
    auto it = [&](const char* key) -> std::string {
        auto found = m.find(key);
        return (found != m.end()) ? found->second : std::string{};
    };
    auto has = [&](const char* key) { return m.count(key) > 0; };

    if (has("ticks"))           cfg.ticks          = toUint64("ticks", it("ticks"));
    if (has("num_processes"))   cfg.num_processes   = toSizeT("num_processes", it("num_processes"));
    if (has("seed"))            cfg.seed            = toUint64("seed", it("seed"));
    if (has("scheduler"))       cfg.scheduler_name  = it("scheduler");
    if (has("scheduler_name"))  cfg.scheduler_name  = it("scheduler_name");
    if (has("workload"))        cfg.workload_name   = it("workload");
    if (has("workload_name"))   cfg.workload_name   = it("workload_name");
    if (has("workload_profile"))cfg.workload_profile = it("workload_profile");
    if (has("trace_file"))      cfg.trace_file      = it("trace_file");
    if (has("arrival_rate"))    cfg.arrival_rate    = toDouble("arrival_rate", it("arrival_rate"));
    if (has("service_rate"))    cfg.service_rate    = toDouble("service_rate", it("service_rate"));
    if (has("alpha"))           cfg.alpha           = toDouble("alpha", it("alpha"));
    if (has("beta"))            cfg.beta            = toDouble("beta", it("beta"));
    if (has("M"))               cfg.M               = toDouble("M", it("M"));
    if (has("pareto_shape"))    cfg.pareto_shape    = toDouble("pareto_shape", it("pareto_shape"));
    if (has("pareto_scale"))    cfg.pareto_scale    = toDouble("pareto_scale", it("pareto_scale"));
    if (has("burst_on_rate"))   cfg.burst_on_rate   = toDouble("burst_on_rate", it("burst_on_rate"));
    if (has("burst_off_rate"))  cfg.burst_off_rate  = toDouble("burst_off_rate", it("burst_off_rate"));
    if (has("burst_p_on_off"))  cfg.burst_p_on_off  = toDouble("burst_p_on_off", it("burst_p_on_off"));
    if (has("burst_p_off_on"))  cfg.burst_p_off_on  = toDouble("burst_p_off_on", it("burst_p_off_on"));
    if (has("uniform_lo"))      cfg.uniform_lo      = toDouble("uniform_lo", it("uniform_lo"));
    if (has("uniform_hi"))      cfg.uniform_hi      = toDouble("uniform_hi", it("uniform_hi"));
    if (has("output_dir"))      cfg.output_dir      = it("output_dir");
    if (has("output"))          cfg.output_dir      = it("output");
    if (has("log_freq"))        cfg.log_freq        = toUint64("log_freq", it("log_freq"));
    if (has("binary_log"))      cfg.binary_log      = toBool("binary_log", it("binary_log"));
    if (has("null_log"))        cfg.null_log        = toBool("null_log", it("null_log"));
    if (has("embi_clipped"))    cfg.embi_clipped    = toBool("embi_clipped", it("embi_clipped"));
}

} // anonymous namespace

// ─── Public API ──────────────────────────────────────────────────────────────

Config ConfigLoader::load(const std::string& path) {
    if (!FileUtils::exists(path)) {
        throw std::runtime_error("ConfigLoader::load: file not found: '" + path + "'");
    }

    std::string ext = FileUtils::extension(path);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    if (ext == ".yaml" || ext == ".yml") {
        return loadYAML(path);
    }
    if (ext == ".json") {
        return loadJSON(path);
    }
    throw std::invalid_argument(
        "ConfigLoader::load: unsupported file extension '" + ext +
        "'. Supported: .yaml, .yml, .json");
}

ExperimentConfig ConfigLoader::loadExperiment(const std::string& path) {
    if (!FileUtils::exists(path)) {
        throw std::runtime_error("ConfigLoader::loadExperiment: file not found: '" + path + "'");
    }
    std::string ext = FileUtils::extension(path);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    if (ext != ".json") {
        throw std::invalid_argument(
            "ConfigLoader::loadExperiment: experiment configs must be JSON (got '" + ext + "')");
    }
    return loadExperimentJSON(path);
}

// ─── Internal parsers ────────────────────────────────────────────────────────

Config ConfigLoader::loadYAML(const std::string& path) {
    auto map = parseYAMLMap(path);
    Config cfg;
    applyYAMLToConfig(map, cfg);
    cfg.validate();
    return cfg;
}

Config ConfigLoader::loadJSON(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("ConfigLoader: cannot open '" + path + "'");
    }

    nlohmann::json j;
    try {
        file >> j;
    } catch (const nlohmann::json::exception& ex) {
        throw std::runtime_error(
            "ConfigLoader: JSON parse error in '" + path + "': " + ex.what());
    }

    Config cfg;
    auto get = [&](const char* key, auto& field) {
        if (j.contains(key) && !j[key].is_null()) {
            using T = std::decay_t<decltype(field)>;
            field = j[key].get<T>();
        }
    };

    get("ticks",          cfg.ticks);
    get("num_processes",  cfg.num_processes);
    get("seed",           cfg.seed);
    get("arrival_rate",   cfg.arrival_rate);
    get("service_rate",   cfg.service_rate);
    get("alpha",          cfg.alpha);
    get("beta",           cfg.beta);
    get("M",              cfg.M);
    get("pareto_shape",   cfg.pareto_shape);
    get("pareto_scale",   cfg.pareto_scale);
    get("burst_on_rate",  cfg.burst_on_rate);
    get("burst_off_rate", cfg.burst_off_rate);
    get("burst_p_on_off", cfg.burst_p_on_off);
    get("burst_p_off_on", cfg.burst_p_off_on);
    get("uniform_lo",     cfg.uniform_lo);
    get("uniform_hi",     cfg.uniform_hi);
    get("log_freq",       cfg.log_freq);
    get("binary_log",     cfg.binary_log);
    get("null_log",       cfg.null_log);
    get("embi_clipped",   cfg.embi_clipped);

    // String fields
    if (j.contains("scheduler") && j["scheduler"].is_string())
        cfg.scheduler_name = j["scheduler"].get<std::string>();
    if (j.contains("scheduler_name") && j["scheduler_name"].is_string())
        cfg.scheduler_name = j["scheduler_name"].get<std::string>();
    if (j.contains("workload") && j["workload"].is_string())
        cfg.workload_name  = j["workload"].get<std::string>();
    if (j.contains("workload_name") && j["workload_name"].is_string())
        cfg.workload_name  = j["workload_name"].get<std::string>();
    if (j.contains("output_dir") && j["output_dir"].is_string())
        cfg.output_dir = j["output_dir"].get<std::string>();
    if (j.contains("output") && j["output"].is_string())
        cfg.output_dir = j["output"].get<std::string>();
    if (j.contains("trace_file") && j["trace_file"].is_string())
        cfg.trace_file = j["trace_file"].get<std::string>();
    if (j.contains("workload_profile") && j["workload_profile"].is_string())
        cfg.workload_profile = j["workload_profile"].get<std::string>();

    cfg.validate();
    return cfg;
}

ExperimentConfig ConfigLoader::loadExperimentJSON(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("ConfigLoader: cannot open '" + path + "'");
    }

    nlohmann::json j;
    try {
        file >> j;
    } catch (const nlohmann::json::exception& ex) {
        throw std::runtime_error(
            "ConfigLoader: JSON parse error in '" + path + "': " + ex.what());
    }

    ExperimentConfig exp;

    // ── Sweep axes (arrays) ──────────────────────────────────────────────────
    if (j.contains("schedulers") && j["schedulers"].is_array())
        exp.schedulers = j["schedulers"].get<std::vector<std::string>>();
    if (j.contains("workloads") && j["workloads"].is_array())
        exp.workloads  = j["workloads"].get<std::vector<std::string>>();
    if (j.contains("seeds") && j["seeds"].is_array())
        exp.seeds = j["seeds"].get<std::vector<uint64_t>>();
    if (j.contains("arrival_rates") && j["arrival_rates"].is_array())
        exp.arrival_rates = j["arrival_rates"].get<std::vector<double>>();
    if (j.contains("arrival_rate_range") && j["arrival_rate_range"].is_array())
        exp.arrival_rates = j["arrival_rate_range"].get<std::vector<double>>();

    // ── Scalar fields ────────────────────────────────────────────────────────
    auto get = [&](const char* key, auto& field) {
        if (j.contains(key) && !j[key].is_null() && !j[key].is_array()) {
            using T = std::decay_t<decltype(field)>;
            field = j[key].get<T>();
        }
    };

    get("ticks",          exp.ticks);
    get("num_processes",  exp.num_processes);
    get("service_rate",   exp.service_rate);
    get("alpha",          exp.alpha);
    get("beta",           exp.beta);
    get("M",              exp.M);
    get("pareto_shape",   exp.pareto_shape);
    get("pareto_scale",   exp.pareto_scale);
    get("burst_on_rate",  exp.burst_on_rate);
    get("burst_off_rate", exp.burst_off_rate);
    get("burst_p_on_off", exp.burst_p_on_off);
    get("burst_p_off_on", exp.burst_p_off_on);
    get("log_freq",       exp.log_freq);
    get("binary_log",     exp.binary_log);
    get("null_log",       exp.null_log);

    if (j.contains("output_dir") && j["output_dir"].is_string())
        exp.output_dir = j["output_dir"].get<std::string>();
    if (j.contains("output") && j["output"].is_string())
        exp.output_dir = j["output"].get<std::string>();
    if (j.contains("trace_file") && j["trace_file"].is_string())
        exp.trace_file = j["trace_file"].get<std::string>();

    // Validate by expanding (which validates each derived Config)
    (void)exp.expand();  // validates; result discarded here, used later in Experiment::run()

    return exp;
}

} // namespace embi
