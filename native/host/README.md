# NVision Native Engine Host

This directory owns the production gRPC boundary around the same `nvision_core` and
`nvision_engine` implementation used by embedded/P/Invoke mode.

The initial checked-in artifact is the versioned protobuf contract. Server generation,
transport selection, supervisor integration, and shared-memory implementation are added
after the C ABI lifecycle and Wire Marker parity slice are stable.

Rules:

- gRPC carries control, diagnostics, and result metadata;
- full-resolution inspection frames use shared-memory descriptors;
- the host never exposes internal C++ or vendor types;
- the .NET Runtime Supervisor owns process restart and known-good recovery policy.
