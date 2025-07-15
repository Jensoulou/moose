//* This file is part of the MOOSE framework
//* https://mooseframework.inl.gov
//*
//* All rights reserved, see COPYRIGHT for full restrictions
//* https://github.com/idaholab/moose/blob/master/COPYRIGHT
//*
//* Licensed under LGPL 2.1, please see LICENSE for details
//* https://www.gnu.org/licenses/lgpl-2.1.html

#include "PorousFlowMassVolumetricExpansionLowerDimensional.h"
#include "MooseMesh.h"
#include "Assembly.h"

#include "MooseVariable.h"

registerMooseObject("PorousFlowApp", PorousFlowMassVolumetricExpansionLowerDimensional);

InputParameters
PorousFlowMassVolumetricExpansionLowerDimensional::validParams()
{
  InputParameters params = PorousFlowMassVolumetricExpansion::validParams();
  params.addCoupledVar("aperture", 1.0, "Aperture of the fracture");
  params.addClassDescription("Component_mass*rate_of_solid_volumetric_expansion, times the aperture of the fracture."
                            "This Kernel lumps the component mass to the nodes.");
  return params;
}

PorousFlowMassVolumetricExpansionLowerDimensional::PorousFlowMassVolumetricExpansionLowerDimensional(
    const InputParameters & parameters)
  : PorousFlowMassVolumetricExpansion(parameters),
    _aperture(coupledValue("aperture")),
    _strain_rate_qp(getMaterialProperty<Real>("PorousFlow_volumetric_strain_rate_qp_ld")),
    _dstrain_rate_qp_dvar(getMaterialProperty<std::vector<RealGradient>>(
        "dPorousFlow_volumetric_strain_rate_qp_dvar_ld"))
{
  if (_fluid_component >= _dictator.numComponents())
    mooseError("The Dictator proclaims that the number of components in this simulation is ",
               _dictator.numComponents(),
               " whereas you have used the Kernel PorousFlowComponetMassVolumetricExpansion with "
               "component = ",
               _fluid_component,
               ".  The Dictator is watching you");

  if (isNodal())
    paramError("variable", "This AuxKernel only supports Elemental fields");
}

Real
PorousFlowMassVolumetricExpansionLowerDimensional::computeQpResidual()
{
  const unsigned elem_dim = _current_elem->dim();
  if (elem_dim == _mesh.dimension())
    mooseError("The variable ",
               _var.name(),
               " must must be defined on lower-dimensional elements "
               "only since it employs "
               "PorousFlowDarcyVelocityComponentLowerDimensional\n");
               
  return _aperture[_qp] * PorousFlowMassVolumetricExpansion::computeQpResidual();
}

Real
PorousFlowMassVolumetricExpansionLowerDimensional::computeQpJacobian()
{
  return _aperture[_qp] * PorousFlowMassVolumetricExpansion::computeQpJacobian();
}

Real
PorousFlowMassVolumetricExpansionLowerDimensional::computeQpOffDiagJacobian(unsigned int jvar)
{
  return _aperture[_qp] * PorousFlowMassVolumetricExpansion::computeQpOffDiagJacobian(jvar);
}

