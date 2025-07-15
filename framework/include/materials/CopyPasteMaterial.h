//* This file is part of the MOOSE framework
//* https://mooseframework.inl.gov
//*
//* All rights reserved, see COPYRIGHT for full restrictions
//* https://github.com/idaholab/moose/blob/master/COPYRIGHT
//*
//* Licensed under LGPL 2.1, please see LICENSE for details
//* https://www.gnu.org/licenses/lgpl-2.1.html

#pragma once

#include "Material.h"

/**
 * This material automatically declares as material properties whatever is passed to it
 * through the parameters 'prop_names' and uses the values from 'prop_values' as the values
 * for those properties.
 *
 * This is not meant to be used in a production capacity... and instead is meant to be used
 * during development phases for ultimate flexibility.
 */
template <typename T, bool is_ad>
class CopyPasteMaterialTempl : public Material//public TwoMaterialPropertyInterface, 
{
public:
  static InputParameters validParams();

  CopyPasteMaterialTempl(const InputParameters & parameters);

  std::pair<const Elem *, unsigned int> find_neighbor_element(const Elem * const elem) const;

protected:
  virtual void initQpStatefulProperties() override;
  virtual void computeQpProperties() override;

  /// Current neighbor element
  const Elem * const & _neighbor_elem;
  /// current side of the neighbor element
  const unsigned int & _neighbor_side;

  const std::string _prop_names;
  const std::string _neighbor_prop_names;
  const GenericMaterialProperty<T, is_ad> & _mp_primary;
  // const Real & _prop_values;

  GenericMaterialProperty<T, is_ad> & _properties;
};



// typedef CopyPasteMaterialTempl<false> CopyPasteMaterial;
// typedef CopyPasteMaterialTempl<true> ADCopyPasteMaterial;

typedef CopyPasteMaterialTempl<Real, false> CopyPasteMaterial;
typedef CopyPasteMaterialTempl<Real, true> ADCopyPasteMaterial;

typedef CopyPasteMaterialTempl<RealVectorValue, false> CopyPasteMaterialVec;
typedef CopyPasteMaterialTempl<RealVectorValue, true> ADCopyPasteMaterialVec;

typedef CopyPasteMaterialTempl<RealGradient, false> CopyPasteMaterialGradient;
typedef CopyPasteMaterialTempl<RealGradient, true> ADCopyPasteMaterialGradient;

typedef CopyPasteMaterialTempl<std::vector<RealVectorValue>, false> CopyPasteMaterialVecVec;
typedef CopyPasteMaterialTempl<std::vector<RealVectorValue>, true> ADCopyPasteMaterialVecVec;

typedef CopyPasteMaterialTempl<std::vector<RealGradient>, false> CopyPasteMaterialVecGradient;
typedef CopyPasteMaterialTempl<std::vector<RealGradient>, true> ADCopyPasteMaterialVecGradient;

typedef CopyPasteMaterialTempl<RankTwoTensor, false> CopyPasteMaterialTensor;
typedef CopyPasteMaterialTempl<RankTwoTensor, true> ADCopyPasteMaterialTensor;
