//* This file is part of the MOOSE framework
//* https://www.mooseframework.org
//*
//* All rights reserved, see COPYRIGHT for full restrictions
//* https://github.com/idaholab/moose/blob/master/COPYRIGHT
//*
//* Licensed under LGPL 2.1, please see LICENSE for details
//* https://www.gnu.org/licenses/lgpl-2.1.html

/**
 * @file ModifiedCamClayStressUpdate.C
 * @brief Implementation of the stress integration and return-mapping for Modified Cam-Clay.
 * @author Jens Niclaes, UCLouvain
 * * Key implementation details:
 * - Safeguards against NaN errors by introducing a `_numerical_zero` check when calculating 
 * derivatives involving the second invariant $J_2$.
 * - Handles both associated and non-associated flow rules via the `_NAfactor` scalar, which 
 * defaults to $1/M^2$ for associated flow.
 * - Enforces standard soil mechanics sign conventions: Tensile strength ($t_s$) must be positive, 
 * and pre-consolidation pressure ($p_c$) must be negative (compression).
 */

#include "ModifiedCamClayStressUpdate.h"
#include "libmesh/utility.h"
#include "ElasticityTensorTools.h"
#include <iostream>

registerMooseObject("SolidMechanicsApp", ModifiedCamClayStressUpdate);

InputParameters
ModifiedCamClayStressUpdate::validParams()
{
  InputParameters params = MultiParameterPlasticityStressUpdate::validParams();
  params.addRequiredParam<Real>(
      "CSL",
      "A Real that defines the slope of the Critical state line. ");
  params.addRequiredParam<UserObjectName>(
      "tensile_strength",
      "A SolidMechanicsHardening UserObject that defines hardening of the "
      "tensile strength.  In physical situations this is positive (and always "
      "must be greater than negative compressive-strength.");
  params.addRequiredParam<UserObjectName>(
      "preconsolidation",
      "A SolidMechanicsHardening UserObject that defines the hardening of the pre-consolidation "
      "pressure. In other words, it gives where the plastic surface intersects the mean stress axis. ");
  params.addParam<bool>("associated",
                        true,
                        "Is the plastic behavior associated?");
  params.addClassDescription("Nonassociative, smoothed, Modified Cam Clay plasticity with hardening/softening");
  return params;
}

ModifiedCamClayStressUpdate::ModifiedCamClayStressUpdate(const InputParameters & parameters)
  : MultiParameterPlasticityStressUpdate(parameters, 2, 1, 1), //(const InputParameters & parameters, unsigned num_sp, unsigned num_yf, unsigned num_intnl)
    _M(getParam<Real>("CSL")),
    _t_s(getUserObject<SolidMechanicsHardeningModel>("tensile_strength")),
    _p_c(getUserObject<SolidMechanicsHardeningModel>("preconsolidation")),
    _associated(getParam<bool>("associated"))
{
  mooseAssert(_t_s.get() != nullptr, "Tensile strength UserObject is not properly initialized.");
  mooseAssert(_p_c.get() != nullptr, "Preconsolidation UserObject is not properly initialized.");
  if (_p_c.value(0.0) > 0.0)
    mooseError("Preconsolidation pressure (_p_c) must be negative because it represents compression.");
  if (_t_s.value(0.0) < 0.0)
    mooseError("Tensile strength (_t_s) must be positive because it represents traction.");
  //if (_K <= 0.0)
  //  mooseError("Bulk modulus must be positive");
  //if (!_associated && _mu <= 0.0)
  //  mooseError("Shear modulus must be positive for non-associated flow rule");

  //std::cout << "ModifiedCamClay - Initialization" << std::endl;
}


////////////// A.FUNCTIONS NECESSARY OVERRIDDEN ///////////////////
/**
 * List of the functions needing to be overriden: 
 * A.1 virtual void yieldFunctionValuesV
 * A.2 virtual void computeAllQV --> Isn't clearly specified, but I believe we should override it
 * A.3 virtual void setIntnlValuesV
 * A.4 virtual void setIntnlDerivativesV
 * A.5 virtual void computeStressParams
 * A.6 virtual void setStressAfterReturnV
 * 
 * List of functions that should be overriden when the previous list 
 * is taken into consideration: 
 * A.5.1 virtual std::vector<RankTwoTensor> dstress_param_dstress
 * A.5.2 virtual std::vector<RankFourTensor> d2stress_param_dstress
 * •
 */

