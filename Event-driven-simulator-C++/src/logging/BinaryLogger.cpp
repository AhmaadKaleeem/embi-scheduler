/**
 * @file BinaryLogger.cpp
 * @brief BinaryLogger implementation: packed binary log with header.
 *
 * @author  EMBI Simulator Project
 * @version 1.0.0
 */

#include "logging/BinaryLogger.hpp"

#include <chrono>
#include <cstring>
#include <stdexcept>

namespace embi {

BinaryLogger::BinaryLogger(const std::string& path, std::size_t buffer_records)
    : path_(path)
    , buffer_cap_(buffer_records > 0 ? buffer_records : kDefaultBufferRecords)
{
    file_.open(path, std::ios::binary | std::ios::out | std::ios::trunc);
    if (!file_.is_open()) {
        throw std::runtime_error("BinaryLogger: cannot open '" + path + "'");
    }
    buffer_.reserve(buffer_cap_);
    writeFileHeader();
}

BinaryLogger::~BinaryLogger() {
    if (!closed_) {
        try { close(); } catch (...) {}
    }
}

void BinaryLogger::writeFileHeader() {
    BinaryLogHeader hdr;
    hdr.record_size = static_cast<uint32_t>(sizeof(BinaryRecord));

    // Current time as nanoseconds since epoch
    auto now  = std::chrono::system_clock::now();
    auto epoch = now.time_since_epoch();
    hdr.start_timestamp_ns =
        static_cast<uint64_t>(
            std::chrono::duration_cast<std::chrono::nanoseconds>(epoch).count());

    file_.write(reinterpret_cast<const char*>(&hdr), sizeof(hdr));
}

void BinaryLogger::write(const LogRecord& r) {
    if (closed_) return;

    BinaryRecord br;
    br.tick            = r.tick;
    br.pid             = static_cast<uint32_t>(r.pid);
    br.queue_length    = static_cast<int32_t>(r.queue_length);
    br.arrival_rate    = static_cast<float>(r.arrival_rate);
    br.service_rate    = static_cast<float>(r.service_rate);
    br.lambda_hat      = static_cast<float>(r.lambda_hat);
    br.mu_hat          = static_cast<float>(r.mu_hat);
    br.scheduler_score = static_cast<float>(r.scheduler_score);
    br.chosen          = static_cast<uint8_t>(r.chosen ? 1 : 0);
    br.waiting_time    = static_cast<float>(r.waiting_time);
    br.completion_time = static_cast<float>(r.completion_time);
    br.throughput      = static_cast<float>(r.throughput);
    br.lyapunov_v      = static_cast<float>(r.lyapunov_v);
    br.lyapunov_drift  = static_cast<float>(r.lyapunov_drift);

    // Zero-fill padding
    std::memset(br.pad, 0, sizeof(br.pad));

    buffer_.push_back(br);
    records_written_++;

    if (buffer_.size() >= buffer_cap_) {
        flushBuffer();
    }
}

void BinaryLogger::flush() {
    if (!closed_) {
        flushBuffer();
        file_.flush();
    }
}

void BinaryLogger::close() {
    if (!closed_) {
        flushBuffer();
        file_.flush();
        file_.close();
        closed_ = true;
    }
}

void BinaryLogger::flushBuffer() {
    if (buffer_.empty()) return;
    file_.write(reinterpret_cast<const char*>(buffer_.data()),
                static_cast<std::streamsize>(buffer_.size() * sizeof(BinaryRecord)));
    buffer_.clear();
}

uint64_t BinaryLogger::recordsWritten() const noexcept {
    return records_written_;
}

} 
