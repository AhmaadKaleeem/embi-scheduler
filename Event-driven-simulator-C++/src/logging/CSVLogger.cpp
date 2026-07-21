/**
 * @file CSVLogger.cpp
 * @brief Buffered CSV logger implementation.
 *
 * @author  EMBI Simulator Project
 * @version 1.0.0
 */

#include "logging/CSVLogger.hpp"

#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace embi {

CSVLogger::CSVLogger(const std::string& path, std::size_t buffer_records)
    : path_(path)
    , buffer_cap_(buffer_records > 0 ? buffer_records : kDefaultBufferRecords)
{
    file_.open(path, std::ios::out | std::ios::trunc);
    if (!file_.is_open()) {
        throw std::runtime_error("CSVLogger: cannot open '" + path + "'");
    }
    // Enable large write buffer (64 KB)
    file_.rdbuf()->pubsetbuf(nullptr, 65536);

    buffer_.reserve(buffer_cap_);
    writeHeader();
}

CSVLogger::~CSVLogger() {
    if (!closed_) {
        try { close(); } catch (...) {}
    }
}

void CSVLogger::writeHeader() {
    file_ << "tick,pid,queue,arrival,service,lambda_hat,mu_hat,"
             "scheduler_score,raw_score,chosen,waiting_time,completion_time,"
             "throughput,V,drift,gap,tau,branch\n";
}

void CSVLogger::write(const LogRecord& r) {
    if (closed_) return;

    // Format to string in the buffer to avoid repeated small writes
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(6)
        << r.tick          << ','
        << r.pid           << ','
        << r.queue_length  << ','
        << r.arrival_rate  << ','
        << r.service_rate  << ','
        << r.lambda_hat    << ','
        << r.mu_hat        << ','
        << r.scheduler_score << ','
        << r.raw_score     << ','
        << (r.chosen ? 1 : 0) << ','
        << r.waiting_time  << ','
        << r.completion_time << ','
        << r.throughput    << ','
        << r.lyapunov_v    << ','
        << r.lyapunov_drift << ','
        << r.gap           << ','
        << r.tau           << ','
        << r.branch
        << '\n';

    buffer_.push_back(oss.str());
    records_written_++;

    if (buffer_.size() >= buffer_cap_) {
        flushBuffer();
    }
}

void CSVLogger::flush() {
    if (!closed_) {
        flushBuffer();
        file_.flush();
    }
}

void CSVLogger::close() {
    if (!closed_) {
        flushBuffer();
        file_.flush();
        file_.close();
        closed_ = true;
    }
}

void CSVLogger::flushBuffer() {
    for (const auto& row : buffer_) {
        file_ << row;
    }
    buffer_.clear();
}

const std::string& CSVLogger::path() const noexcept {
    return path_;
}

uint64_t CSVLogger::recordsWritten() const noexcept {
    return records_written_;
}

} // namespace embi
