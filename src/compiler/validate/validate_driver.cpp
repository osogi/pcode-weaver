// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: © 2026 Efremov Alexey <4osogi@gmail.com>

#include "validate/validate_driver.hh"
Errorable<void> ValidateDriver::validate(const ast::Rule &rule) {

  auto resPatGraphBuild = pgraph.addPatterns(rule.patterns);
  if (!resPatGraphBuild.has_value()) {
    return err("Build Pattern Graph: " + resPatGraphBuild.error().message());
  }
  pgraph.updateMaxArgForPnodes(false);
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
      inferencer.inferenceUserConds(pgraph, &userRuntimeConditions);
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
  getActgraph().updateMaxArgForPnodes(true);

  auto resActGraphValidate = getActgraph().validate();
  if (!resActGraphValidate.has_value()) {
    return err(
        "Validate Action Graph: " + resActGraphValidate.error().message()
    );
  }

  auto resActGraphInference =
      inferencer.inference(getActgraph(), &requiredConditions);
  getActgraph().removeConditionsWithNewNodes(requiredConditions);
  if (!resActGraphInference.has_value()) {
    return err(
        "Inference Action Graph: " + resActGraphInference.error().message()
    );
  }

  runtimeValueRequirements = RuntimeValueRequirements::fromActionGraph(
      pgraph, getActgraph(), sizesolver, bbsolver
  );

  return {};
};

const std::vector<speccond::SpecCondition> &ValidateDriver::getURTC() const {
  return userRuntimeConditions;
};
const std::vector<speccond::SpecCondition> &ValidateDriver::getRQC() const {
  return requiredConditions;
};

std::vector<speccond::SpecCondition>
ValidateDriver::getRuntimeCheckConditions() const {
  std::vector<speccond::SpecCondition> checkConditions(userRuntimeConditions);
  checkConditions.reserve(
      userRuntimeConditions.size() + requiredConditions.size()
  );
  checkConditions.insert(
      checkConditions.end(),
      requiredConditions.begin(),
      requiredConditions.end()
  );
  return checkConditions;
}

const graph::PcodeGraph &ValidateDriver::getPGraph() const { return pgraph; }

const graph::ActionPcodeGraph &ValidateDriver::getActGraph() const {
  return actgraph.value();
}

const solvers::SizeSolver &ValidateDriver::getSizeSolver() const {
  return sizesolver;
}

const RuntimeValueRequirements &
ValidateDriver::getRuntimeValueRequirements() const {
  return runtimeValueRequirements;
}