////////////// FUNCTIONS POSSIBLY OVERRIDDEN ///////////////////
/**
 * List of the functions that we may choose to override: 
 * B.1 virtual void initializeVarsV
 * B.2 virtual void setInelasticStrainIncrementAfterReturn
 * B.3 virtual void consistentTangentOperatorV
 */

////////////// FUNCTIONS POSSIBLY USED ///////////////////
/**
 * List of the functions that we may use: 
 * C.1 virtual void preReturnMapV --> To record stuff or do other computations prior to the return-mapping algorithm
 * C.2 virtual void finalizeReturnProcess --> To perform calculations before any return-map process is performed
 * C.3 virtual void initializeReturnProcess --> To perform calculations after the return-map process has completed 
 *                                            successfully in stress_param space but before the returned stress 
 *                                            tensor has been calculcated.
 */


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////            A             /////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * A.1 virtual void yieldFunctionValuesV --> Need to be overriden
 * 
 *    * The yield function is F = 1/M^2 * q^2 + (p - t_s) * (p - p_c)
  * See eq. 2.2 of "Cam-clay pasticity, Part I: Implicit integration of elasto-plastic
  * consitutive relations" of Ronaldo I. BORJA and Seung R. LEE (1990)
  * Where: 
  *        q = J2 = second invariant (but different from I2) = Von Mises stress
  *        p = mean stress = trace(stress)/3 = tr(s). The trace(stress) is the first invariant I1
  *        M = slope of the CSL line and is the summit of the ellipses in the p-q plane
  *        p_c = the preconsolidation pressure, meaning the intersection other than at the 
  *              origin between the plastic surface and the p-axis. 
  *
  * Since we defined two stress parameters as p and q, the equation is easily written
 */
void
ModifiedCamClayStressUpdate::yieldFunctionValuesV(
    const std::vector<Real> & stress_params,  // 1. Pass-by-const-reference: Read-only access to stress_params
    const std::vector<Real> & intnl,          // 2. Pass-by-const-reference: Read-only access to intnl
    std::vector<Real> & yf) const             // 3. Pass-by-reference: Allows, within the function, modification the value of the object referred by yf 
{
  //std::cout << "yieldFunctionValuesV" << std::endl;
  // Compute material parameters
  const Real M = _M;     // Slope of the critical state line
  const Real t_s = _t_s.value(intnl[0]); // Tensile strength
  const Real p_c = _p_c.value(intnl[0]); // Pre-consolidation pressure

  // Compute invariants (p and q)
  const Real p = stress_params[0];                        // Mean stress
  const Real q = stress_params[1];                        // Deviatoric stress magnitude                               
  
  yf[0] = (q * q) / (M * M) + (p - t_s) * (p - p_c);
}


/**
 * A.2 virtual void computeAllQV --> Isn't clearly specified, but I believe we should override it
 */
