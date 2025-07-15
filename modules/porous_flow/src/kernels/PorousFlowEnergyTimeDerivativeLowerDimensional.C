//* This file is part of the MOOSE framework
//* https://mooseframework.inl.gov
//*
//* All rights reserved, see COPYRIGHT for full restrictions
//* https://github.com/idaholab/moose/blob/master/COPYRIGHT
//*
//* Licensed under LGPL 2.1, please see LICENSE for details
//* https://www.gnu.org/licenses/lgpl-2.1.html

#include "PorousFlowEnergyTimeDerivativeLowerDimensional.h"

#include "MooseVariable.h"

registerMooseObject("PorousFlowApp", PorousFlowEnergyTimeDerivativeLowerDimensional);

InputParameters
PorousFlowEnergyTimeDerivativeLowerDimensional::validParams()
{
  InputParameters params = PorousFlowEnergyTimeDerivative::validParams();
  params.addCoupledVar("aperture", 1.0, "Aperture of the fracture");
  params.addClassDescription("Derivative of heat-energy-density wrt time, "
                             "multiplied by the aperture of the fracture");
  return params;
}

PorousFlowEnergyTimeDerivativeLowerDimensional::PorousFlowEnergyTimeDerivativeLowerDimensional(const InputParameters & parameters)
  : PorousFlowEnergyTimeDerivative(parameters),
    _aperture(coupledValue("aperture"))
{
}

Real
PorousFlowEnergyTimeDerivativeLowerDimensional::computeQpResidual()
{
  const unsigned elem_dim = _current_elem->dim();
  if (elem_dim == _mesh.dimension())
    mooseError("The variable ",
               _var.name(),
               " must must be defined on lower-dimensional elements "
               "only since it employs "
               "PorousFlowDarcyVelocityComponentLowerDimensional\n");

  // if (_aperture[_qp] < 0)
  // {
  //   mooseWarning("The aperture should be greater or equal to zero at all nodes."
  //                 "Resetting to 0.");
  //   _aperture[_qp] = std::max(0.0, _aperture[_qp]);
  // }

  return _aperture[_qp] * PorousFlowEnergyTimeDerivative::computeQpResidual();
}

Real
PorousFlowEnergyTimeDerivativeLowerDimensional::computeQpJacobian()
{
  return _aperture[_qp] * PorousFlowEnergyTimeDerivative::computeQpJacobian();
}

Real
PorousFlowEnergyTimeDerivativeLowerDimensional::computeQpOffDiagJacobian(unsigned int jvar)
{
  return _aperture[_qp] * PorousFlowEnergyTimeDerivative::computeQpOffDiagJacobian(jvar);
}
