# Plugins

Replaceable camera, vision, AI, PLC, robot, and storage implementations belong here.

Vendor SDK types must not cross the plugin boundary.

Camera, AI backend, and performance-sensitive node plugins use the versioned native C
ABI. Managed plugins are reserved for non-hot-path authoring, reporting, exporter, and
business integrations.
