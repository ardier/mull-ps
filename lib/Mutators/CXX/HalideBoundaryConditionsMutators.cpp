#include "mull/Mutators/CXX/HalideBoundaryConditionsMutators.h"

using namespace mull;
using namespace mull::cxx;

/// The BoundaryConditions swaps are performed on the Clang AST by
/// mull-cxx-frontend, not on LLVM IR, so there is no irm::IRMutation behind
/// them. An empty low-level mutator list makes TrivialCXXMutator::getMutations
/// return nothing, which is exactly right: the IR route must not claim these
/// mutation points.
static std::vector<std::unique_ptr<irm::IRMutation>> noIRMutations() {
  return {};
}

std::string HalideRepeatEdgeToRepeatImage::ID() {
  return "Halide_repeat_edge_to_repeat_image";
}

HalideRepeatEdgeToRepeatImage::HalideRepeatEdgeToRepeatImage()
    : TrivialCXXMutator(noIRMutations(),
                        MutatorKind::Halide_BC_RepeatEdgeToRepeatImage,
                        HalideRepeatEdgeToRepeatImage::ID(),
                        "Replaces Halide::BoundaryConditions::repeat_edge with repeat_image",
                        "repeat_image",
                        "Replaced Halide boundary condition repeat_edge with repeat_image") {}

std::string HalideRepeatEdgeToMirrorImage::ID() {
  return "Halide_repeat_edge_to_mirror_image";
}

HalideRepeatEdgeToMirrorImage::HalideRepeatEdgeToMirrorImage()
    : TrivialCXXMutator(noIRMutations(),
                        MutatorKind::Halide_BC_RepeatEdgeToMirrorImage,
                        HalideRepeatEdgeToMirrorImage::ID(),
                        "Replaces Halide::BoundaryConditions::repeat_edge with mirror_image",
                        "mirror_image",
                        "Replaced Halide boundary condition repeat_edge with mirror_image") {}

std::string HalideRepeatEdgeToMirrorInterior::ID() {
  return "Halide_repeat_edge_to_mirror_interior";
}

HalideRepeatEdgeToMirrorInterior::HalideRepeatEdgeToMirrorInterior()
    : TrivialCXXMutator(noIRMutations(),
                        MutatorKind::Halide_BC_RepeatEdgeToMirrorInterior,
                        HalideRepeatEdgeToMirrorInterior::ID(),
                        "Replaces Halide::BoundaryConditions::repeat_edge with mirror_interior",
                        "mirror_interior",
                        "Replaced Halide boundary condition repeat_edge with mirror_interior") {}

std::string HalideRepeatImageToRepeatEdge::ID() {
  return "Halide_repeat_image_to_repeat_edge";
}

HalideRepeatImageToRepeatEdge::HalideRepeatImageToRepeatEdge()
    : TrivialCXXMutator(noIRMutations(),
                        MutatorKind::Halide_BC_RepeatImageToRepeatEdge,
                        HalideRepeatImageToRepeatEdge::ID(),
                        "Replaces Halide::BoundaryConditions::repeat_image with repeat_edge",
                        "repeat_edge",
                        "Replaced Halide boundary condition repeat_image with repeat_edge") {}

std::string HalideRepeatImageToMirrorImage::ID() {
  return "Halide_repeat_image_to_mirror_image";
}

HalideRepeatImageToMirrorImage::HalideRepeatImageToMirrorImage()
    : TrivialCXXMutator(noIRMutations(),
                        MutatorKind::Halide_BC_RepeatImageToMirrorImage,
                        HalideRepeatImageToMirrorImage::ID(),
                        "Replaces Halide::BoundaryConditions::repeat_image with mirror_image",
                        "mirror_image",
                        "Replaced Halide boundary condition repeat_image with mirror_image") {}

std::string HalideRepeatImageToMirrorInterior::ID() {
  return "Halide_repeat_image_to_mirror_interior";
}

