# SDK

Public .NET, C++, and Python embedding/control surfaces live here.

The first internal facade is `dotnet/NTech.NVision.Native`, which owns safe P/Invoke over
the versioned native C ABI. It is not yet a stable external SDK. Internal C++ Engine APIs
must not be treated as public ABI.
