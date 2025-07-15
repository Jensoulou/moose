//* This file is part of the MOOSE framework
//* https://mooseframework.inl.gov
//*
//* All rights reserved, see COPYRIGHT for full restrictions
//* https://github.com/idaholab/moose/blob/master/COPYRIGHT
//*
//* Licensed under LGPL 2.1, please see LICENSE for details
//* https://www.gnu.org/licenses/lgpl-2.1.html

#include "PorousFlowDispersiveFluxLowerDimensional.h"
#include "MooseMesh.h"
#include "Assembly.h"


#include "MooseVariable.h"

registerMooseObject("PorousFlowApp", PorousFlowDispersiveFluxLowerDimensional);

InputParameters
PorousFlowDispersiveFluxLowerDimensional::validParams()
{
  InputParameters params = PorousFlowDispersiveFlux::validParams();
  params.addCoupledVar("aperture", 1.0, "Aperture of the fracture");
  params.addClassDescription(
      "Dispersive and diffusive flux of the component given by fluid_component in all phases, "
      "multiplied by the aperture of the fracture");
  return params;
}

PorousFlowDispersiveFluxLowerDimensional::PorousFlowDispersiveFluxLowerDimensional(const InputParameters & parameters)
  : PorousFlowDispersiveFlux(parameters),
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

  // Check that sufficient values of the dispersion coefficients have been entered
  if (_disp_long.size() != _num_phases)
    paramError(
        "disp_long",
        "The number of longitudinal dispersion coefficients is not equal to the number of phases");

  if (_disp_trans.size() != _num_phases)
    paramError("disp_trans",
               "The number of transverse dispersion coefficients disp_trans is not equal to the "
               "number of phases");

  if (isNodal())
    paramError("variable", "This AuxKernel only supports Elemental fields");
}

Real
PorousFlowDispersiveFluxLowerDimensional::computeQpResidual()
{
  const unsigned elem_dim = _current_elem->dim();
  if (elem_dim == _mesh.dimension())
    mooseError("The variable ",
               _var.name(),
               " must must be defined on lower-dimensional elements "
               "only since it employs "
               "PorousFlowDarcyVelocityComponentLowerDimensional\n");
               
  return _aperture[_qp] * PorousFlowDispersiveFlux::computeQpResidual();
}

Real
PorousFlowDispersiveFluxLowerDimensional::computeQpJacobian()
{
  return _aperture[_qp] * PorousFlowDispersiveFlux::computeQpJacobian();
}

Real
PorousFlowDispersiveFluxLowerDimensional::computeQpOffDiagJacobian(unsigned int jvar)
{
  return _aperture[_qp] * PorousFlowDispersiveFlux::computeQpOffDiagJacobian(jvar);
}
