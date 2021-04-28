// Copyright (c) 2021, PickNik, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
/// \author: Andy Zelenak

#include "admittance_controller/incremental_ik_calculator.hpp"

namespace admittance_controller
{
IncrementalIKCalculator::IncrementalIKCalculator(std::shared_ptr<rclcpp::Node>& node) : node_(node)
{
  // TODO(andyz): Parameterize robot description and joint group
  std::unique_ptr<robot_model_loader::RobotModelLoader> model_loader_ptr =
      std::unique_ptr<robot_model_loader::RobotModelLoader>(new robot_model_loader::RobotModelLoader(node_, "/robot_description", false /* do not load kinematics plugins */));
  const moveit::core::RobotModelPtr& kinematic_model = model_loader_ptr->getModel();
  // TODO(andyz): joint_model_group_ is a raw pointer. Is it thread safe?
  joint_model_group_ = kinematic_model->getJointModelGroup("gemini");
  kinematic_state_ = std::make_shared<moveit::core::RobotState>(kinematic_model);

  // By default, the MoveIt Jacobian frame is the last link
  // TODO(andyz): parameterize this
  moveit_jacobian_frame_ = "end_effector";
}

bool IncrementalIKCalculator::convertCartesianDeltasToJointDeltas(const Eigen::VectorXd delta_x, std::string& delta_x_frame, Eigen::VectorXd& delta_theta)
{
  // Transform delta_x to the moveit_jacobian_frame

  // Multiply with the pseudoinverse to get delta_theta
  jacobian_ = kinematic_state_->getJacobian(joint_model_group_);
  svd_ = Eigen::JacobiSVD<Eigen::MatrixXd>(jacobian_, Eigen::ComputeThinU | Eigen::ComputeThinV);
  matrix_s_ = svd_.singularValues().asDiagonal();
  pseudo_inverse_ = svd_.matrixV() * matrix_s_.inverse() * svd_.matrixU().transpose();
  delta_theta = pseudo_inverse_ * delta_x;

  return true;
}
}  // namespace admittance_controller
