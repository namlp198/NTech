# Engine

The current managed P0–P2 execution capabilities remain here as a behavior oracle during
the ADR-0005 migration. New canonical Core/Engine execution belongs under `native/`.

Engine projects must remain headless and must not reference Studio or Runtime UI.

Managed persistence, query, application coordination, and product-plane facades may be
extracted from these projects as the native vertical slice reaches parity.
