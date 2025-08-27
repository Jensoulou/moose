//* This file is part of the MOOSE framework
//* https://mooseframework.inl.gov
//*
//* All rights reserved, see COPYRIGHT for full restrictions
//* https://github.com/idaholab/moose/blob/master/COPYRIGHT
//*
//* Licensed under LGPL 2.1, please see LICENSE for details
//* https://www.gnu.org/licenses/lgpl-2.1.html

#pragma once

#include "InterfaceMaterial.h"

/**
 * Interface material calculates a variable's jump value across an interface
 */
template <bool is_ad>
class InterfaceValueMaterial_test_gradientTempl : public InterfaceMaterial
{
public:
  static InputParameters validParams();

  InterfaceValueMaterial_test_gradientTempl(const InputParameters & parameters);

protected:
  virtual void computeQpProperties() override;
  virtual void initQpStatefulProperties() override;

  const std::string _mp_primary_name;
  MaterialProperty<std::vector<RealGradient>> & _mp_primary;
  const MaterialProperty<std::vector<RealGradient>> & _mp_secondary;
};

typedef InterfaceValueMaterial_test_gradientTempl<false> InterfaceValueMaterial_test_gradient;
typedef InterfaceValueMaterial_test_gradientTempl<true> ADInterfaceValueMaterial_test_gradient;
