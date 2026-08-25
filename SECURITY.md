# Security Policy

## Supported version

The latest `main` branch and the newest tagged release receive fixes.

## Reporting a vulnerability

Please do not open a public issue for a vulnerability that could affect users. Contact the repository owner privately with:

- affected version and platform;
- minimal reproduction steps;
- expected and observed behavior;
- likely impact; and
- a suggested fix, if available.

GeoViz does not make network requests at runtime. Its main external supply-chain surface is the pinned raylib source fetched by CMake during configuration. Scene files are bounded to 10,000 points and 10,000 half-planes before allocation.

