//* This file is part of the MOOSE framework
//* https://mooseframework.inl.gov
//*
//* All rights reserved, see COPYRIGHT for full restrictions
//* https://github.com/idaholab/moose/blob/master/COPYRIGHT
//*
//* Licensed under LGPL 2.1, please see LICENSE for details
//* https://www.gnu.org/licenses/lgpl-2.1.html

#include "CopyPasteMaterial.h"

registerMooseObject("MooseApp", CopyPasteMaterial);
registerMooseObject("MooseApp", ADCopyPasteMaterial);
registerMooseObject("MooseApp", CopyPasteMaterialVec);
registerMooseObject("MooseApp", ADCopyPasteMaterialVec);
registerMooseObject("MooseApp", CopyPasteMaterialGradient);
registerMooseObject("MooseApp", ADCopyPasteMaterialGradient);
registerMooseObject("MooseApp", CopyPasteMaterialVecVec);
registerMooseObject("MooseApp", ADCopyPasteMaterialVecVec);
registerMooseObject("MooseApp", CopyPasteMaterialVecGradient);
registerMooseObject("MooseApp", ADCopyPasteMaterialVecGradient);
registerMooseObject("MooseApp", CopyPasteMaterialTensor);
registerMooseObject("MooseApp", ADCopyPasteMaterialTensor);

template <typename T, bool is_ad>
InputParameters
CopyPasteMaterialTempl<T, is_ad>::validParams()
{
  InputParameters params = Material::validParams();
  params.addClassDescription(
      "Declares one material property based on 'prop_names' and attributes values by copy-pasting those of the neighbor.");
  params.addRequiredParam<std::string>(
      "prop_names", "The name of the property this material will have");
  params.addRequiredParam<std::string>(
      "neighbor_prop_names", "The name of the property to be copied from the neighbor");
  // params.addRequiredParam<Real>("prop_values",
  //                                            "The values associated with the named properties");
  params.set<MooseEnum>("constant_on") = "NONE";
  // params.declareControllable("prop_values");
  return params;
}

template <typename T, bool is_ad>
CopyPasteMaterialTempl<T, is_ad>::CopyPasteMaterialTempl(
    const InputParameters & parameters)
  : Material(parameters),
    _neighbor_elem(_assembly.neighbor()),
    _neighbor_side(_assembly.neighborSide()),
    _prop_names(getParam<std::string>("prop_names")),
    _neighbor_prop_names(getParam<std::string>("neighbor_prop_names")),
    _mp_primary(MaterialPropertyInterface::getGenericMaterialPropertyByName<T, is_ad>(
                  _neighbor_prop_names,
                  _fe_problem.getMaterialData(Moose::NEIGHBOR_MATERIAL_DATA, parameters.get<THREAD_ID>("_tid")),
                  0)),
    // _prop_values(getParam<Real>("prop_values")), 
    _properties(declareGenericProperty<T, is_ad>(_prop_names))
{

}

template <typename T, bool is_ad>
void
CopyPasteMaterialTempl<T, is_ad>::initQpStatefulProperties()
{
  computeQpProperties();
}

