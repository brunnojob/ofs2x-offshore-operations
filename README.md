# OFS2X Offshore Operations

A native C++20 command-line engine for offshore crew management, emergency station staffing and timed safety drills. Its domain model enforces vessel-specific eligibility, certificate requirements and auditable attendance.

## Reference dashboard video

This recording shows the reference offshore dashboard. The implementation in this repository is a native C++20 CLI.

[![Watch the reference dashboard](evidence/preview.jpg)](https://github.com/brunnojob/ofs2x-offshore-operations/blob/main/evidence/demo.mp4)

[Download the MP4](https://github.com/brunnojob/ofs2x-offshore-operations/raw/refs/heads/main/evidence/demo.mp4)

The original recording is stored in `evidence/demo.mp4`; its checksum is recorded in [evidence/provenance.json](evidence/provenance.json).

## Features

- Crew registration, activation and vessel assignment.
- Station rules with staffing minimums and required certificates.
- Certificate expiry checks against Unix timestamps.
- Vessel isolation and prevention of simultaneous station assignments.
- Idempotent attendance, station occupancy and staffing shortages.
- Monotonic drill duration and prevention of completed drill ID reuse.
- Ordered audit events with an extensible repository interface.
- Mutex-protected operations and an independently synchronized memory repository.
- Input validation, structured status reporting and native regression tests.

## Requirements

A C++20 compiler (GCC 12+ recommended), GNU Make and a compatible host environment. Python 3 is required for the CLI regression tests.

## Build and test

```sh
make all test
./build/offshore
```

## Example session

```text
crew alice vessel-1 1 BST 2000000000
crew bob vessel-1 1 BST 2000000000
station muster 2 BST
start drill-1 vessel-1
check-in alice muster 1800000000
status
check-in bob muster 1800000000
status
finish
audit
quit
```

After the first check-in, `status` prints:

```text
active
muster 1/2 shortage=1
```

After the second check-in, it prints `muster 2/2 shortage=0`. The duration printed by `finish` depends on elapsed time.

Use `help` to print the command syntax. Blank lines are ignored. Unexpected arguments are rejected before the command runs. Errors go to stderr; the CLI continues processing input and exits with status 1 if any command failed or an input read error occurred. An eligibility denial is a normal decision and does not set the error exit status.

| Command | Purpose |
| --- | --- |
| `crew ID VESSEL ENABLED [CERT EXPIRY]...` | Register or update crew. |
| `station ID MINIMUM [CERT]...` | Configure station requirements. |
| `start ID VESSEL` | Start a vessel drill. |
| `check-in CREW STATION UNIX_SECONDS` | Validate and register attendance. |
| `status` | Print headcounts and shortages. |
| `finish` | Finish the drill and report elapsed time. |
| `audit` | Display sequential audit events. |
| `help` | Show command usage. |
| `quit` | Close the program. |

Configuration is locked during active drills. Certificate expiry values must be greater than the non-negative check-in timestamp.

## Project structure

```text
include/operations.hpp     Domain model and audit repository contract
src/main.cpp               Command-line application
tests/test.cpp             Domain regression suite
tests/cli_test.py          CLI regression suite
docs/persistence.md        Persistence design notes
evidence/demo.mp4          Original demonstration
evidence/preview.jpg       Video cover image
.github/workflows/ci.yml   Native CI checks
Makefile                   Build and tests
```

## Architecture

The operations core is separate from the terminal interface. It stores crew, station rules and the active drill in memory. `Repository` defines audit event storage; `MemoryRepository` is its default adapter. Audit events are appended before operational state changes. An adapter must append completely or throw without side effects. This implementation does not provide transactional atomicity between audit storage and in-memory state: a later allocation failure can leave an event without a completed state change.

This is an in-memory implementation. It does not restore state across application restarts, connect to a maritime authority or verify externally issued credentials. For durable storage requirements, see [persistence design](docs/persistence.md).

## Credits

Developed by **Brunno** — [brunnodev.store](https://brunnodev.store).

Released under the [MIT License](LICENSE).