void
ModifiedCamClayStressUpdate::computeAllQV(const std::vector<Real> & stress_params,          // 1. Pass-by-const-reference: Read-only access to stress_params
                                            const std::vector<Real> & intnl,                // 2. Pass-by-const-reference: Read-only access to intnl
                                            std::vector<yieldAndFlow> & all_q) const        // 3. Pass-by-reference: Allows, within the function, modification the value of the object referred by all_q
{
  //std::cout << "computeAllQV" << std::endl;
   //Completely fills all_q with correct values.  These values are:
   //(1) the yield function values, yf[i]
   //(2) d(yf[i])/d(stress_params[j])
   //(3) d(yf[i])/d(intnl[j])
   //(4) d(flowPotential[i])/d(stress_params[j])
   //(5) d2(flowPotential[i])/d(stress_params[j])/d(stress_params[k])
   //(6) d2(flowPotential[i])/d(stress_params[j])/d(intnl[k])

  // Compute material parameters
  const Real phi = _NAfactor;  // 1 over the sqrt of the slope of the (non) associated critical state line
  const Real M = _M;     // Slope of the critical state line
  const Real t_s = _t_s.value(intnl[0]); // Tensile strength
  const Real p_c = _p_c.value(intnl[0]); // Pre-consolidation pressure
  const Real dMdi = 0.0; //_M.derivative(intnl[0]);
  const Real dt_sdi = _t_s.derivative(intnl[0]);
  const Real dp_cdi = _p_c.derivative(intnl[0]);

  // Compute invariants (p and q)
  const Real p = stress_params[0];       // Mean stress
  const Real q = stress_params[1];       // Deviatoric stress magnitude   

  //(1) the yield function values, yf[i]
  all_q[0].f = (q * q) / (M * M) + (p - t_s) * (p - p_c); // F = 1/M^2 * q^2 + p^2 - (p_c + t_s) * p + t_s * p_c

  // (2) d(yf[i])/d(stress_params[j])
  all_q[0].df[0] = 2 * p - (p_c + t_s); //df/dp
  all_q[0].df[1] = 1/(M * M) * 2 * q; //df/dq

  // (3) d(yf[i])/d(intnl[j])
  all_q[0].df_di[0] = -2 * (q * q) / (M * M * M) * dMdi + dp_cdi * (t_s - p) + dt_sdi * (p_c - p); //df/dintnl

  // (4) d(flowPotential[i])/d(stress_params[j])
  //      The non associative flow rule function is Q = phi * q^2 + (p- t_s) * (p - p_c)
  //      See eq. 4.2 of "Cam-clay pasticity, Part I: Implicit integration of elasto-plastic
  //      consitutive relations" of Ronaldo I. BORJA and Seung R. LEE (1990)
  //      
  //      Of course, when phi --> 1/M^2, the flow rule is associated. 
  //      
  //      In this class, phi is _NAfactor
  //

  all_q[0].dg[0] = 2 * p - (p_c + t_s); //dQ/dp
  all_q[0].dg[1] = 2 * phi * q;  //dQ/dq

  // (5) d2(flowPotential[i])/d(stress_params[j])/d(intnl[k])
  all_q[0].d2g_di[0][0] = dp_cdi + dt_sdi; //d(dQ/dp)/dintnl
  all_q[0].d2g_di[1][0] = 0.0; //d(dQ/dq)/dintnl

  // (6) d2(flowPotential[i])/d(stress_params[j])/d(stress_params[k])
  all_q[0].d2g[0][0] = 2.0; //d(dQ/dp)/dp
  all_q[0].d2g[0][1] = 0.0; //d(dQ/dp)/dq
  all_q[0].d2g[1][0] = 0.0; //d(dQ/dq)/dp
  all_q[0].d2g[1][1] = 2 * phi; //d(dQ/dq)/dq


  //std::cout << "computeAllQV - NAfactor: " << phi << std::endl;

}

/** A.3 virtual void setIntnlValuesV -> Needs to be overriden
 * 
 * Hardening Equation 8.8 of "8. Cam Clay and Modified Cam Caly Material Models" Rocscience 2022: 
 *    (p_c)_n+1 = (p_c)_n * exp(\nu_n\cdot \Delta\epsilon^p_v\cdot p/(\lambda-\kappa))
 * where: 
 * • (p_c)_n:   current preconsolidation pressure
 * • (p_c)_n+1: updated pre-consolidation pressure
 * • \nu_n:     current specific volume (\nu = 1+e), e is the void ratio
 * • \Delta\epsilon^p_v:  increment of plastic volumetric strain
 * • p:                   current mean stress (p = tr(\sigma)/3) --> stress_params[0]
 * • \lambda:             Slope of the normal consolidation line (compressibility of the virgin loading)
 * • \kappa: Slope of the swelling line (compressibility for unloading/reloading)
 * 
 * The hardening is thus solely defined via the plastic volumetric strain. Therefore, the 
 * internal parameter should be the plastic volumetric strain which would be computed by
 * the difference between the elasticly computed mean stress (p_trial) and the current
 * mean stress (p_current). Such difference and the deformation associated would be part
 * of the plastc regime.  
 */
