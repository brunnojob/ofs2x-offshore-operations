#include "operations.hpp"
#include <cassert>
#include <stdexcept>
template <typename Exception, typename Action>
void expect_error(Action action) {
    bool rejected = false;
    try {
        action();
    } catch (const Exception&) {
        rejected = true;
    }
    assert(rejected);
}

class FailingRepository final : public offshore::Repository {
public:
    void append(const offshore::Event&) override {
        throw std::runtime_error("storage unavailable");
    }

    std::vector<offshore::Event> events() const override {
        return {};
    }
};

int main() {
    offshore::MemoryRepository repository;
    offshore::Operations operations(repository);
    operations.register_crew({"a", "v1", true, {{"BST", 200}}});
    operations.register_crew({"b", "v1", true, {{"BST", 100}}});
    operations.register_crew({"c", "v2", true, {{"BST", 200}}});
    operations.station({"muster", {"BST"}, 2});
    operations.station({"fire", {"BST"}, 1});
    const auto start = offshore::Clock::now();
    operations.start("d1", "v1", start);
    assert(operations.check_in("a", "muster", 100).eligible);
    assert(operations.check_in("a", "muster", 100).eligible);
    assert(!operations.check_in("b", "muster", 100).eligible);
    assert(!operations.check_in("c", "muster", 100).eligible);
    bool rejected = false;
    try { operations.check_in("a", "fire", 100); } catch (const std::logic_error&) { rejected = true; }
    assert(rejected);
    assert(operations.deficiencies().size() == 2);
    assert(operations.status().active);
    assert(operations.status().stations.at(1).assigned == 1);
    const auto result = operations.finish(start + offshore::Seconds(90));
    assert(result.duration.count() == 90);
    assert(result.attendance.at("muster").size() == 1);
    assert(!operations.status().active);
    rejected = false;
    try { operations.start("d1", "v1", start); } catch (const std::logic_error&) { rejected = true; }
    assert(rejected);
    rejected = false;
    try { operations.check_in("a", "muster", 100); } catch (const std::logic_error&) { rejected = true; }
    assert(rejected);
    operations.start("d2", "v1", start);
    assert(operations.status().stations.at(0).assigned == 0);
    static_cast<void>(operations.finish(start));
    const auto events = repository.events();
    for (std::size_t i = 0; i < events.size(); ++i) assert(events[i].sequence == i + 1);

    offshore::MemoryRepository edge_repository;
    offshore::Operations edge(edge_repository);
    edge.register_crew({"disabled", "v1", false, {{"BST", 200}}});
    edge.register_crew({"valid", "v1", true, {{"BST", 200}}});
    edge.station({"muster", {"BST"}, 1});
    edge.start("edge", "v1", start);
    const auto before = edge_repository.events().size();
    expect_error<std::invalid_argument>([&] { edge.check_in("valid", "muster", -1); });
    expect_error<std::out_of_range>([&] { edge.check_in("unknown", "muster", 100); });
    expect_error<std::out_of_range>([&] { edge.check_in("valid", "unknown", 100); });
    assert(edge_repository.events().size() == before);
    assert(edge.status().stations.at(0).assigned == 0);
    const auto disabled = edge.check_in("disabled", "muster", 100);
    assert(!disabled.eligible);
    assert(disabled.failures.at(0) == "disabled");
    expect_error<std::invalid_argument>([&] { edge.station({"other", {}, 1}); });
    expect_error<std::logic_error>([&] { edge.finish(start - offshore::Seconds(1)); });
    assert(edge.status().active);

    FailingRepository failing_repository;
    offshore::Operations unavailable(failing_repository);
    expect_error<std::runtime_error>([&] { unavailable.station({"muster", {}, 1}); });
    assert(unavailable.status().stations.empty());
    expect_error<std::runtime_error>([&] { unavailable.register_crew({"valid", "v1", true, {}}); });
}

