# Skyrim Together Reborn Enhanced

An immersive cooperative fork of Skyrim Together Reborn.

# Changelog

## [Unreleased]

## Upstream base

Skyrim Together Reborn Enhanced is based on the official TiltedEvolution project.

The exact upstream commit used for each release is documented in `UPSTREAM.md`.

## Project status

Current version: **0.1.0-alpha.1**

This is an experimental alpha release. Features and compatibility may change.

### Added

- Player-to-player item trading
- Native 3D preview for traded items
- Automatic preview framing and raster-based refinement
- Modular preview architecture
- Initial project documentation

### Changed

- Refactored the item preview subsystem into dedicated components
- Decoupled trade preview hosting from the trade service

### Known limitations

- The trading system is still experimental
- Stack splitting is not yet finalized
- Gold trading is not yet available
- Transaction hardening and edge-case handling remain in development

## Vision

STRE aims to transform Skyrim Together into a richer cooperative RPG experience while remaining faithful to the original game.

Rather than turning Skyrim into an MMO, STRE focuses on:

- immersive cooperative gameplay;
- meaningful player interactions;
- improved trading;
- downed-state mechanics;
- long-term progression;
- maintainable architecture.

## Features

### Available

- Player-to-player item trading
- Native 3D preview of offered items
- Automatic item framing and refinement
- Integrated trade interface
- Refactored and modular preview subsystem

### In development

- Transaction hardening and edge-case handling
- Stack splitting
- Gold trading
- Improved trade confirmation workflow
- Downed-state and cooperative recovery system

## Build

STRE uses the same build process as the official Skyrim Together Reborn project.

Refer to the official build documentation.

## Documentation

- Architecture
- Roadmap
- Changelog
- ADR

## Credits

Based on Skyrim Together Reborn by The Tilted Phoques Team.

STRE is an independent community fork.

## License

GPL v3 (same as upstream).