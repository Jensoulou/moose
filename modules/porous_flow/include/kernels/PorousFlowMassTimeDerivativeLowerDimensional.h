//* This file is part of the MOOSE framework
//* https://mooseframework.inl.gov
//*
//* All rights reserved, see COPYRIGHT for full restrictions
//* https://github.com/idaholab/moose/blob/master/COPYRIGHT
//*
//* Licensed under LGPL 2.1, please see LICENSE for details
//* https://www.gnu.org/licenses/lgpl-2.1.html

#pragma once

#include "TimeDerivative.h"
#include "PorousFlowDictator.h"
#include "PorousFlowMassTimeDerivative.h"

/**
 * Multiplication of PorousFlowMassTimeDerivative by the _aperture of the fracture
 */
class PorousFlowMassTimeDerivativeLowerDimensional : public PorousFlowMassTimeDerivative
{
public:
  static InputParameters validParams();

  PorousFlowMassTimeDerivativeLowerDimensional(const InputParameters & parameters);

protected:
  virtual Real computeQpResidual() override;
  virtual Real computeQpJacobian() override;
  virtual Real computeQpOffDiagJacobian(unsigned int jvar) override;

  /// Fracture aperture (width)
  const VariableValue & _aperture;
};
