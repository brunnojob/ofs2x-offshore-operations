#include "operations.hpp"
#include <cassert>
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
    const auto result = operations.finish(start + offshore::Seconds(90));
    assert(result.duration.count() == 90);
    assert(result.attendance.at("muster").size() == 1);
    rejected = false;
    try { operations.check_in("a", "muster", 100); } catch (const std::logic_error&) { rejected = true; }
    assert(rejected);
}
