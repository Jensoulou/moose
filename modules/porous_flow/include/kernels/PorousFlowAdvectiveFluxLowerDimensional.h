//* This file is part of the MOOSE framework
//* https://mooseframework.inl.gov
//*
//* All rights reserved, see COPYRIGHT for full restrictions
//* https://github.com/idaholab/moose/blob/master/COPYRIGHT
//*
//* Licensed under LGPL 2.1, please see LICENSE for details
//* https://www.gnu.org/licenses/lgpl-2.1.html

#pragma once

#include "PorousFlowDarcyBase.h"
#include "PorousFlowAdvectiveFlux.h"

/**
 * Multiplication of PorousFlowAdvectiveFlux by the _aperture of the fracture
 */
class PorousFlowAdvectiveFluxLowerDimensional : public PorousFlowAdvectiveFlux
{
public:
  static InputParameters validParams();

  PorousFlowAdvectiveFluxLowerDimensional(const InputParameters & parameters);

protected:
  virtual Real mobility(unsigned nodenum, unsigned phase) const override;
  virtual Real dmobility(unsigned nodenum, unsigned phase, unsigned pvar) const override;

  /// Fracture aperture (width)
  const VariableValue & _aperture;
};
