#include "operations.hpp"
#include <charconv>
#include <iostream>
#include <sstream>

namespace {
void require_end(std::istringstream& input) {
    std::string extra;
    if (input >> extra) throw std::invalid_argument("unexpected argument: " + extra);
}

template <typename T>
T parse_number(const std::string& token, const std::string& field) {
    T value{};
    const auto [position, error] = std::from_chars(token.data(), token.data() + token.size(), value);
    if (error != std::errc{} || position != token.data() + token.size())
        throw std::invalid_argument("invalid " + field + ": " + token);
    return value;
}
}

int main() {
    offshore::MemoryRepository repository;
    offshore::Operations operations(repository);
    bool failed = false;
    std::string line;
    while (std::getline(std::cin, line)) {
        try {
            std::istringstream input(line);
            std::string command;
            if (!(input >> command)) continue;
            if (command == "crew") {
                offshore::Crew crew;
                std::string kind;
                std::string enabled;
                if (!(input >> crew.id >> crew.vessel >> enabled)) throw std::invalid_argument("crew ID VESSEL ENABLED [CERT EXPIRY]...");
                const auto flag = parse_number<unsigned>(enabled, "enabled");
                if (flag > 1) throw std::invalid_argument("enabled must be 0 or 1");
                crew.enabled = flag == 1;
                while (input >> kind) {
                    std::string raw_expiry;
                    if (!(input >> raw_expiry)) throw std::invalid_argument("certificate expiry required");
                    const auto expiry = parse_number<std::int64_t>(raw_expiry, "certificate expiry");
                    crew.certificates.push_back({kind, expiry});
                }
                operations.register_crew(std::move(crew));
            } else if (command == "station") {
                offshore::Rule rule;
                std::string kind;
                std::string raw_minimum;
                if (!(input >> rule.station >> raw_minimum)) throw std::invalid_argument("station ID MINIMUM [CERT]...");
                rule.minimum = parse_number<std::size_t>(raw_minimum, "minimum");
                while (input >> kind) rule.required.insert(kind);
                operations.station(std::move(rule));
            } else if (command == "start") {
                std::string id, vessel;
                if (!(input >> id >> vessel)) throw std::invalid_argument("start ID VESSEL");
                require_end(input);
                operations.start(id, vessel, offshore::Clock::now());
            } else if (command == "check-in") {
                std::string id, station;
                std::string timestamp;
                if (!(input >> id >> station >> timestamp)) throw std::invalid_argument("check-in CREW STATION UNIX_SECONDS");
                const auto now = parse_number<std::int64_t>(timestamp, "timestamp");
                require_end(input);
                const auto result = operations.check_in(id, station, now);
                std::cout << (result.eligible ? "accepted" : "denied") << '\n';
                for (const auto& reason : result.failures) std::cout << reason << '\n';
            } else if (command == "status") {
                require_end(input);
                const auto status = operations.status();
                std::cout << (status.active ? "active" : "inactive") << '\n';
                for (const auto& station : status.stations)
                    std::cout << station.station << " " << station.assigned << "/" << station.minimum << " shortage=" << station.shortage << '\n';
            } else if (command == "finish") {
                require_end(input);
                const auto result = operations.finish(offshore::Clock::now());
                std::cout << result.id << ' ' << result.duration.count() << " seconds\n";
            } else if (command == "audit") {
                require_end(input);
                for (const auto& e : repository.events()) std::cout << e.sequence << ' ' << e.action << ' ' << e.subject << '\n';
            } else if (command == "help") {
                require_end(input);
                std::cout << "crew ID VESSEL ENABLED [CERT EXPIRY]...\nstation ID MINIMUM [CERT]...\nstart ID VESSEL\ncheck-in CREW STATION UNIX_SECONDS\nstatus\nfinish\naudit\nquit\n";
            } else if (command == "quit") {
                require_end(input);
                break;
            } else {
                throw std::invalid_argument("unknown command");
            }
        } catch (const std::exception& e) {
            failed = true;
            std::cerr << "error: " << e.what() << '\n';
        }
    }
    return failed || std::cin.bad() ? 1 : 0;
}
