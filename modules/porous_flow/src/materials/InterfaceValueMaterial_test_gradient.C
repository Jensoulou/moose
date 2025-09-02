//* This file is part of the MOOSE framework
//* https://mooseframework.inl.gov
//*
//* All rights reserved, see COPYRIGHT for full restrictions
//* https://github.com/idaholab/moose/blob/master/COPYRIGHT
//*
//* Licensed under LGPL 2.1, please see LICENSE for details
//* https://www.gnu.org/licenses/lgpl-2.1.html

#include "InterfaceValueMaterial_test_gradient.h"
#include "InterfaceValueTools.h"

registerMooseObject("PorousFlowApp", InterfaceValueMaterial_test_gradient);
registerMooseObject("MooseTestApp", InterfaceValueMaterial_test_gradient);
registerMooseObject("MooseTestApp", ADInterfaceValueMaterial_test_gradient);

template <bool is_ad>
InputParameters
InterfaceValueMaterial_test_gradientTempl<is_ad>::validParams()
{
  InputParameters params = InterfaceMaterial::validParams();
  params.addClassDescription("Calculates a variable's jump value across an interface.");
  params.addRequiredParam<std::string>(
      "mat_prop_primary", "The material property on the primary side of the interface");
  return params;
}

template <bool is_ad>
InterfaceValueMaterial_test_gradientTempl<is_ad>::InterfaceValueMaterial_test_gradientTempl(const InputParameters & parameters)
  : InterfaceMaterial(parameters),
    _mp_primary_name(getParam<std::string>("mat_prop_primary")),
    _mp_primary(declareProperty<std::vector<RealGradient>>(_mp_primary_name)),
    _mp_secondary(getNeighborMaterialPropertyByName<std::vector<RealGradient>>(_mp_primary_name))
{
}

template <bool is_ad>
void
InterfaceValueMaterial_test_gradientTempl<is_ad>::computeQpProperties()
{
    std::cout << "Hello from my InterfaceValueMaterial_test_gradientTempl<is_ad>::computeQpProperties!" << std::endl;
    mooseAssert(_neighbor_elem, "Neighbor elem is NULL!");
    _mp_primary[_qp] = _mp_secondary[_qp];
}

template <bool is_ad>
void
InterfaceValueMaterial_test_gradientTempl<is_ad>::initQpStatefulProperties()
{
  mooseAssert(_neighbor_elem, "Neighbor elem is NULL!");
  _mp_primary[_qp] = std::vector<RealGradient>(_mp_secondary[_qp].size(), RealGradient());
}

template class InterfaceValueMaterial_test_gradientTempl<false>;
template class InterfaceValueMaterial_test_gradientTempl<true>;
