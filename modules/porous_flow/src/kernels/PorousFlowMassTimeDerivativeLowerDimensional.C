//* This file is part of the MOOSE framework
//* https://mooseframework.inl.gov
//*
//* All rights reserved, see COPYRIGHT for full restrictions
//* https://github.com/idaholab/moose/blob/master/COPYRIGHT
//*
//* Licensed under LGPL 2.1, please see LICENSE for details
//* https://www.gnu.org/licenses/lgpl-2.1.html

#include "PorousFlowMassTimeDerivativeLowerDimensional.h"
#include "MooseMesh.h"
#include "Assembly.h"


#include "MooseVariable.h"

#include "libmesh/quadrature.h"

#include <limits>

registerMooseObject("PorousFlowApp", PorousFlowMassTimeDerivativeLowerDimensional);

InputParameters
PorousFlowMassTimeDerivativeLowerDimensional::validParams()
{
  InputParameters params = PorousFlowMassTimeDerivative::validParams();
  params.addCoupledVar("aperture", 1.0, "Aperture of the fracture");
  params.addClassDescription("Derivative of fluid-component mass with respect to time, "
                             "multiplied by the aperture of the fracture. "
                             "Mass lumping to the nodes is used.");
  return params;
}

PorousFlowMassTimeDerivativeLowerDimensional::PorousFlowMassTimeDerivativeLowerDimensional(const InputParameters & parameters)
  : PorousFlowMassTimeDerivative(parameters),
    _aperture(coupledValue("aperture"))
{
  if (_fluid_component >= _dictator.numComponents())
    paramError(
        "fluid_component",
        "The Dictator proclaims that the maximum fluid component index in this simulation is ",
        _dictator.numComponents() - 1,
        " whereas you have used ",
        _fluid_component,
        ". Remember that indexing starts at 0. The Dictator does not take such mistakes lightly.");

  if (isNodal())
    paramError("variable", "This AuxKernel only supports Elemental fields");
}

Real
PorousFlowMassTimeDerivativeLowerDimensional::computeQpResidual()
{
  const unsigned elem_dim = _current_elem->dim();
  if (elem_dim == _mesh.dimension())
    mooseError("The variable ",
               _var.name(),
               " must must be defined on lower-dimensional elements "
               "only since it employs "
               "PorousFlowDarcyVelocityComponentLowerDimensional\n");

  return _aperture[_qp] * PorousFlowMassTimeDerivative::computeQpResidual();
}

Real
PorousFlowMassTimeDerivativeLowerDimensional::computeQpJacobian()
{
  return _aperture[_qp] * PorousFlowMassTimeDerivative::computeQpJacobian();
}

Real
PorousFlowMassTimeDerivativeLowerDimensional::computeQpOffDiagJacobian(unsigned int jvar)
{
  return _aperture[_qp] * PorousFlowMassTimeDerivative::computeQpOffDiagJacobian(jvar);
}
