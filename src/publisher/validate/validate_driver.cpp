#include "validate/validate_driver.hh"

Errorable<void> ValidateDriver::validate(const ast::Rule &rule) {

  auto resPatGraphBuild = pgraph.addPatterns(rule.patterns);
  if (!resPatGraphBuild.has_value()) {
    return err("Build Pattern Graph: " + resPatGraphBuild.error().message());
  }

  auto resPatGraphValidate = pgraph.validate();
  if (!resPatGraphValidate.has_value()) {
    return err(
        "Validate Pattern Graph: " + resPatGraphValidate.error().message()
    );
  }

  auto resPatGraphInference = inferencer.inference();
  if (!resPatGraphInference.has_value()) {
    return err(
        "Inference Pattern Graph: " + resPatGraphInference.error().message()
    );
  }

  auto resPatGraphInferenceUserConds =
      inferencer.inferenceUserConds(&runtimeConditions);
  if (!resPatGraphInferenceUserConds.has_value()) {
    return err(
        "Inference User Conditions Pattern Graph: " +
        resPatGraphInferenceUserConds.error().message()
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
