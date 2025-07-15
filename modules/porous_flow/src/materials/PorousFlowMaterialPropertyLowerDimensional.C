//* This file is part of the MOOSE framework
//* https://mooseframework.inl.gov
//*
//* All rights reserved, see COPYRIGHT for full restrictions
//* https://github.com/idaholab/moose/blob/master/COPYRIGHT
//*
//* Licensed under LGPL 2.1, please see LICENSE for details
//* https://www.gnu.org/licenses/lgpl-2.1.html

#include "PorousFlowMaterialPropertyLowerDimensional.h"
#include "InterfaceValueTools.h"

registerMooseObject("PorousFlowApp", PorousFlowMaterialPropertyLowerDimensional);

InputParameters
PorousFlowMaterialPropertyLowerDimensional::validParams()
{
  InputParameters params = InterfaceMaterial::validParams();
  params.addClassDescription("Copies a material property to a lower dimesional element nodes"
                             "from its higher dimensional element neighbour.");
  params.addRequiredParam<std::string>("mat_prop", "The material property on the primary side of the interface");
  params.addParam<bool>("need_gradient",
                        false,
                        "get also the gradient of the mat_prop");
  MooseEnum const_option("NONE=0 ELEMENT=1 SUBDOMAIN=2", "none");
  params.addParam<MooseEnum>(
      "constant_on",
      const_option,
      "When ELEMENT, MOOSE will only call computeQpProperties() for the 0th "
      "quadrature point, and then copy that value to the other qps."
      "When SUBDOMAIN, MOOSE will only call computeQpProperties() for the 0th "
      "quadrature point, and then copy that value to the other qps. Evaluations on element qps "
      "will be skipped");
  return params;
}

PorousFlowMaterialPropertyLowerDimensional::PorousFlowMaterialPropertyLowerDimensional(const InputParameters & parameters)
  : InterfaceMaterial(parameters),
    // Coupleable(this, false),
    // MaterialPropertyInterface(this, blockIDs(), boundaryIDs()),
    _mp_name(getParam<std::string>("mat_prop")),
    _gradient(getParam<bool>("need_gradient")),
    _mp_primary(declareProperty<Real>(_mp_name + "_ld")),
    _mp_secondary(getNeighborMaterialPropertyByName<Real>(_mp_name)), 
    _mp_dprimary_dvar(_gradient ? &declareProperty<std::vector<RealGradient>>("d"+_mp_name+"_dvar" + "_ld") : nullptr),
    _mp_dsecondary_dvar(_gradient ? &getNeighborMaterialPropertyByName<std::vector<RealGradient>>("d"+_mp_name+"_dvar") : nullptr), 
    _neighbor(_material_data_type == Moose::NEIGHBOR_MATERIAL_DATA),
    _current_subdomain_id(_neighbor ? _assembly.currentNeighborSubdomainID()
                                    : _assembly.currentSubdomainID()), 
    _constant_option(computeConstantOption())

{
  std::cout << "Hello from my PorousFlowMaterialPropertyLowerDimensional Params!" << std::endl;
  std::cout << _mp_name << std::endl;
}

void
PorousFlowMaterialPropertyLowerDimensional::computeQpProperties()
{
  mooseAssert(_neighbor_elem, "Neighbor elem is NULL!");

  _mp_primary[_qp] = _mp_secondary[_qp];
  
  std::cout << "Hello from my PorousFlowMaterialPropertyLowerDimensional::computeQpProperties!" << std::endl;
  std::cout << _mp_name + "_ld" << std::endl;

  if (_gradient)
    std::cout << "Hello from my PorousFlowMaterialPropertyLowerDimensional::computeQpProperties!, in the IF" << std::endl;
    std::cout << "d" + _mp_name + "_dvar_ld" << std::endl;
    // _mp_dprimary_dvar = &declareProperty<std::vector<RealGradient>>("d" + _mp_name + "_dvar_ld");
    (*_mp_dprimary_dvar)[_qp] = (*_mp_dsecondary_dvar)[_qp];
  
}

void
PorousFlowMaterialPropertyLowerDimensional::initQpStatefulProperties()
{
  std::cout << "Hello from my PorousFlowMaterialPropertyLowerDimensional::initQpStatefulProperties!" << std::endl;
  mooseAssert(_neighbor_elem, "Neighbor elem is NULL!");
  _mp_primary[_qp] = 0.0; 
}



// template class PorousFlowMaterialPropertyLowerDimensional<false>;
// template class PorousFlowMaterialPropertyLowerDimensional<true>;


// function from Material.C

void
PorousFlowMaterialPropertyLowerDimensional::subdomainSetup()
{
  std::cout << "Hello from my PorousFlowMaterialPropertyLowerDimensional::subdomainSetup!" << std::endl;
  if (_constant_option == ConstantTypeEnum::SUBDOMAIN)
  {
    auto nqp = _fe_problem.getMaxQps();

    MaterialProperties & props = materialData().props();
    for (const auto & prop_id : _supplied_prop_ids)
      props[prop_id].resize(nqp);

    // consider all properties are active
    _active_prop_ids.clear();
    for (const auto & id : _supplied_prop_ids)
      _active_prop_ids.insert(id);

    _qp = 0;
    computeQpProperties();

    for (const auto & prop_id : _supplied_prop_ids)
      for (decltype(nqp) qp = 1; qp < nqp; ++qp)
        props[prop_id].qpCopy(qp, props[prop_id], 0);
  }
}

PorousFlowMaterialPropertyLowerDimensional::ConstantTypeEnum
PorousFlowMaterialPropertyLowerDimensional::computeConstantOption()
{
  std::cout << "Hello from my PorousFlowMaterialPropertyLowerDimensional::computeConstantOption!" << std::endl;
  auto co = getParam<MooseEnum>("constant_on").getEnum<ConstantTypeEnum>();

  // If the material is operating on a boundary we'll have to _at least_ run it
  // once per element, as there is no boundarySetup, and boundaries are worked
  // on as they are encountered on the elements while looping elements.
  if (_bnd && co == ConstantTypeEnum::SUBDOMAIN)
    co = ConstantTypeEnum::ELEMENT;

  return co;
}