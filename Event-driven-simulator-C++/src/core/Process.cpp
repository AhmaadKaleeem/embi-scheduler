/**
 * @file Process.cpp
 * @brief Implementation of the Process simulation model.
 *
 * @author  EMBI Simulator Project
 * @version 1.0.0
 */

#include "core/Process.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>

namespace embi {

// ─── Construction ────────────────────────────────────────────────────────────

Process::Process(std::size_t id_,
                 double      true_lambda,
                 double      true_mu,
                 double      alpha_,
                 double      beta_,
                 std::size_t queue_preallocate)
    : id(id_)
    , true_arrival_rate(true_lambda)
    , true_service_rate(true_mu)
    , lambda_hat(true_lambda)   // Warm-start: assume true rate
    , mu_hat(true_mu)           // Warm-start: assume true rate
    , alpha(alpha_)
    , beta(beta_)
{
    // Pre-size the deque backing vector would require std::vector instead.
    // Using a deque satisfies O(1) push_back/pop_front without manual sizing.
    // Hint: reserve the first chunk to avoid early reallocations.
    // (std::deque has no reserve(), so we insert-and-clear as a warm-up.)
    // For performance, we simply leave the deque empty; it rarely grows large
    // in a stable queueing system.
    (void)queue_preallocate;  // parameter reserved for future optimisation
}

// ─── Arrival ─────────────────────────────────────────────────────────────────

void Process::arrival(double tick) {
    // Update EWMA arrival rate from inter-arrival time
    if (last_arrival_time >= 0.0) {
        double inter_arrival = tick - last_arrival_time;
        if (inter_arrival > 0.0) {
            updateArrivalEstimate(inter_arrival);
        }
    } else {
        // First arrival: set first_arrival_time
        first_arrival_time = tick;
    }

    last_arrival_time = tick;
    queue_length++;
    arrival_count++;

    // Push arrival tick for per-job waiting time tracking
    job_arrival_queue_.push_back(tick);
}

// ─── Service ─────────────────────────────────────────────────────────────────

double Process::service(double tick) {
    if (queue_length <= 0 || job_arrival_queue_.empty()) {
        // Defensive guard: should not happen in a correct simulation
        return 0.0;
    }

    // Compute waiting time for the oldest job in queue
    double arrival_tick = job_arrival_queue_.front();
    job_arrival_queue_.pop_front();

    double waiting_time = tick - arrival_tick;
    // Clamp to 0 (floating-point should never be negative, but guard anyway)
    waiting_time = std::max(0.0, waiting_time);

    // Update EWMA service estimate (unit service model: 1 tick per job)
    updateServiceEstimate(1.0);

    queue_length--;
    completed_jobs++;
    total_waiting_time += waiting_time;
    last_service_time   = tick;
    ticks_since_last_service = 0;

    return waiting_time;
}

// ─── Idle tick ────────────────────────────────────────────────────────────────

void Process::tickIdle(uint64_t starvation_threshold) {
    ticks_since_last_service++;
    max_starvation_ticks = std::max(max_starvation_ticks, ticks_since_last_service);

    if (ticks_since_last_service == starvation_threshold && queue_length > 0) {
        starvation_events++;
    }
}

// ─── EWMA updates ────────────────────────────────────────────────────────────

void Process::updateArrivalEstimate(double inter_arrival_ticks) {
    // Guard: inter_arrival must be positive for a well-defined rate
    if (inter_arrival_ticks <= 0.0) return;

    double observed_rate = 1.0 / inter_arrival_ticks;
    lambda_hat = (1.0 - alpha) * lambda_hat + alpha * observed_rate;
}

void Process::updateServiceEstimate(double service_ticks) {
    if (service_ticks <= 0.0) return;

    double observed_rate = 1.0 / service_ticks;
    mu_hat = (1.0 - beta) * mu_hat + beta * observed_rate;
}

// ─── Reset ────────────────────────────────────────────────────────────────────

void Process::reset() {
    queue_length             = 0;
    arrival_count            = 0;
    completed_jobs           = 0;
    first_arrival_time       = -1.0;
    last_arrival_time        = -1.0;
    last_service_time        = -1.0;
    total_waiting_time       = 0.0;
    ticks_since_last_service = 0;
    max_starvation_ticks     = 0;
    starvation_events        = 0;

    // Re-initialise EWMA to true rates (warm-start for next run)
    lambda_hat = true_arrival_rate;
    mu_hat     = true_service_rate;

    // Clear per-job arrival timestamps
    job_arrival_queue_.clear();
}

// ─── Computed properties ─────────────────────────────────────────────────────

double Process::averageWaitingTime() const noexcept {
    if (completed_jobs == 0) return 0.0;
    return total_waiting_time / static_cast<double>(completed_jobs);
}

double Process::cpuShare(uint64_t total_ticks) const noexcept {
    if (total_ticks == 0) return 0.0;
    return static_cast<double>(completed_jobs) / static_cast<double>(total_ticks);
}

} // namespace embi
