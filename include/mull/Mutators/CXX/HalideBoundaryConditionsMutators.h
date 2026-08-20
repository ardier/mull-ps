#pragma once

#include "mull/Mutators/CXX/TrivialCXXMutator.h"

namespace mull {
namespace cxx {

/// Halide's BoundaryConditions family swap.
///
/// Halide::BoundaryConditions::{repeat_edge, repeat_image, mirror_image,
/// mirror_interior} all share the same overload set --
/// (const Func &, const Region &), a function template on any Func-like input
/// with explicit bounds, and a function template on any Func-like input that
/// derives the bounds from the input -- so any one of them can be swapped for
/// any other at a call site while keeping the arguments untouched.
/// constant_exterior is deliberately excluded: it takes an extra value
/// argument and is therefore not arity-compatible.
///
/// These mutators are produced exclusively by mull-cxx-frontend (the Clang AST
/// route). The idiomatic entry points are function templates instantiated per
/// buffer type, so there is no stable mangled name for the IR route to swap;
/// see docs/halide-mangled-names.md. The classes below therefore carry no
/// low-level IR mutation and exist so that the mutator identifiers, the
/// halide_boundary_conditions group and the reporters all resolve.

class HalideRepeatEdgeToRepeatImage : public TrivialCXXMutator {
public:
  static std::string ID();
  HalideRepeatEdgeToRepeatImage();
};

class HalideRepeatEdgeToMirrorImage : public TrivialCXXMutator {
public:
  static std::string ID();
  HalideRepeatEdgeToMirrorImage();
};

class HalideRepeatEdgeToMirrorInterior : public TrivialCXXMutator {
public:
  static std::string ID();
  HalideRepeatEdgeToMirrorInterior();
};

class HalideRepeatImageToRepeatEdge : public TrivialCXXMutator {
public:
  static std::string ID();
  HalideRepeatImageToRepeatEdge();
};

class HalideRepeatImageToMirrorImage : public TrivialCXXMutator {
public:
  static std::string ID();
  HalideRepeatImageToMirrorImage();
};

class HalideRepeatImageToMirrorInterior : public TrivialCXXMutator {
public:
  static std::string ID();
  HalideRepeatImageToMirrorInterior();
};

class HalideMirrorImageToRepeatEdge : public TrivialCXXMutator {
public:
  static std::string ID();
  HalideMirrorImageToRepeatEdge();
};

class HalideMirrorImageToRepeatImage : public TrivialCXXMutator {
public:
  static std::string ID();
  HalideMirrorImageToRepeatImage();
};

class HalideMirrorImageToMirrorInterior : public TrivialCXXMutator {
public:
  static std::string ID();
  HalideMirrorImageToMirrorInterior();
};

class HalideMirrorInteriorToRepeatEdge : public TrivialCXXMutator {
public:
  static std::string ID();
  HalideMirrorInteriorToRepeatEdge();
};

class HalideMirrorInteriorToRepeatImage : public TrivialCXXMutator {
public:
  static std::string ID();
  HalideMirrorInteriorToRepeatImage();
};

class HalideMirrorInteriorToMirrorImage : public TrivialCXXMutator {
public:
  static std::string ID();
  HalideMirrorInteriorToMirrorImage();
};

} // namespace cxx
} // namespace mull
