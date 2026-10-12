# Persistence boundary

`Repository` defines audit append and read operations; `MemoryRepository` is the default adapter. Crew, station rules and active drills remain in the domain process.

A future durable integration must store crew identifiers, vessel assignments, certificates, station rules, drill start/finish and attendance. Use a unique crew identifier, a unique drill identifier and a unique `(drill, crew)` attendance key. Preserve the distinction between Unix expiry timestamps and monotonic elapsed time.

The current interface is an audit sink, not state hydration. Before enabling restart recovery, extend it with an explicit snapshot/load contract and commit domain changes and events in one transaction. Map persisted certificate records back to `Crew`, then register rules before starting a drill. Do not silently replay an active drill across process restarts: its monotonic timestamp belongs to the original process.

Do not perform side effects inside a failing `append`. Do not call the domain recursively from an adapter. No database implementation or connection string is included.
