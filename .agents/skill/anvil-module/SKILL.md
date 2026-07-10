---
name: anvil-module
description: Use when designing, implementing or reviewing the Anvil platform layer, including windowing, events and input.
---

# Anvil

Anvil is the platform abstraction layer of Forge Engine.

Its purpose is to isolate operating system details behind a small, explicit and platform-neutral API.

Anvil should remain completely independent from Furnace and every higher-level engine module.

---

# Design Goals

Every design decision should move complexity toward backend implementations while keeping the public API:

- Small
- Explicit
- Stable
- Predictable
- Platform-neutral

Prefer exposing capabilities rather than implementation details.

Mechanism belongs in Anvil.

Policy belongs in higher-level modules.

---

# Responsibilities

Anvil owns platform services, including:

- Window management (X11, Wayland on Linux; Win32 on Windows)
- Input
- Event collection
- Logging

Planned responsibilities (not yet implemented):

- Filesystem
- Timing
- Threading
- Synchronization
- Dynamic library loading
- Display and monitor information

Anvil does **not** own:

- Rendering
- Graphics APIs
- Asset management
- Scene management
- ECS
- Gameplay systems
- Application logic

When implementing a new feature, first determine whether it is truly a platform concern.

If another module naturally owns the responsibility, do not expand Anvil.

---

# Public API

Public APIs should:

- Express platform capabilities.
- Hide implementation details.
- Minimize exposed types.
- Be easy to understand.
- Remain stable over time.

Avoid:

- God objects
- Generic utility APIs
- Leaking backend implementation details
- Platform-specific behavior in public interfaces

---

# Cross-Platform Design

Design APIs around concepts shared by every supported platform.

Do not expose platform-specific capabilities unless a stable cross-platform abstraction has been identified.

Platform differences should remain implementation details whenever possible.

Adding support for a new platform should primarily require implementing a new backend rather than modifying existing ones.

If supporting a new platform requires changing unrelated backends, reconsider the abstraction.

---

# Backend Design

Backends should provide equivalent behavior through a common public API.

Platform-specific optimizations are encouraged as long as they do not alter observable public behavior.

Keep backend code isolated from engine modules.

---

# Ownership & Lifetime

Ownership must always be explicit.

Every resource should have:

- a clear creator;
- a clear owner;
- a clear destroyer.

Resource lifetime should remain predictable and independent whenever possible.

Avoid hidden ownership transfers.

---

# Error Handling

Translate platform failures into engine-level errors.

Avoid exposing backend-specific error codes or implementation details through the public API.

Error handling should be explicit and predictable.

---

# Public Types

Prefer opaque types for public resources.

Opaque types provide:

- Type safety
- Encapsulation
- ABI stability

Example:

```c
typedef struct AnvilWindow AnvilWindow;

AnvilWindow* anvl_window_create(...);
```

Avoid exposing implementation structs, integer handles or `void*` as public resource types unless there is a compelling reason.

---

# Decision Checklist

Before introducing a new API, ask:

- Does this belong in Anvil?
- Is this a platform service or a higher-level policy?
- Can this remain backend-specific?
- Does another module already own this responsibility?
- Will this API remain stable across supported platforms?
- Does this reduce or increase coupling?

If the answer is unclear, prefer the simpler design.

---

# Code Review Checklist

Always verify:

- Platform isolation
- Module boundaries
- Public API consistency
- Explicit ownership
- Resource lifetime
- Error handling
- Thread safety
- Backend consistency
- Cross-platform behavior
- Hidden platform leaks

---

# Preferred Solutions

Prefer:

- Explicit APIs
- Composition
- Translation-unit encapsulation
- Opaque public types
- Backend-local implementations
- Clear ownership
- Small modules

Avoid:

- Global mutable state
- Hidden allocations
- Platform-specific logic in public code
- Premature abstraction
- Duplicated backend logic
- APIs that combine unrelated responsibilities

---

# Agent Behavior

When working on Anvil:

- Preserve module boundaries.
- Prefer evolving existing abstractions over creating new ones.
- Justify architectural changes before implementation.
- Consider portability from the beginning.
- Keep the public API simpler than the backend implementation.

The primary goal of Anvil is not to expose every platform feature, but to provide a clean, stable and maintainable foundation for the rest of the engine.
