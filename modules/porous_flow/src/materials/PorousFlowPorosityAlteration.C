//* This file is part of the MOOSE framework
//* https://mooseframework.inl.gov
//*
//* All rights reserved, see COPYRIGHT for full restrictions
//* https://github.com/idaholab/moose/blob/master/COPYRIGHT
//*
//* Licensed under LGPL 2.1, please see LICENSE for details
//* https://www.gnu.org/licenses/lgpl-2.1.html

#include "PorousFlowPorosityAlteration.h"

registerMooseObject("PorousFlowApp", PorousFlowPorosityAlteration);

InputParameters
PorousFlowPorosityAlteration::validParams()
{
  InputParameters params = PorousFlowPorosity::validParams();  
  params.addParam<bool>(
      "alteration", false, "If true, porosity will be a function of the alteration index");
  params.addCoupledVar("alteration_index", 0.0, "Index of alteration integrated over time");
  params.addParam<Real>("alteration_scaling",
                        "Scaling coefficient to mitigate the alteration on the porosity");
  params.addClassDescription("This Material calculates the porosity PorousFlow simulations");
  return params;
}

PorousFlowPorosityAlteration::PorousFlowPorosityAlteration(const InputParameters & parameters)
  : PorousFlowPorosity(parameters),
  _alteration(getParam<bool>("alteration")),
  _ai(coupledValue("alteration_index")), 
  _ai_coef(isParamValid("alteration_scaling") ? getParam<Real>("alteration_scaling")
                                                       : 1.25e4)
{

}

Real
PorousFlowPorosityAlteration::atNegInfinityQp() const
{
  Real result = PorousFlowPorosity::atNegInfinityQp();
  if (_alteration)
  {
      result += _ai[_qp]/_ai_coef; 
  }
  return result;
}

Real
PorousFlowPorosityAlteration::datNegInfinityQp(unsigned pvar) const
{
  return PorousFlowPorosity::datNegInfinityQp(pvar);
}

Real
PorousFlowPorosityAlteration::atZeroQp() const
{
  Real result = PorousFlowPorosity::atZeroQp();
  const Real y_min = 0.01; 
  //Real y0 = _phi0[0]; 
  Real x = _ai[_qp] / _ai_coef;
  if (_alteration)
  {
    if (result + x > y_min)
    {
      result += x;
    }
    else
    {
      //std::cout << "atZeroQp (result) - 2nd option: bad result = "
      //         << result + x << std::endl;

      result = y_min * std::exp(((result + x) - y_min) / y_min);

      //std::cout << "replaced by: good result = " << result << std::endl;
    }
  }
  //std::cout << "atZeroQp (result): " << result << std::endl;
  return result;
}

Real
PorousFlowPorosityAlteration::datZeroQp(unsigned pvar) const
{
  return PorousFlowPorosity::datZeroQp(pvar);
}

Real
PorousFlowPorosityAlteration::decayQp() const
{
  return PorousFlowPorosity::decayQp();
  // Real result = 0.0;

  // result = PorousFlowPorosity::decayQp(); 

  // if (_alteration){
  //   result += _ai[_qp] * _ai_coef;
  //   // std::cout << "[DEBUG] alteration[qp] = " << _ai[_qp] << " Alteration coef = "<< _ai_coef << "\n";
  // }

  // return result;
}

Real
PorousFlowPorosityAlteration::ddecayQp_dvar(unsigned pvar) const
{
  return PorousFlowPorosity::ddecayQp_dvar(pvar); 

  // Real result = 0.0;

  // result = PorousFlowPorosity::ddecayQp_dvar(pvar); 

  // return result;
}

RealGradient
PorousFlowPorosityAlteration::ddecayQp_dgradvar(unsigned pvar) const
{
  return PorousFlowPorosity::ddecayQp_dgradvar(pvar);
}
