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
#include "RankTwoTensor.h"
#include "PorousFlowMassVolumetricExpansion.h"

/**
 * Multiplication of PorousFlowMassVolumetricExpansion by the _aperture of the fracture
 */
class PorousFlowMassVolumetricExpansionLowerDimensional : public PorousFlowMassVolumetricExpansion
{
public:
  static InputParameters validParams();

  PorousFlowMassVolumetricExpansionLowerDimensional(const InputParameters & parameters);

protected:
  virtual Real computeQpResidual() override;
  virtual Real computeQpJacobian() override;
  virtual Real computeQpOffDiagJacobian(unsigned int jvar) override;

  /// Fracture aperture (width)
  const VariableValue & _aperture;
  // Strain rate
  const MaterialProperty<Real> & _strain_rate_qp;
  /// d(strain rate)/d(PorousFlow variable)
  const MaterialProperty<std::vector<RealGradient>> & _dstrain_rate_qp_dvar;
};
