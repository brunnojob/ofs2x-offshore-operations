# OFS2X Offshore Operations

C++20 operations core based on the offshore fleet, crew and drill workflows shown in `ofs2x.mp4`.

## Capabilities

- Crew registration and vessel assignment.
- Station requirements with certificate expiry checks.
- Timed drills using a monotonic clock.
- Idempotent attendance and prevention of simultaneous station assignment.
- Minimum staffing deficiencies and ordered audit events.
- Repository interface for future persistence.

## Build and run

Linux, GCC 12 or newer, GNU Make.

```sh
make all test
./build/offshore
```

The terminal accepts these commands:

```text
crew alice vessel-1 1 BST 2000000000
crew bob vessel-1 1 BST 2000000000
station muster 2 BST
start drill-1 vessel-1
check-in alice muster 1800000000
status
check-in bob muster 1800000000
finish
audit
quit
```

Certificate expiry and eligibility timestamps use Unix seconds. Certificates are supplied by the operator; this library does not issue or validate documents with external authorities. The application starts empty and keeps state in memory.

## Architecture

`include/operations.hpp` contains the domain and repository contract. `src/main.cpp` exposes a terminal interface. `tests/test.cpp` covers expired certificates, vessel isolation, duplicate attendance, station conflicts and drill timing.

Future persistence requirements: [docs/persistence.md](docs/persistence.md).

## Video evidence

[![Offshore operations reference](evidence/preview.jpg)](evidence/demo.mp4)

[Open the original demonstration](evidence/demo.mp4)

The video demonstrates the author's reference system. This repository is a new native implementation of the observed workflows, rather than the original dashboard source. The recording is not a test run of this implementation. Original filename and SHA-256 are recorded in [evidence/provenance.json](evidence/provenance.json).

## Credits and license

Created for [brunnodev.store](https://brunnodev.store/). MIT: [LICENSE](LICENSE).
