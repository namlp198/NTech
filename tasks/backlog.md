# Repository Backlog

| ID | Task | Phase | Priority | Dependency |
|---|---|---|---|---|
| P1-003 | Implement Project and Job | P1 | High | Strong IDs |
| P1-004 | Implement WorkflowDefinition, nodes, ports, and connections | P1 | High | Project and Job |
| P1-005 | Implement Recipe and override resolution | P1 | High | Workflow parameters |
| P1-006 | Implement node registry and validation | P1 | High | WorkflowDefinition |
| P1-007 | Implement immutable ExecutionPlan compiler | P1 | High | Validation |
| P1-008 | Implement sequential Workflow runtime | P1 | High | ExecutionPlan |
| P1-009 | Implement failure, timeout, cancellation, timing, and bounded parallelism | P1 | High | Sequential runtime |
| P2-001 | Implement image ownership and File Camera | P2 | High | P1 exit gate |
| P2-002 | Implement `.nvmodel` and ONNX Runtime classification | P2 | High | Image contracts |
| P2-003 | Implement Wire Marker persisted vertical slice | P2 | High | ONNX classification |
