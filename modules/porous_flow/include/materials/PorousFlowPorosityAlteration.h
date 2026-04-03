//* This file is part of the MOOSE framework
//* https://mooseframework.inl.gov
//*
//* All rights reserved, see COPYRIGHT for full restrictions
//* https://github.com/idaholab/moose/blob/master/COPYRIGHT
//*
//* Licensed under LGPL 2.1, please see LICENSE for details
//* https://www.gnu.org/licenses/lgpl-2.1.html

#pragma once

#include "PorousFlowPorosity.h"

/**
 * Material designed to provide the porosity in PorousFlow simulations
 * chemistry + biot + (phi0 - reference_chemistry - biot) * exp(-vol_strain
 *    + coeff * (effective_pressure - reference_pressure)
 *    + thermal_exp_coeff * (temperature - reference_temperature))
 *    + coeff * Alteration Index value
 * 
 * This class depends on  which already includes
 * dependency on mechanical, thermal, pressure and chemical phenomenom
 */
class PorousFlowPorosityAlteration : public PorousFlowPorosity
{
public:
  static InputParameters validParams();

  PorousFlowPorosityAlteration(const InputParameters & parameters);

protected:
  virtual Real atNegInfinityQp() const override;
  virtual Real datNegInfinityQp(unsigned pvar) const override;
  virtual Real atZeroQp() const override;
  virtual Real datZeroQp(unsigned pvar) const override;
  virtual Real decayQp() const override;
  virtual Real ddecayQp_dvar(unsigned pvar) const override;
  virtual RealGradient ddecayQp_dgradvar(unsigned pvar) const override;

  /// Porosity is a function of alteration index
  const bool _alteration;

  /// Alteration index variable value
  const VariableValue & _ai;

  /// Scaling coefficient to mitigate the alteration on the porosity
  const Real _ai_coef;
};
