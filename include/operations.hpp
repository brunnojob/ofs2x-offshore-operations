#pragma once
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <map>
#include <limits>
#include <mutex>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace offshore {
using Seconds = std::chrono::seconds;
using Clock = std::chrono::steady_clock;
struct Certificate {
    std::string kind;
    std::int64_t valid_until;
};
struct Crew {
    std::string id;
    std::string vessel;
    bool enabled;
    std::vector<Certificate> certificates;
};
struct Rule {
    std::string station;
    std::set<std::string> required;
    std::size_t minimum;
};
struct Decision {
    bool eligible;
    std::vector<std::string> failures;
};
struct DrillResult {
    std::string id;
    Seconds duration;
    std::map<std::string, std::set<std::string>> attendance;
};
struct Event {
    std::uint64_t sequence;
    std::string action;
    std::string subject;
};
struct StationStatus {
    std::string station;
    std::size_t assigned;
    std::size_t minimum;
    std::size_t shortage;
};
struct DrillStatus {
    std::string id;
    std::string vessel;
    bool active;
    std::vector<StationStatus> stations;
};
struct Repository {
    virtual ~Repository() = default;
    virtual void append(const Event&) = 0;
    virtual std::vector<Event> events() const = 0;
};
class MemoryRepository final : public Repository {
    mutable std::mutex mutex_;
    std::vector<Event> data_;
public:
    void append(const Event& event) override {
        std::lock_guard lock(mutex_);
        data_.push_back(event);
    }
    [[nodiscard]] std::vector<Event> events() const override {
        std::lock_guard lock(mutex_);
        return data_;
    }
};
class Operations {
    mutable std::mutex mutex_;
    Repository& repository_;
    std::map<std::string, Crew> crew_;
    std::map<std::string, Rule> rules_;
    std::map<std::string, std::set<std::string>> attendance_;
    std::map<std::string, std::string> assignments_;
    std::set<std::string> completed_drills_;
    std::string drill_;
    std::string vessel_;
    Clock::time_point started_{};
    bool active_ = false;
    std::uint64_t sequence_ = 0;
    void emit(const std::string& action, const std::string& subject) {
        if (sequence_ == std::numeric_limits<std::uint64_t>::max()) throw std::overflow_error("audit sequence exhausted");
        repository_.append({sequence_ + 1, action, subject});
        ++sequence_;
    }
    Decision evaluate(const Crew& crew, const Rule& rule, std::int64_t now) const {
        Decision result{true, {}};
        if (!crew.enabled) result.failures.push_back("disabled");
        if (crew.vessel != vessel_) result.failures.push_back("wrong-vessel");
        for (const auto& kind : rule.required) {
            const bool valid = std::any_of(crew.certificates.begin(), crew.certificates.end(), [&](const auto& c) {
                return c.kind == kind && c.valid_until > now;
            });
            if (!valid) result.failures.push_back("missing-or-expired:" + kind);
        }
        result.eligible = result.failures.empty();
        return result;
    }
public:
    explicit Operations(Repository& repository) : repository_(repository) {
        for (const auto& event : repository.events()) sequence_ = std::max(sequence_, event.sequence);
    }
    void register_crew(Crew crew) {
        std::lock_guard lock(mutex_);
        if (active_ || crew.id.empty() || crew.vessel.empty()) throw std::invalid_argument("invalid crew mutation");
        for (const auto& certificate : crew.certificates)
            if (certificate.kind.empty() || certificate.valid_until < 0) throw std::invalid_argument("invalid certificate");
        const auto id = crew.id;
        emit(crew_.contains(id) ? "crew-updated" : "crew-registered", id);
        crew_.insert_or_assign(id, std::move(crew));
    }
    void station(Rule rule) {
        std::lock_guard lock(mutex_);
        if (active_ || rule.station.empty() || rule.minimum == 0 || rule.required.contains("")) throw std::invalid_argument("invalid station");
        const auto id = rule.station;
        emit(rules_.contains(id) ? "station-updated" : "station-configured", id);
        rules_.insert_or_assign(id, std::move(rule));
    }
    void start(std::string id, std::string vessel, Clock::time_point now) {
        std::lock_guard lock(mutex_);
        if (active_ || id.empty() || vessel.empty() || rules_.empty() || completed_drills_.contains(id)) throw std::logic_error("cannot start drill");
        emit("drill-started", id);
        drill_ = std::move(id);
        vessel_ = std::move(vessel);
        started_ = now;
        attendance_.clear();
        assignments_.clear();
        active_ = true;
    }
    Decision check_in(const std::string& crew_id, const std::string& station_id, std::int64_t now) {
        std::lock_guard lock(mutex_);
        if (!active_) throw std::logic_error("no active drill");
        if (now < 0) throw std::invalid_argument("timestamp must be non-negative");
        const auto result = evaluate(crew_.at(crew_id), rules_.at(station_id), now);
        if (!result.eligible) {
            emit("check-in-denied", crew_id);
            return result;
        }
        if (const auto assignment = assignments_.find(crew_id); assignment != assignments_.end()) {
            if (assignment->second != station_id) throw std::logic_error("crew assigned elsewhere");
            return result;
        }
        emit("check-in-accepted", crew_id + ":" + station_id);
        attendance_[station_id].insert(crew_id);
        assignments_.emplace(crew_id, station_id);
        return result;
    }
    [[nodiscard]] DrillStatus status() const {
        std::lock_guard lock(mutex_);
        DrillStatus result{drill_, vessel_, active_, {}};
        for (const auto& [id, rule] : rules_) {
            const auto it = attendance_.find(id);
            const auto count = it == attendance_.end() ? std::size_t{0} : it->second.size();
            result.stations.push_back({id, count, rule.minimum, count < rule.minimum ? rule.minimum - count : 0});
        }
        return result;
    }
    std::vector<std::string> deficiencies() const {
        std::lock_guard lock(mutex_);
        std::vector<std::string> result;
        for (const auto& [id, rule] : rules_) {
            const auto it = attendance_.find(id);
            const auto count = it == attendance_.end() ? 0 : it->second.size();
            if (count < rule.minimum) result.push_back(id + ":" + std::to_string(rule.minimum - count));
        }
        return result;
    }
    DrillResult finish(Clock::time_point now) {
        std::lock_guard lock(mutex_);
        if (!active_ || now < started_) throw std::logic_error("invalid drill clock");
        emit("drill-finished", drill_);
        completed_drills_.insert(drill_);
        active_ = false;
        return {drill_, std::chrono::duration_cast<Seconds>(now - started_), attendance_};
    }
};
}

