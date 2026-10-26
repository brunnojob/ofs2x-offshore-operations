# Persistence boundary

`Operations` owns crew, station requirements, assignments, completed drill identifiers and an active drill. `Repository` is an audit append/read interface, not a state-hydration contract. The default `MemoryRepository` is synchronized, but the application intentionally loses runtime state after shutdown.

## Atomicity

The domain appends the audit event before mutating operational state. Adapters must either append an event in full or throw without side effects. Subsequent in-memory mutations can still throw after an append succeeds; append-first ordering is not a transaction or a strong exception guarantee for the complete operation.

A durable implementation must commit domain records and corresponding audit events within one database transaction, with failure recovery that preserves ordering.

## Storage and recovery

A durable storage layer should model crew, certificates, vessels, stations, drills, attendance and events separately. Enforce unique crew and drill identifiers, a unique `(drill_id, crew_id)` attendance key, and sequential event ordering. Certificates use Unix timestamps; drill timing uses a monotonic clock during the process lifetime.

Do not reconstruct a monotonic clock instant after restart. Restore wall-clock timestamps for reporting, explicitly mark incomplete drills, and add versioned snapshot/load operations before supporting restart recovery. This release does not provide a database adapter or restart recovery.
