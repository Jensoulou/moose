//* This file is part of the MOOSE framework
//* https://mooseframework.inl.gov
//*
//* All rights reserved, see COPYRIGHT for full restrictions
//* https://github.com/idaholab/moose/blob/master/COPYRIGHT
//*
//* Licensed under LGPL 2.1, please see LICENSE for details
//* https://www.gnu.org/licenses/lgpl-2.1.html

#include "PorousFlowAdvectiveFluxLowerDimensional.h"
#include "MooseMesh.h"
#include "Assembly.h"

registerMooseObject("PorousFlowApp", PorousFlowAdvectiveFluxLowerDimensional);

InputParameters
PorousFlowAdvectiveFluxLowerDimensional::validParams()
{
  InputParameters params = PorousFlowAdvectiveFlux::validParams();
  params.addCoupledVar("aperture", 1.0, "Aperture of the fracture");
  params.addClassDescription(
      "Fully-upwinded advective flux of the component given by fluid_component, "
                             "multiplied by the aperture of the fracture. ");
  return params;
}

PorousFlowAdvectiveFluxLowerDimensional::PorousFlowAdvectiveFluxLowerDimensional(const InputParameters & parameters)
  : PorousFlowAdvectiveFlux(parameters),
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
PorousFlowAdvectiveFluxLowerDimensional::mobility(unsigned nodenum, unsigned phase) const
{
  const unsigned elem_dim = _current_elem->dim();
  if (elem_dim == _mesh.dimension())
    mooseError("The variable ",
               _var.name(),
               " must must be defined on lower-dimensional elements "
               "only since it employs "
               "PorousFlowDarcyVelocityComponentLowerDimensional\n");
               
  return _aperture[_qp] * PorousFlowAdvectiveFlux::mobility(nodenum, phase);
}

Real
PorousFlowAdvectiveFluxLowerDimensional::dmobility(unsigned nodenum, unsigned phase, unsigned pvar) const
{
  return _aperture[_qp] * PorousFlowAdvectiveFlux::dmobility(nodenum, phase, pvar);
}
