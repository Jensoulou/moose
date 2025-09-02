//* This file is part of the MOOSE framework
//* https://mooseframework.inl.gov
//*
//* All rights reserved, see COPYRIGHT for full restrictions
//* https://github.com/idaholab/moose/blob/master/COPYRIGHT
//*
//* Licensed under LGPL 2.1, please see LICENSE for details
//* https://www.gnu.org/licenses/lgpl-2.1.html

#pragma once

#include "InterfaceMaterial.h"

/**
 * Interface material calculates a variable's jump value across an interface
 */
class PorousFlowMaterialPropertyLowerDimensional : public InterfaceMaterial
{
  public:
    static InputParameters validParams();

    PorousFlowMaterialPropertyLowerDimensional(const InputParameters & parameters);

    virtual void subdomainSetup() override;

    enum class ConstantTypeEnum
    {
      NONE,
      ELEMENT,
      SUBDOMAIN
    };
  protected:
    virtual void computeQpProperties() override;
    virtual void initQpStatefulProperties() override;

    const std::string _mp_name;
    const bool _gradient;
    MaterialProperty<Real> & _mp_primary;
    const MaterialProperty<Real> & _mp_secondary;
    MaterialProperty<std::vector<RealGradient>> * _mp_dprimary_dvar;
    const MaterialProperty<std::vector<RealGradient>> * _mp_dsecondary_dvar;

    bool _neighbor;
    const SubdomainID & _current_subdomain_id;
    /// Options of the constantness level of the material
    const ConstantTypeEnum _constant_option;

  private:
    ConstantTypeEnum computeConstantOption();
};

//typedef PorousFlowMaterialPropertyLowerDimensional<false> InterfaceValueMaterial;
