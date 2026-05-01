#include "validate/validate_driver.hh"
Errorable<void> ValidateDriver::validate(const ast::Rule &rule) {

  auto resPatGraphBuild = pgraph.addPatterns(rule.patterns);
  if (!resPatGraphBuild.has_value()) {
    return err("Build Pattern Graph: " + resPatGraphBuild.error().message());
  }
  pgraph.updateMaxArgForPnodes(true);
  pgraph.initGhostNodes(cntx);

  actgraph.emplace(pgraph);

  auto resPatGraphValidate = pgraph.validate();
  if (!resPatGraphValidate.has_value()) {
    return err(
        "Validate Pattern Graph: " + resPatGraphValidate.error().message()
    );
  }

  auto resPatGraphInference = inferencer.inference(pgraph);
  if (!resPatGraphInference.has_value()) {
    return err(
        "Inference Pattern Graph: " + resPatGraphInference.error().message()
    );
  }

  auto resPatGraphInferenceUserConds =
      inferencer.inferenceUserConds(pgraph, &runtimeConditions);
  if (!resPatGraphInferenceUserConds.has_value()) {
    return err(
        "Inference User Conditions Pattern Graph: " +
        resPatGraphInferenceUserConds.error().message()
    );
  }

  // Actions

  auto resActGraphBuild = getActgraph().addActions(rule.actions);
  if (!resActGraphBuild.has_value()) {
    return err(
        "Build Action Graph (Action Step): " +
        resActGraphBuild.error().message()
    );
  }
  getActgraph().updateMaxArgForPnodes(false);

  auto resActGraphValidate = getActgraph().validate();
  if (!resActGraphValidate.has_value()) {
    return err(
        "Validate Action Graph: " + resActGraphValidate.error().message()
    );
  }

  auto resActGraphInference =
      inferencer.inference(getActgraph(), &requiredConditions);
  if (!resActGraphInference.has_value()) {
    return err(
        "Inference Action Graph: " + resActGraphInference.error().message()
    );
  }

  return {};
};

const std::vector<speccond::SpecCondition> &ValidateDriver::getRTC() const {
  return runtimeConditions;
};
const std::vector<speccond::SpecCondition> &ValidateDriver::getRQC() const {
  return requiredConditions;
};
