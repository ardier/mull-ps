#include "ASTInstrumentation.h"
#include "ASTMutationsSearchVisitor.h"
#include "ASTNodeFactory.h"
#include "MullASTMutator.h"
#include "MutationMap.h"
#include "rust/mull-cxx-bridge/bridge.rs.h"

#include <clang/AST/AST.h>
#include <clang/AST/ASTConsumer.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/FrontendPluginRegistry.h>
#include <clang/Sema/Sema.h>
#include <clang/Sema/SemaConsumer.h>
#include <llvm/Support/raw_ostream.h>

using namespace clang;
using namespace llvm;

namespace mull {
namespace cxx {

class MullASTConsumer : public ASTConsumer {
  CompilerInstance &instance;
  std::unique_ptr<MullASTMutator> astMutator;
  MutationMap mutationMap;

public:
  MullASTConsumer(CompilerInstance &instance, const MutationMap mutationMap)
      : instance(instance), astMutator(nullptr), mutationMap(mutationMap) {}

  void Initialize(ASTContext &Context) override {
    ASTConsumer::Initialize(Context);
  }

  /// This function can be considered a main() function of the
  /// mull-cxx-frontend plugin. This method is called multiple times by
  /// clang::ParseAST() for each declaration when it's finished being parsed.
  /// For each found function declaration below, a two-pass approach is used:
  /// 1) First all mutation points are found in the function declaration by the
  /// recursive AST visitor class ASTMutationsSearchVisitor.
  /// 2) For each mutation point, the mutations are performed on the Clang AST
  /// level. The mutation is performed by the higher-level MullASTMutator class
  /// which class to the lower-level ClangASTMutator class.
  bool HandleTopLevelDecl(DeclGroupRef DG) override {
    /// Could be a better place to create this. But at Initialize(), getSema()
    /// hits an internal assert because it is not initialized yet at that time.
    if (!astMutator) {
      astMutator = std::make_unique<MullASTMutator>(instance.getASTContext(), instance.getSema());
      astMutator->instrumentTranslationUnit();
    }

    for (DeclGroupRef::iterator I = DG.begin(), E = DG.end(); I != E; ++I) {
      if ((*I)->getKind() == Decl::Function) {
        mutateFunction(static_cast<FunctionDecl *>(*I));
        continue;
      }
      /// Halide generators put all of their code in the generate() method of a
      /// class, usually inside an anonymous namespace, so none of it is ever a
      /// top-level function declaration. Descending into namespaces and class
      /// bodies is what makes those bodies reachable at all. It is gated on an
      /// opt-in mutator being enabled so that the mutators that have only ever
      /// been applied to top-level functions keep behaving exactly as before.
      if (mutationMap.needsDeepDeclTraversal()) {
        mutateNestedFunctions(*I);
      }
    }

    return true;
  }

private:
  /// Recurses through the declaration contexts that can lexically contain a
  /// function definition, mutating every function body found on the way.
  void mutateNestedFunctions(Decl *decl) {
    if (FunctionDecl *f = dyn_cast<FunctionDecl>(decl)) {
      /// Only definitions written in this source have anything to mutate;
      /// template instantiations share their pattern's source locations and
      /// would produce duplicate mutation points.
      if (f->doesThisDeclarationHaveABody() && !f->isTemplateInstantiation()) {
        mutateFunction(f);
      }
      return;
    }
    if (!isa<NamespaceDecl>(decl) && !isa<CXXRecordDecl>(decl) && !isa<LinkageSpecDecl>(decl)) {
      return;
    }
    auto *declContext = dyn_cast<DeclContext>(decl);
    if (declContext == nullptr) {
      return;
    }
    for (Decl *nested : declContext->decls()) {
      mutateNestedFunctions(nested);
    }
  }

  void mutateFunction(FunctionDecl *f) {
    if (f->getDeclName().getAsString() == "main") {
      return;
    }

    clang::SourceLocation functionLocation = f->getLocation();
    if (instance.getSourceManager().isInSystemHeader(functionLocation)) {
      return;
    }
    std::string sourceFilePath = instance.getSourceManager().getFilename(functionLocation).str();
    if (sourceFilePath.find("include/gtest") != std::string::npos) {
      return;
    }
    ASTMutationsSearchVisitor visitor(instance.getASTContext(), mutationMap, f);
    errs() << "HandleTopLevelDecl: Looking at function: " << f->getDeclName() << "\n";
    visitor.TraverseFunctionDecl(f);

    for (auto &foundMutation : visitor.getAstMutations()) {
      foundMutation->performMutation(*astMutator);
    }
  }

public:

  // This method is the last to be called when all declarations have already
  // been called on with HandleTopLevelDecl(). At this point, it is possible to
  // visualize the final mutated AST tree.
  void HandleTranslationUnit(ASTContext &context) override {
    // The following is useful for debugging mutations:
    // context.getTranslationUnitDecl()->print(llvm::errs(), 2);
    // context.getTranslationUnitDecl()->dump();
    // exit(1);
  }
};

class MullAction : public PluginASTAction {
  MutationMap mutationMap;

protected:
  std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &CI, llvm::StringRef) override {
    return std::make_unique<MullASTConsumer>(CI, mutationMap);
  }

  bool ParseArgs(const CompilerInstance &CI, const std::vector<std::string> &args) override {
    auto core = init_core_ffi(DiagOutput::Stdout);
    const auto &config = core->config();
    /// config.mutators is the raw list from mull.yml / the command line, so it
    /// can name groups ("cxx_default", "halide_boundary_conditions") as well as
    /// individual mutators. Expand through the same Rust implementation the IR
    /// frontend and mull-runner use, instead of duplicating the group table
    /// here; MutationMap then ignores whatever names it does not implement.
    rust::Vec<rust::String> requested;
    for (const auto &mutator : config.mutators) {
      requested.push_back(mutator);
    }
    for (const auto &mutator : expand_mutator_groups(std::move(requested))) {
      mutationMap.addMutation(std::string(mutator));
    }
    mutationMap.setDefaultMutationsIfNotSpecified();
    return true;
  }

  PluginASTAction::ActionType getActionType() override {
    /// Note: AddBeforeMainAction is the only option when mutations have effect.
    return AddBeforeMainAction;
  }
};

} // namespace cxx
} // namespace mull

static FrontendPluginRegistry::Add<mull::cxx::MullAction> X("mull-cxx-frontend",
                                                            "Mull: Prepare mutations");
