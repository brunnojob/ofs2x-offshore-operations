#include "operations.hpp"
#include <iostream>
#include <sstream>
int main() {
    offshore::MemoryRepository repository;
    offshore::Operations operations(repository);
    std::string line;
    while (std::getline(std::cin, line)) {
        try {
            std::istringstream input(line);
            std::string command; input >> command;
            if (command == "crew") {
                offshore::Crew crew; std::string kind; std::int64_t expiry;
                if (!(input >> crew.id >> crew.vessel >> crew.enabled)) throw std::invalid_argument("crew ID VESSEL ENABLED [CERT EXPIRY]...");
                while (input >> kind) {
                    if (!(input >> expiry)) throw std::invalid_argument("certificate expiry required");
                    crew.certificates.push_back({kind, expiry});
                }
                operations.register_crew(std::move(crew));
            } else if (command == "station") {
                offshore::Rule rule; std::string kind;
                if (!(input >> rule.station >> rule.minimum)) throw std::invalid_argument("station ID MINIMUM [CERT]...");
                while (input >> kind) rule.required.insert(kind);
                operations.station(std::move(rule));
            } else if (command == "start") {
                std::string id, vessel;
                if (!(input >> id >> vessel)) throw std::invalid_argument("start ID VESSEL");
                operations.start(id, vessel, offshore::Clock::now());
            } else if (command == "check-in") {
                std::string id, station; std::int64_t now;
                if (!(input >> id >> station >> now)) throw std::invalid_argument("check-in CREW STATION UNIX_SECONDS");
                const auto result = operations.check_in(id, station, now);
                std::cout << (result.eligible ? "accepted" : "denied") << '\n';
                for (const auto& reason : result.failures) std::cout << reason << '\n';
            } else if (command == "status") {
                for (const auto& item : operations.deficiencies()) std::cout << item << '\n';
            } else if (command == "finish") {
                const auto result = operations.finish(offshore::Clock::now());
                std::cout << result.id << ' ' << result.duration.count() << " seconds\n";
            } else if (command == "audit") {
                for (const auto& e : repository.events()) std::cout << e.sequence << ' ' << e.action << ' ' << e.subject << '\n';
            } else if (command == "quit") break;
            else throw std::invalid_argument("unknown command");
        } catch (const std::exception& e) { std::cerr << "error: " << e.what() << '\n'; }
    }
}
