# Track actor and boundary verification

## Animated boundary and original assets — 2026-09-25

`make verify-animated-boundary` executes original `1fec8..1ff97` for
20,000 consecutive updates and compares level, signed timer, direction,
random seed and every palette write. The race now advances this state before
car motion, including countdown frames. The platform applies pending colours
199..203 through the framework copper list during display blanking.

`make verify-track-actor-assets` decodes the five base images and nine appended
animation frames from `SLICKS.DAT`, and compares every pixel and dimension
against the original `2e51a` loader. All fourteen match. Truncations before
the final image ends are rejected. No extracted asset bytes are committed.

The base image indices are 79, 80, 81, 82 and 89. The appended bank follows
the `12 34 00` marker after the material data; its nine frames belong to
kinds 0, 0, 0, 2, 2, 2, 4, 4 and 4 respectively. The largest logical image
has 110 pixels. These decoded assets are not yet connected to race rendering.

The Amiga build passes. A muted A1200, 2 MiB/no Fast RAM run has passed 4,800
updates with the integrated boundary state; this is a bounded runtime check,
not proof of race completion or the remaining actor integration.