HalideRepeatImageToMirrorInterior::HalideRepeatImageToMirrorInterior()
    : TrivialCXXMutator(noIRMutations(),
                        MutatorKind::Halide_BC_RepeatImageToMirrorInterior,
                        HalideRepeatImageToMirrorInterior::ID(),
                        "Replaces Halide::BoundaryConditions::repeat_image with mirror_interior",
                        "mirror_interior",
                        "Replaced Halide boundary condition repeat_image with mirror_interior") {}

std::string HalideMirrorImageToRepeatEdge::ID() {
  return "Halide_mirror_image_to_repeat_edge";
}

HalideMirrorImageToRepeatEdge::HalideMirrorImageToRepeatEdge()
    : TrivialCXXMutator(noIRMutations(),
                        MutatorKind::Halide_BC_MirrorImageToRepeatEdge,
                        HalideMirrorImageToRepeatEdge::ID(),
                        "Replaces Halide::BoundaryConditions::mirror_image with repeat_edge",
                        "repeat_edge",
                        "Replaced Halide boundary condition mirror_image with repeat_edge") {}

std::string HalideMirrorImageToRepeatImage::ID() {
  return "Halide_mirror_image_to_repeat_image";
}

HalideMirrorImageToRepeatImage::HalideMirrorImageToRepeatImage()
    : TrivialCXXMutator(noIRMutations(),
                        MutatorKind::Halide_BC_MirrorImageToRepeatImage,
                        HalideMirrorImageToRepeatImage::ID(),
                        "Replaces Halide::BoundaryConditions::mirror_image with repeat_image",
                        "repeat_image",
                        "Replaced Halide boundary condition mirror_image with repeat_image") {}

std::string HalideMirrorImageToMirrorInterior::ID() {
  return "Halide_mirror_image_to_mirror_interior";
}

HalideMirrorImageToMirrorInterior::HalideMirrorImageToMirrorInterior()
    : TrivialCXXMutator(noIRMutations(),
                        MutatorKind::Halide_BC_MirrorImageToMirrorInterior,
                        HalideMirrorImageToMirrorInterior::ID(),
                        "Replaces Halide::BoundaryConditions::mirror_image with mirror_interior",
                        "mirror_interior",
                        "Replaced Halide boundary condition mirror_image with mirror_interior") {}

std::string HalideMirrorInteriorToRepeatEdge::ID() {
  return "Halide_mirror_interior_to_repeat_edge";
}

HalideMirrorInteriorToRepeatEdge::HalideMirrorInteriorToRepeatEdge()
    : TrivialCXXMutator(noIRMutations(),
                        MutatorKind::Halide_BC_MirrorInteriorToRepeatEdge,
                        HalideMirrorInteriorToRepeatEdge::ID(),
                        "Replaces Halide::BoundaryConditions::mirror_interior with repeat_edge",
                        "repeat_edge",
                        "Replaced Halide boundary condition mirror_interior with repeat_edge") {}

std::string HalideMirrorInteriorToRepeatImage::ID() {
  return "Halide_mirror_interior_to_repeat_image";
}

HalideMirrorInteriorToRepeatImage::HalideMirrorInteriorToRepeatImage()
    : TrivialCXXMutator(noIRMutations(),
                        MutatorKind::Halide_BC_MirrorInteriorToRepeatImage,
                        HalideMirrorInteriorToRepeatImage::ID(),
                        "Replaces Halide::BoundaryConditions::mirror_interior with repeat_image",
                        "repeat_image",
                        "Replaced Halide boundary condition mirror_interior with repeat_image") {}

std::string HalideMirrorInteriorToMirrorImage::ID() {
  return "Halide_mirror_interior_to_mirror_image";
}

HalideMirrorInteriorToMirrorImage::HalideMirrorInteriorToMirrorImage()
    : TrivialCXXMutator(noIRMutations(),
                        MutatorKind::Halide_BC_MirrorInteriorToMirrorImage,
                        HalideMirrorInteriorToMirrorImage::ID(),
                        "Replaces Halide::BoundaryConditions::mirror_interior with mirror_image",
                        "mirror_image",
                        "Replaced Halide boundary condition mirror_interior with mirror_image") {}

