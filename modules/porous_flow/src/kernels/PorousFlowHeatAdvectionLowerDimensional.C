//* This file is part of the MOOSE framework
//* https://mooseframework.inl.gov
//*
//* All rights reserved, see COPYRIGHT for full restrictions
//* https://github.com/idaholab/moose/blob/master/COPYRIGHT
//*
//* Licensed under LGPL 2.1, please see LICENSE for details
//* https://www.gnu.org/licenses/lgpl-2.1.html

#include "PorousFlowHeatAdvectionLowerDimensional.h"
#include "MooseMesh.h"
#include "Assembly.h"


registerMooseObject("PorousFlowApp", PorousFlowHeatAdvectionLowerDimensional);

InputParameters
PorousFlowHeatAdvectionLowerDimensional::validParams()
{
  InputParameters params = PorousFlowHeatAdvection::validParams();
  params.addCoupledVar("aperture", 1.0, "Aperture of the fracture");
  params.addClassDescription("Fully-upwinded heat flux, advected by the fluid, "
                             "multiplied by the aperture of the fracture. ");
  return params;
}

PorousFlowHeatAdvectionLowerDimensional::PorousFlowHeatAdvectionLowerDimensional(const InputParameters & parameters)
  : PorousFlowHeatAdvection(parameters),
    _aperture(coupledValue("aperture"))
{
  if (isNodal())
    paramError("variable", "This AuxKernel only supports Elemental fields");
}

Real
PorousFlowHeatAdvectionLowerDimensional::mobility(unsigned nodenum, unsigned phase) const
{
  const unsigned elem_dim = _current_elem->dim();
  if (elem_dim == _mesh.dimension())
    mooseError("The variable ",
               _var.name(),
               " must must be defined on lower-dimensional elements "
               "only since it employs "
               "PorousFlowDarcyVelocityComponentLowerDimensional\n");

  return _aperture[_qp] * PorousFlowHeatAdvection::mobility(nodenum, phase);
}

Real
PorousFlowHeatAdvectionLowerDimensional::dmobility(unsigned nodenum, unsigned phase, unsigned pvar) const
{
  return _aperture[_qp] * PorousFlowHeatAdvection::dmobility(nodenum, phase, pvar);
}
