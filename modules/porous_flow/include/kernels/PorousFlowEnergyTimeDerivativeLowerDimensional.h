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
#include "PorousFlowEnergyTimeDerivative.h"

/**
 * Kernel = (heat_energy - heat_energy_old)/dt
 * It is lumped to the nodes
 */
class PorousFlowEnergyTimeDerivativeLowerDimensional : public PorousFlowEnergyTimeDerivative
{
public:
  static InputParameters validParams();

  PorousFlowEnergyTimeDerivativeLowerDimensional(const InputParameters & parameters);

protected:
  virtual Real computeQpResidual() override;
  virtual Real computeQpJacobian() override;
  virtual Real computeQpOffDiagJacobian(unsigned int jvar) override;
  
  /// Fracture aperture (width)
  const VariableValue & _aperture;
};
