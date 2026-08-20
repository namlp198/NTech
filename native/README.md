# Native NVision Engine

This directory owns the canonical portable C++20 Core and Engine introduced by
ADR-0005.

```text
core/   Internal native Core/Engine implementation
abi/    Stable versioned C ABI for P/Invoke and embedding
host/   Production Engine Host gRPC contract and implementation
tests/  Native lifetime, ABI, behavior, and performance verification
```

Internal C++ APIs may evolve with the source tree. Only the C ABI and versioned gRPC
contract are binary/process boundaries. Do not export STL types, exceptions, raw vendor
types, or compiler-specific object layouts.