void
ModifiedCamClayStressUpdate::setIntnlValuesV(const std::vector<Real> & trial_stress_params,
                                               const std::vector<Real> & current_stress_params,
                                               const std::vector<Real> & intnl_old,
                                               std::vector<Real> & intnl) const
{
  //std::cout << "setIntnlValuesV" << std::endl;
  // Extract the mean stress (p) from the current and trial stress states
  const Real p_current = current_stress_params[0]; // Mean stress (corrected)
  const Real p_trial = trial_stress_params[0]; 

  // Compute the plastic volumetric strain increment
  const Real delta_eps_vp = -(p_trial - p_current) / _K; // K is bulk modulus (constant)

  // Update the internal parameter (plastic volumetric strain)
  intnl[0] = intnl_old[0] + delta_eps_vp;
}

/**
 * A.4 virtual void setIntnlDerivativesV -> Needs to be overriden
 */
void
ModifiedCamClayStressUpdate::setIntnlDerivativesV(const std::vector<Real> & /*trial_stress_params*/,
                                                    const std::vector<Real> & /*current_stress_params*/,
                                                    const std::vector<Real> & /*intnl*/,
                                                    std::vector<std::vector<Real>> & dintnl) const
{
  //std::cout << "setIntnlDerivativesV" << std::endl;
  //std::cout << "setIntnlDerivativesV - K: " << _K << std::endl;
  dintnl[0][0] = 1/_K; //  dintnl/dp
  dintnl[0][1] = 0.0; //      dintnl/dq
}

/** A.5 STRESS PAREMETERS
 * 
  * To my understanding, stress parameters are the variables derived from the 
  * stress state to describe the plastic behavior. 
  * In such fashion, the stress parameters of the Modified Cam-Clay plastic model
  * the mean stress, p, and the equivalent deviatoric stress/von Mises stress, q. 
  * Their equations are: 
  * • p = tr(σ)/3; 
  * • q = sqrt(3/2 * s:s), where s = σ - p * I, with the identity matrix. 
  * 
  * In MOOSE, we have: 
  * • stress.trace() = tr(σ) = σ_11 + σ_22 + σ_33
  * • stress.secondInvariant() = 1/2 * s:s
  * Therefore, the two stress parameters are: 
  * • p = tr(σ)/3 = stress.trace()/3; 
  * • q = sqrt(3/2 * s:s) = sqrt(3*stress.secondInvariant()) = sqrt(3*J2)
  */
void
ModifiedCamClayStressUpdate::computeStressParams(const RankTwoTensor & stress, // no "const" on stress_params --> modifying it here
                                                   std::vector<Real> & stress_params) const 
{
  //std::cout << "computeStressParams" << std::endl;
  stress_params[0] = stress.trace()/3; // p
  stress_params[1] = std::sqrt(3*stress.secondInvariant()); //q
}

/** A.5.1 STRESS PARAMETERS DERIVATIVES
 * 
  * The two stress parameters are: 
  * • p = tr(σ)/3 = stress.trace()/3; 
  * • q = sqrt(3/2 * s:s) = sqrt(3*stress.secondInvariant()) = sqrt(3*J2)
  * 
  * Their derivatives with respect to the stress: 
  * • dp/dσ = stress.dtrace()/3; 
  * • dq/dσ = 1/2*sqrt(3/J2) * dJ2/dσ
 */
std::vector<RankTwoTensor>
ModifiedCamClayStressUpdate::dstress_param_dstress(const RankTwoTensor & stress) const
{
  //std::cout << "dstress_param_dstress - IN" << std::endl;
  std::vector<RankTwoTensor> dsp(2);
  dsp[0] = stress.dtrace()/3; //dp/ds
  dsp[1] = (stress.secondInvariant() > _numerical_zero) ? 
    (0.5 * std::sqrt(3 / stress.secondInvariant()) * stress.dsecondInvariant()) : RankTwoTensor(); //dq/ds
  //dsp[1] = 1/2 * std::sqrt(3/stress.secondInvariant()) * stress.dsecondInvariant();
  
  //std::cout << "dstress_param_dstress - OUT" << std::endl;
  return dsp; 
}

