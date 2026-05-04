#include "compiled_rule.hh"

#include <ghidra/funcdata.hh>

#include <unordered_map>
#include <unordered_set>

class PcodeWeaverRule {
  pcodeweaver::compiled::Rule compiled;
  std::unordered_map<pcodeweaver::compiled::StepId, ghidra::PcodeOp *>
      patternPnodes;
  std::unordered_map<pcodeweaver::compiled::StepId, ghidra::Varnode *>
      patternVarnodes;
  std::unordered_set<pcodeweaver::compiled::StepId> patternEmptySteps;

  // Check pattern for target PcodeOp as root node
  ghidra::int4 applyPatternToPnode(ghidra::Funcdata &data, ghidra::PcodeOp *op);

  // Checks whether a suitable location for the pattern can be found within
  // this function, and if so, stores auxiliary data for the subsequent steps.
  ghidra::int4 applyPattern(ghidra::Funcdata &data);

  ghidra::int4 apply(ghidra::Funcdata &data);
};
