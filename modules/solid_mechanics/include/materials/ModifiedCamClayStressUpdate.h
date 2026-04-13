//* This file is part of the MOOSE framework
//* https://www.mooseframework.org
//*
//* All rights reserved, see COPYRIGHT for full restrictions
//* https://github.com/idaholab/moose/blob/master/COPYRIGHT
//*
//* Licensed under LGPL 2.1, please see LICENSE for details
//* https://www.gnu.org/licenses/lgpl-2.1.html

/**
 * @class ModifiedCamClayStressUpdate
 * @brief Performs the implicit stress return mapping algorithm for the Modified Cam-Clay plasticity model.
 * @author Jens Niclaes, UCLouvain
 *
 * @details
 * This class inherits from `MultiParameterPlasticityStressUpdate` and is responsible for numerically 
 * integrating the elastoplastic constitutive equations. To guarantee numerical stability and clean code 
 * architecture, this class projects the 3D 6x6 Voigt stress tensor into a 2D stress parameter space 
 * comprised of:
 * - $p$: Mean stress (hydrostatic pressure), where $p = \text{tr}(\sigma)/3$
 * - $q$: Deviatoric stress (von Mises equivalent stress), where $q = \sqrt{3 J_2}$
 *
 * The class dynamically extracts isotropic elastic properties (Bulk Modulus $K$ and Shear Modulus $G$) 
 * directly from the provided `Eijkl` elasticity tensor and formulates the reduced stiffness (`_Eij`) 
 * and compliance (`_Cij`) matrices in the $p-q$ space.
 *
 * During the plastic regime, it updates the internal variable (plastic volumetric strain) which drives 
 * the hardening of the pre-consolidation pressure. 
 */

#pragma once

#include "MultiParameterPlasticityStressUpdate.h"
#include "SolidMechanicsHardeningModel.h"
#include <vector> 

/**
 * ModifiedCamClayStressUpdate implements rate-independent nonassociative
 * Modified Cam Clay plasticity with hardening/softening.
 */
class ModifiedCamClayStressUpdate : public MultiParameterPlasticityStressUpdate
{
public:
  static InputParameters validParams();

  ModifiedCamClayStressUpdate(const InputParameters & parameters);

  /**
   * Does the model require the elasticity tensor to be isotropic?
   */
  bool requiresIsotropicTensor() override { return true; }

  bool isIsotropic() override { return true; };
  
private: 
  const Real _numerical_zero = 1e-16;

protected:
  /// Value of the CSL slope
  Real _M;
  
  /// Hardening model for tensile strength
  const SolidMechanicsHardeningModel & _t_s;

  /// Hardening model for compressive strength
  const SolidMechanicsHardeningModel & _p_c;

  /// Value of the bulk modulus
  Real _K;

  /// Whether the plastic behaviour is associated or not
  const bool _associated;

  /// Value of the shear modulus
  Real _mu;

  /// Non associated factor value
  Real _NAfactor;

  void yieldFunctionValuesV(const std::vector<Real> & stress_params,
                            const std::vector<Real> & intnl,
                            std::vector<Real> & yf) const override;

  void computeAllQV(const std::vector<Real> & stress_params,
                    const std::vector<Real> & intnl,
                    std::vector<yieldAndFlow> & all_q) const override;

  void setIntnlValuesV(const std::vector<Real> & trial_stress_params,
                       const std::vector<Real> & current_stress_params,
                       const std::vector<Real> & intnl_old,
                       std::vector<Real> & intnl) const override;

  void setIntnlDerivativesV(const std::vector<Real> & trial_stress_params,
                            const std::vector<Real> & current_stress_params,
                            const std::vector<Real> & intnl,
                            std::vector<std::vector<Real>> & dintnl) const override;

  void computeStressParams(const RankTwoTensor & stress,
                           std::vector<Real> & stress_params) const override;

  std::vector<RankTwoTensor> dstress_param_dstress(const RankTwoTensor & stress) const override;

  std::vector<RankFourTensor> d2stress_param_dstress(const RankTwoTensor & stress) const override;

  virtual void setStressAfterReturnV(const RankTwoTensor & stress_trial,
                                     const std::vector<Real> & stress_params,
                                     Real gaE,
                                     const std::vector<Real> & intnl,
                                     const yieldAndFlow & smoothed_q,
                                     const RankFourTensor & Eijkl,
                                     RankTwoTensor & stress) const override;

  void setEffectiveElasticity(const RankFourTensor & Eijkl) override;


  virtual void preReturnMapV(const std::vector<Real> & trial_stress_params,
                             const RankTwoTensor & stress_trial,
                             const std::vector<Real> & intnl_old,
                             const std::vector<Real> & yf,
                             const RankFourTensor & Eijkl) override;
  
};