/** A.5.2 STRESS PARAMETERS SECOND DERIVATIVES
 * 
  * The two stress parameters are: 
  * • p = tr(σ)/3 = stress.trace()/3; 
  * • q = sqrt(3/2 * s:s) = sqrt(3*stress.secondInvariant()) = sqrt(3*J2)
  * 
  * Their derivatives with respect to the stress: 
  * • dp/dσ = stress.dtrace()/3; 
  * • dq/dσ = 1/2*sqrt(3/J2) * dJ2/dσ
  * 
  * Their second derivatives with respect to the stress: 
  * • d^2p/dσ^2 = 0; 
  * • d^2/dσ^2 = sqrt(3)/2*(-0.5/J2^(3/2)*(dJ2/dσ)^2 + 1/sqrt(J2) * d^2J2/dσ^2)
 */
std::vector<RankFourTensor>
ModifiedCamClayStressUpdate::d2stress_param_dstress(const RankTwoTensor & stress) const
{
  //std::cout << "d2stress_param_dstress" << std::endl;
  std::vector<RankFourTensor> d2sp(2);
  d2sp[0] = RankFourTensor(); 
  const Real j2 = stress.secondInvariant();
  const RankTwoTensor dj2 = stress.dsecondInvariant();
  //d2sp[1] = 0.5*std::sqrt(3)*(-0.5/std::pow(j2, 1.5)*dj2.outerProduct(dj2) + 1/std::sqrt(j2)*stress.d2secondInvariant());
  if (j2 > _numerical_zero)
    d2sp[1] = 0.5 * std::sqrt(3) * (-0.5 / std::pow(j2, 1.5) * dj2.outerProduct(dj2) + 1 / std::sqrt(j2) * stress.d2secondInvariant());
  else
    d2sp[1] = RankFourTensor();

  return d2sp;
}

/**
 * A.6 virtual void setStressAfterReturnV --> Need to be overriden
 * 
 */
