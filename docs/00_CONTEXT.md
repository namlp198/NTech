# 00_CONTEXT

## Project

NTech NVision — Industrial Vision Intelligence Platform

## Purpose

Build a reusable industrial Machine Vision platform for authoring, publishing, executing, and operating automated visual inspections with traditional vision, AI, and industrial automation.

## Current State

- Working: P0 repository foundation, the complete P1 Workflow engine, and the P2 Wire Marker headless inspection slice.
- Working P2 path: FrameLease ownership → File Camera → AcquisitionService → compiled Crop/Classify/Compare/Decision Workflow → ONNX Runtime CPU → immutable InspectionResult → file image evidence + SQLite.
- Blocked: Nothing in the current P2 slice.
- Not started: ProjectFormat/Studio authoring, Runtime Service, PLC integration, real camera plugins, and production-trained inspection models.

## Architecture Summary

NVision is a capability-oriented modular monolith. Executable products live in `apps`; reusable headless behavior lives in `engine`; vendor and backend variability lives in `plugins`; durable contracts live in `schemas`. Studio and Runtime are separate applications using the same Engine.

## Important Constraints

- Follow `docs/architecture/14_REPOSITORY_STRUCTURE.md`.
- Domain and Engine cannot depend on WPF or vendor SDKs.
- Technical inspection failures never become valid-looking OK/NG results.
- File Camera, PLC Simulator, and ONNX Runtime precede real hardware.

## Current Development Focus

Prepare P3 ProjectFormat and Studio MVP while preserving the P2 headless execution path as a permanent regression suite.

## Verification Baseline

`scripts/check.ps1` currently completes restore, Release build, 113 tests, and formatting verification.

## Notes for AI

- Read accepted ADRs before changing architecture.
- Do not infer real camera or PLC details before hardware selection.
- Update this file and `handoff/latest.md` when project state changes.
