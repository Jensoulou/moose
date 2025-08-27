//* This file is part of the MOOSE framework
//* https://mooseframework.inl.gov
//*
//* All rights reserved, see COPYRIGHT for full restrictions
//* https://github.com/idaholab/moose/blob/master/COPYRIGHT
//*
//* Licensed under LGPL 2.1, please see LICENSE for details
//* https://www.gnu.org/licenses/lgpl-2.1.html

#include "PorousFlowHeatConductionLowerDimensional.h"
#include "MooseMesh.h"
#include "Assembly.h"


#include "MooseVariable.h"

registerMooseObject("PorousFlowApp", PorousFlowHeatConductionLowerDimensional);

InputParameters
PorousFlowHeatConductionLowerDimensional::validParams()
{
  InputParameters params = PorousFlowHeatConduction::validParams();
  params.addCoupledVar("aperture", 1.0, "Aperture of the fracture");
  params.addClassDescription("Heat conduction in the Porous Flow module, "
                             "multiplied by the aperture of the fracture. ");
  return params;
}

PorousFlowHeatConductionLowerDimensional::PorousFlowHeatConductionLowerDimensional(const InputParameters & parameters)
  : PorousFlowHeatConduction(parameters),
    _aperture(coupledValue("aperture"))
{
  if (isNodal())
    paramError("variable", "This AuxKernel only supports Elemental fields");
}

Real
PorousFlowHeatConductionLowerDimensional::computeQpResidual()
{
  const unsigned elem_dim = _current_elem->dim();
  if (elem_dim == _mesh.dimension())
    mooseError("The variable ",
               _var.name(),
               " must must be defined on lower-dimensional elements "
               "only since it employs "
               "PorousFlowDarcyVelocityComponentLowerDimensional\n");

  return _aperture[_qp] * PorousFlowHeatConduction::computeQpResidual();
}

Real
PorousFlowHeatConductionLowerDimensional::computeQpJacobian()
{
  return _aperture[_qp] * PorousFlowHeatConduction::computeQpJacobian();
}

Real
PorousFlowHeatConductionLowerDimensional::computeQpOffDiagJacobian(unsigned int jvar)
{
  return _aperture[_qp] * PorousFlowHeatConduction::computeQpOffDiagJacobian(jvar);
}
