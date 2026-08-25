# Tests

NVision tests are organized by verification purpose:

```text
unit
integration
architecture
compatibility
golden
hardware
performance
system
```

P0 establishes architecture tests. Additional suites are created with the corresponding implementation capability.

Native C++ tests are built by CMake and executed by CTest. xUnit remains responsible for
managed product behavior, ABI/P/Invoke integration, gRPC contracts, and end-to-end parity.

N1 adds direct native tests for Frame validation/ownership, File Camera replay/lifecycle,
timeout/cancellation, and missing sources, plus C ABI/.NET integration coverage for
camera and frame handle ownership.
