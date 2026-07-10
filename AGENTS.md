# AGENTS.md

# Forge Engine

This document defines how AI agents should contribute to the Forge Engine repository.

---

# Project Philosophy

Forge Engine is a modular game engine written in C11 with an emphasis on explicit design, low-level control, portability and long-term maintainability.

The project values:

- Explicit behavior over hidden magic.
- Small, composable modules.
- Stable public APIs.
- Low coupling.
- High cohesion.
- Predictable control flow.
- Incremental evolution.
- Simplicity before abstraction.

When multiple valid solutions exist, prefer the one with the lowest long-term architectural complexity.

---

# Architecture

The intended architecture organizes Forge Engine into independent modules with clearly defined responsibilities:

```
Sandbox
    │
    ├──► Furnace
    │        │
    │        ▼
    └──────► Anvil
```

> **Note:** As of the current implementation, `main()` resides within the Anvil project as a ConsoleApp. A separate Sandbox executable is planned for future development.

## Module Responsibilities

### Anvil

Platform abstraction.

Responsible for:

- Window management (X11, Wayland on Linux; Win32 on Windows)
- Input
- Events
- Logging

Planned responsibilities (not yet implemented):

- Filesystem
- Timing
- Threading
- Synchronization
- Dynamic library loading
- Display and monitor information

Anvil never implements rendering logic.

---

### Furnace

Rendering abstraction.

Responsible for:

- Graphics APIs
- Context creation
- Rendering resources
- Pipelines
- Buffers
- Textures
- Shaders

Furnace depends only on Anvil's public API.

---

### Sandbox

Application executable.

Responsible for:

- `main()`
- Engine initialization
- Module orchestration
- Experiments and validation

---

# Architectural Constraints

The following rules should be treated as mandatory.

## Dependencies

- Anvil never depends on Furnace.
- Furnace depends only on Anvil's public API.
- Platform backends remain isolated.
- Public headers must never expose platform-specific implementation details.

### Supported Platforms

Anvil supports the following platforms:

| Platform | Windowing Backend |
|----------|-------------------|
| Windows  | Win32             |
| Linux    | X11, Wayland      |
| macOS    | (detection only)  |

macOS platform detection exists in `platform_detection.h`, but no window backend implementation is provided yet.

## Ownership

Ownership must always be explicit.

Every resource must have a clearly defined creator and destroyer.

Avoid shared ownership unless it is explicitly required.

## APIs

Public APIs should be:

- Small
- Explicit
- Predictable
- Easy to reason about

Avoid APIs that solve multiple unrelated problems.

## Module Boundaries

Each module should have a single primary responsibility.

Move functionality to the appropriate module instead of expanding responsibilities.

---

# Architecture Evolution

Prefer extending existing systems over introducing new ones.

Before proposing a new abstraction, ask:

- Is there more than one concrete implementation?
- Does this reduce coupling?
- Does this improve clarity?
- Does this solve an existing problem rather than an anticipated one?

If not, prefer the simpler solution.

Large architectural rewrites should always be discussed before implementation.

---

# Coding Standards

Use modern C11.

Prefer:

- Composition over inheritance
- Explicit ownership
- Opaque types where appropriate
- Translation-unit encapsulation
- Clear, descriptive names
- Data-oriented design when beneficial

Avoid:

- Hidden allocations
- Global mutable state
- Implicit ownership
- Premature optimization
- Premature generalization

## Naming

- Use `snake_case` for functions and variables.
- Use `UPPER_SNAKE_CASE` for macros and compile-time constants.
- Prefix every public symbol with its owning module (`anvl_`, `frnc_`, etc.).
- Public names should clearly reflect their module responsibility.
- Avoid generic public names such as `create()`, `init()` or `run()` without a module prefix.
- Try to use the pattern: Module prefix + Action Verb + Resource (`anvl_create_window`, `frnc_init_backend`, etc.)

## Logging

- Use the engine logging system instead of direct `printf()` or `fprintf()`.
- Log messages should provide meaningful context.

The current implementation writes to stderr with ANSI color codes and timestamps. Two convenience macro families are available:

- Core macros (`ANVIL_CORE_*`) — for Anvil internal use, auto-prefixes the module name as "ANVIL".
- Client macros (`ANVIL_*/anvl_logger_*`) — for external consumers who pass their own client/module name.

---

# Development Workflow

When making changes:

- Preserve the existing architecture whenever possible.
- Keep public APIs stable.
- Minimize breaking changes.
- Explain architectural trade-offs.
- Keep implementations as simple as possible.

Do not introduce new third-party dependencies without explicit approval.

When adding source files, update the corresponding Premake configuration.

---

# Reading Existing Code

Before proposing architectural changes:

- Read only the files relevant to the current task.
- Prefer targeted searches over reading entire files.
- Understand the current design before suggesting alternatives.
- Follow existing patterns unless there is a strong technical reason not to.
- Do not infer responsibilities solely from filenames.

---

# Decision Making

When proposing changes:

- Prefer improving an existing module over creating a new one.
- Consider long-term maintainability.
- Consider portability.
- Consider API stability.
- Consider module boundaries.
- Explain trade-offs before recommending a solution.

Recommendations should always be proportional to the problem being solved.

---

# Documentation

Architecture decisions should be documented.

When introducing a significant architectural change:

- Explain the motivation.
- Describe the trade-offs.
- Document the impact on existing modules.

If documentation becomes inconsistent with the implementation, update the documentation as part of the change.

Future contributors should understand not only **what** was implemented, but **why** the decision was made.

---

# Code Reviews

When reviewing code, pay particular attention to:

- Correctness
- Ownership
- Lifetime management
- Memory management
- Undefined Behavior
- Error handling
- API consistency
- Module boundaries
- Coupling
- Thread safety (when applicable)

Mention both strengths and weaknesses.

---

# Agent Behavior

Do not assume architectural changes are desirable.

Prefer understanding the current design before proposing alternatives.

When information is insufficient:

1. Ask clarifying questions.
2. State assumptions explicitly.
3. Compare viable alternatives.
4. Justify recommendations.

The objective is to preserve a coherent architecture over the lifetime of the project rather than simply produce working code.