void
ModifiedCamClayStressUpdate::setStressAfterReturnV(const RankTwoTensor & stress_trial,
                                                  const std::vector<Real> & stress_params,
                                                  Real /*gaE*/,
                                                  const std::vector<Real> & /*intnl*/,
                                                  const yieldAndFlow & /*smoothed_q*/,
                                                  const RankFourTensor & /*Eijkl*/,
                                                  RankTwoTensor & stress) const
{
  //std::cout << "setStressAfterReturnV" << std::endl;
  const Real p_trial = stress_trial.trace()/3;
  const Real q_trial = std::sqrt(3 * stress_trial.secondInvariant());
  Real multiplier = 1.0;
  if (q_trial > 0.0)
    multiplier = stress_params[1]/q_trial;
  stress = multiplier * stress_trial - multiplier * p_trial * RankTwoTensor(RankTwoTensor::initIdentity) + stress_params[0] * RankTwoTensor(RankTwoTensor::initIdentity); 

  // Step 1: Compute deviatoric stress tensor from the trial stress
  // RankTwoTensor deviatoric_trial = stress_trial;
  // deviatoric_trial.scaledAdd(RankTwoTensor(RankTwoTensor::initIdentity), -stress_trial.trace() / 3.0);

  // Step 2: Scale the deviatoric stress tensor to match q_ok
  // const Real q_trial = std::sqrt(3 * stress_trial.secondInvariant());
  // RankTwoTensor deviatoric_corrected = deviatoric_trial;
  // if (q_trial > 0.0)
  //  deviatoric_corrected *= stress_params[1] / q_trial; // Scale to achieve q_ok

  // Step 3: Combine deviatoric and mean stress components
  // stress = deviatoric_corrected + RankTwoTensor(RankTwoTensor::initIdentity) * (stress_params[0] / 3.0);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////             B            /////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////             C            /////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void
ModifiedCamClayStressUpdate::setEffectiveElasticity(const RankFourTensor & Eijkl)
{
  // Verify isotropy
  if (!Eijkl.isIsotropic())
    mooseError("Material requires isotropic elasticity tensor");

  // Get elastic parameters
  _mu = ElasticityTensorTools::getIsotropicShearModulus(Eijkl); // G
  _K = ElasticityTensorTools::getIsotropicBulkModulus(Eijkl);   // K

  // Volumetric projection (p = -K*ε_vol)
  _Eij[0][0] = 9.0 * _K;  // 9 comes from full tensor contraction

  // Deviatoric projection (q = √(3J₂) = 3G*ε_s)
  // Key change: Match your q definition with factor of √3
  _Eij[1][1] = 3.0 * _mu; // Now correctly relates to √(3J₂)

  // No pressure-deviatoric coupling for isotropic materials
  _Eij[0][1] = _Eij[1][0] = 0.0; 

  // Normalizing factor (matches pressure units)
  _En = _K; 

  // Compute compliance matrix (analytic inverse)
  const Real inv_det = 1.0/(_Eij[0][0] * _Eij[1][1]);
  _Cij[0][0] = inv_det * _Eij[1][1];  // 1/(9K)
  _Cij[1][1] = inv_det * _Eij[0][0];  // 1/(3G)
  _Cij[0][1] = _Cij[1][0] = 0.0;
  
  /*
  //std::cout << "setEffectiveElasticity" << std::endl;
  //std::cout << "setEffectiveElasticity - _num_sp: " << _num_sp << std::endl;
  // Verify isotropy (optional but recommended)
  if (!Eijkl.isIsotropic())
    mooseError("Material requires isotropic elasticity tensor");

  const Real E = ElasticityTensorTools::getIsotropicYoungsModulus(Eijkl);
  const Real nu = ElasticityTensorTools::getIsotropicPoissonsRatio(Eijkl);
  // Get Lamé parameters directly from tensor
  const Real lambda = Eijkl(0,0,1,1); // C12 component
  _mu = Eijkl(0,1,0,1);      // C44 component (shear modulus)
  _K = lambda + (2.0/3.0)*_mu; // Bulk modulus

  //std::cout << "setEffectiveElasticity - E: " << E << std::endl;
  //std::cout << "setEffectiveElasticity - nu: " << nu << std::endl;
  //std::cout << "setEffectiveElasticity - K: " << _K << std::endl;
  //std::cout << "setEffectiveElasticity - mu: " << _mu << std::endl;
  // Pressure projection (sum of all components)
  _Eij[0][0] = Eijkl.sum3x3(); // 9*K for isotropic materials
  
  // Shear projection (using 01-01 component)
  _Eij[1][1] = 2 * _mu; // 2*mu for isotropic
  
  // Cross terms (zero for most models)
  _Eij[0][1] = _Eij[1][0] = 0.0;
  
  // Normalizing factor (often the last diagonal term)
  _En = _Eij[1][1]; // Or another appropriate scaling
  
  // Compute compliance matrix
  const Real det = _Eij[0][0]*_Eij[1][1] - _Eij[0][1]*_Eij[1][0];
  _Cij[0][0] = _Eij[1][1]/det;
  _Cij[1][1] = _Eij[0][0]/det;
  _Cij[0][1] = _Cij[1][0] = -_Eij[0][1]/det;
  */
  



  // Debug output
  if (false)
  {
    Moose::out << "Isotropic Stiffness Matrix (Voigt):\n";
    //printMatrix(_Eij);
    Moose::out << "Isotropic Compliance Matrix (Voigt):\n";
    //printMatrix(_Cij);

    //std::cout << "setEffectiveElasticity - Eij: " << std::endl;
    for (int i = 0; i < _num_sp; ++i) { // Printing _Eij
      for (int j = 0; j < _num_sp; ++j) {
          //std::cout << std::setw(5) << _Eij[i][j] << " "; // Use setw for alignment (optional)
      }
      //std::cout << std::endl; // Newline after each row
    }

    //std::cout << "setEffectiveElasticity - En: " << _En << std::endl;

    //std::cout << "setEffectiveElasticity - Cij: " << std::endl;
    for (int i = 0; i < _num_sp; ++i) { // Printing _Cij
      for (int j = 0; j < _num_sp; ++j) {
          //std::cout << std::setw(5) << _Cij[i][j] << " "; // Use setw for alignment (optional)
      }
      //std::cout << std::endl; // Newline after each row
    }
  }
}


/**
 * 
void
ModifiedCamClayStressUpdate::setEffectiveElasticity(const RankFourTensor & Eijkl)
{
  //std::cout << "setEffectiveElasticity" << std::endl;
  Real E = ElasticityTensorTools::getIsotropicYoungsModulus(Eijkl);
  Real nu = ElasticityTensorTools::getIsotropicPoissonsRatio(Eijkl);

  _K = E / (3.0 * (1.0 - 2.0 * nu));
  _mu = E / (2.0 * (1.0 + nu));


  //std::cout << "setEffectiveElasticity - E: " << E << std::endl;
  //std::cout << "setEffectiveElasticity - nu: " << nu << std::endl;
  //std::cout << "setEffectiveElasticity - K: " << _K << std::endl;
  //std::cout << "setEffectiveElasticity - mu: " << _mu << std::endl;

  // Construct the reduced elasticity matrix _Eij (6x6 Voigt notation).
  _Eij.resize(6, std::vector<Real>(6, 0.0)); // Correct: Ensures all values are initialized to 0



  // Construct the reduced elasticity matrix _Eij (6x6 Voigt notation).
  _Eij[0][0] = (1.0 - nu) * E / ((1.0 + nu) * (1.0 - 2.0 * nu));
  _Eij[0][1] = nu * E / ((1.0 + nu) * (1.0 - 2.0 * nu));
  _Eij[0][2] = _Eij[0][1]; // Symmetry
  _Eij[1][0] = _Eij[0][1];
  _Eij[1][1] = _Eij[0][0];
  _Eij[1][2] = _Eij[0][1];
  _Eij[2][0] = _Eij[0][2];
  _Eij[2][1] = _Eij[1][2];
  _Eij[2][2] = _Eij[0][0];
  _Eij[3][3] = _mu;
  _Eij[4][4] = _mu;
  _Eij[5][5] = _mu;


  //std::cout << "setEffectiveElasticity - Eij: " << std::endl;
  for (int i = 0; i < 6; ++i) { // Printing _Eij
    for (int j = 0; j < 6; ++j) {
        //std::cout << std::setw(5) << _Eij[i][j] << " "; // Use setw for alignment (optional)
    }
    //std::cout << std::endl; // Newline after each row
  }

  // Calculate _En (SCALAR value - corrected!)
  _En = (3*_K*(1-2*nu))/(1+nu); // Corrected: Using the appropriate formula for isotropic materials.

  //std::cout << "setEffectiveElasticity - En: " << _En << std::endl;

  // Calculate _Cij (tangent stiffness - placeholder)
  // You MUST replace this with the correct calculation based on your
  // Modified Cam-Clay formulation.  It should be related to _Eij
  // in the elastic predictor step.
  //std::cout << "setEffectiveElasticity - Cij: " << std::endl;
  _Cij.resize(6, std::vector<Real>(6, 0.0)); // Correct: Size both dimensions!
  for (int i = 0; i < 6; ++i) {
    for (int j = 0; j < 6; ++j) {
      _Cij[i][j] = _Eij[i][j]; // Placeholder
    }
  }
  for (int i = 0; i < 6; ++i) { // Printing _Cij
    for (int j = 0; j < 6; ++j) {
        //std::cout << std::setw(5) << _Eij[i][j] << " "; // Use setw for alignment (optional)
    }
    //std::cout << std::endl; // Newline after each row
  }
}
 */

void
ModifiedCamClayStressUpdate::preReturnMapV(
    const std::vector<Real> & /*trial_stress_params*/,
    const RankTwoTensor & /*stress_trial*/,
    const std::vector<Real> & /*intnl_old*/,
    const std::vector<Real> & /*yf*/,
    const RankFourTensor & Eijkl)
{
  //std::cout << "preReturnMap" << std::endl;

  Real E = ElasticityTensorTools::getIsotropicYoungsModulus(Eijkl);
  Real nu = ElasticityTensorTools::getIsotropicPoissonsRatio(Eijkl);

  //std::cout << "preReturnMap - E: " << E << std::endl;
  //std::cout << "preReturnMap - nu: " << nu << std::endl;
  
  _K = E / (3.0 * (1.0 - 2.0 * nu));
  _mu = E / (2.0 * (1.0 + nu));

  //std::cout << "preReturnMap - K: " << _K << std::endl;
  //std::cout << "preReturnMap - mu: " << _mu << std::endl;
  // Definition of the parameter for the non associated flow rule: NAfactor
  // When NAfactor = 1/M^2, the flow rule is associated, see below
  if (_associated)
    _NAfactor = 1/(_M * _M); 
  else
    _NAfactor = _K/(3 * _mu); 
}