template <typename T, bool is_ad>
void
CopyPasteMaterialTempl<T, is_ad>::computeQpProperties()
{
  auto [neighbor_elem, side] = find_neighbor_element(_current_elem);
  std::cout << "_current_elem id: " << _current_elem->id() << std::endl;
  std::cout << "_current_elem dim: " << _current_elem->dim() << std::endl;
  std::cout << "neighbor_elem id: " << neighbor_elem->id() << std::endl;
  std::cout << "neighbor_elem dim: " << neighbor_elem->dim() << std::endl;
  std::cout << "neighbor_elem side dim: " << neighbor_elem->side_ptr(side)->dim() << std::endl;
  std::cout << "______neighbor_elem: " << _assembly.neighbor() << std::endl;
  std::cout << "neighborLowerDElem: " << _assembly.neighborLowerDElem() << std::endl;
  _assembly.reinitNeighborAtPhysical(neighbor_elem, side, std::vector<Point>{_q_point[_qp]}); 
  std::cout << "______neighbor_elem: " << _assembly.neighbor() << std::endl;
  std::cout << "______neighbor_elem id: " << _assembly.neighbor()->id() << std::endl;
  std::cout << "______neighbor_elem dim: " << _assembly.neighbor()->dim() << std::endl;
  std::cout << "______neighbor_elem face pt: " << _assembly.neighbor()->side_ptr(side)->point(0) << std::endl;
  std::cout << "______neighbor_elem face pt: " << _assembly.neighbor()->side_ptr(side)->point(1) << std::endl;
  // std::cout << "______neighbor_elem qrule face: " << _assembly.qruleFace(neighbor_elem, side)->n_points() << std::endl;
  // std::cout << "______current_elem qrule face: " << _qrule->n_points() << std::endl;

  // const auto & qpts = _assembly.qRuleNeighbor();

  std::cout << "==== Face neighbor quadrature points ====" << std::endl;
  for (unsigned int i = 0; i < _assembly.qRuleNeighbor()->n_points(); ++i){
    const Point & qpoint = _assembly.qRuleNeighbor()->get_points()[i];
    std::cout << "QP " << i << ": " << qpoint << std::endl;
    std::cout << "  Point[" << i << "] = " << qpoint << std::endl;
    if constexpr (std::is_same_v<T, Real>)
      std::cout << "  Material value[" << i << "] = " << _mp_primary[i] << std::endl;
    // std::cout << " Real[_qp]: " << _mp_primary[i] << std::endl;
  }

  // const unsigned int dim = neighbor_elem->side_ptr(side)->dim();
  // if (_current_elem->dim() == dim){
    // QuadratureType qtype = _qrule->type();
    // Order qorder = _qrule->get_order();
    // std::cout << "_current_elem order(): " << qorder << std::endl;
    // std::cout << "_current_elem qtype(): " << qtype << std::endl;
    // std::unique_ptr<QBase> qrule = QBase::build(qtype, dim, qorder);
    // Reinitialize the quadrature rule for this element
    // qrule->init(*neighbor_elem);

    // Now loop over the quadrature points
  //   for (unsigned int qp = 0; qp < qrule->n_points(); ++qp)
  //   {
  //     const Point & qpoint = qrule->get_points()[qp];
  //     std::cout << "QP " << qp << ": " << qpoint << std::endl;
  //   }
    
  //   for (unsigned int qp = 0; qp < _assembly.qRuleFace()->n_points(); ++qp)
  //   {
  //     const Point & qpoint = _assembly.qRuleFace()->get_points()[qp];
  //     std::cout << "QP " << qp << ": " << qpoint << std::endl;
  //   }
  // }

  // // Choose an order for the quadrature rule
  // const Order qorder = THIRD; // or whatever is appropriate

  // // Choose a quadrature type
  // const QuadratureType qtype = QGAUSS;

  // // Create the quadrature rule
  // std::unique_ptr<QBase> qrule = QBase::build(qtype, dim, qorder);

  // // Reinitialize the quadrature rule for this element
  // qrule->reinit(*elem);

  // // Now loop over the quadrature points
  // for (unsigned int qp = 0; qp < qrule->n_points(); ++qp)
  // {
  //   const Point & qpoint = qrule->get_points()[qp];
  //   std::cout << "QP " << qp << ": " << qpoint << std::endl;
  // }

  if constexpr (std::is_same_v<T, Real>)
    std::cout << "At _qp " << _qp << " at " << _q_point[_qp] << " Real[_qp]: " << _mp_primary[_qp] << std::endl;
  else if constexpr (std::is_same_v<T, RealVectorValue>)
    std::cout << "RealVectorValue: " << _mp_primary[_qp] << std::endl;
  else if constexpr (std::is_same_v<T, RankTwoTensor>)
    std::cout << "Tensor: " << _mp_primary[_qp] << std::endl;
  else if constexpr (std::is_same_v<T, std::vector<RealVectorValue>>){
      std::cout << "Vector of RealVectorValue at qp = " << _qp << ": ";
      for (const auto & val : _mp_primary[_qp])
        std::cout << val << ", ";
      std::cout << std::endl;
    }
  else
    mooseWarning("Print not implemented for this material property type.");

  _properties[_qp] = _mp_primary[_qp];
}

template <typename T, bool is_ad>
std::pair<const Elem *, unsigned int> 
CopyPasteMaterialTempl<T, is_ad>::find_neighbor_element(const Elem * const elem) const
{
  for (const auto & neighbor :  _mesh.getMesh().active_element_ptr_range())
  {
    if (elem->subdomain_id() == neighbor->subdomain_id())
      continue;

    for (unsigned int side = 0; side < neighbor->n_sides(); ++side) 
    {
      unsigned int shared_points = 0; 
      for (unsigned int k = 0; k < neighbor->side_ptr(side)->n_nodes(); ++k)
      {
        const Point & coord_neighbor = neighbor->side_ptr(side)->point(k);
        for (unsigned int i = 0; i < elem->n_nodes(); ++i)
        {
          const Point & coord_elem = elem->point(i);
          if (coord_elem == coord_neighbor)
          {
            shared_points = shared_points + 1; 
          }
        }
      }
      if (shared_points == 2)
      {
        return std::make_pair(neighbor, side); 
      }
    }
  }
  
  return std::make_pair(nullptr, 0); // No neighbor+side found
}


// Scalar
template class CopyPasteMaterialTempl<Real, false>;
template class CopyPasteMaterialTempl<Real, true>;

// Vector
template class CopyPasteMaterialTempl<RealVectorValue, false>;
template class CopyPasteMaterialTempl<RealVectorValue, true>;

// Gradient
// template class CopyPasteMaterialTempl<RealGradient, false>;
// template class CopyPasteMaterialTempl<RealGradient, true>;

// Rank-two tensor
template class CopyPasteMaterialTempl<RankTwoTensor, false>;
template class CopyPasteMaterialTempl<RankTwoTensor, true>;

// Vector of RealVectorValue
template class CopyPasteMaterialTempl<std::vector<RealVectorValue>, false>;
template class CopyPasteMaterialTempl<std::vector<RealVectorValue>, true>;

// Vector of RealGradient
// template class CopyPasteMaterialTempl<std::vector<RealGradient>, false>;
// template class CopyPasteMaterialTempl<std::vector<RealGradient>, true>;
