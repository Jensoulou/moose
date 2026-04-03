# Test that porosity is correctly calculated.
# Porosity = biot + (phi0 - biot) * exp(-vol_strain + (biot - 1) / solid_bulk * (porepressure - ref_pressure) + thermal_exp_coeff * (temperature - ref_temperature))
# The parameters used are:
# biot = 0.7
# phi0 = 0.5
# vol_strain = 0.5
# solid_bulk = 0.3
# porepressure = 2
# ref_pressure = 3
# thermal_exp_coeff = 0.5
# temperature = 4
# ref_temperature = 3.5
# which yield porosity = 0.276599996677
[Mesh]
  type = GeneratedMesh
  dim = 3
[]

[GlobalParams]
  PorousFlowDictator = dictator
  displacements = 'disp_x disp_y disp_z'
  biot_coefficient = 0.7
[]

[Variables]
  [porepressure]
    initial_condition = 2
  []
  [temperature]
    initial_condition = 4
  []
  [disp_x]
  []
  [disp_y]
  []
  [disp_z]
  []
[]

[ICs]
  [disp_x]
    type = FunctionIC
    function = '0.5 * x'
    variable = disp_x
  []
[]

[Kernels]
  [dummy_p]
    type = TimeDerivative
    variable = porepressure
  []
  [dummy_t]
    type = TimeDerivative
    variable = temperature
  []
  [dummy_x]
    type = TimeDerivative
    variable = disp_x
  []
  [dummy_y]
    type = TimeDerivative
    variable = disp_y
  []
  [dummy_z]
    type = TimeDerivative
    variable = disp_z
  []
[]

[AuxVariables]
  [porosity]
    order = CONSTANT
    family = MONOMIAL
  []
    [AuxV_DarcyFlow_x]
        order = CONSTANT
        family = MONOMIAL
    []    
    [AuxV_DarcyFlow_y]
        order = CONSTANT
        family = MONOMIAL
    []     
    [AuxV_DarcyFlow_z]
        order = CONSTANT
        family = MONOMIAL
    []  
    [AuxV_temperature]
        # order = CONSTANT
        family = MONOMIAL
    []
    [AuxV_grad_temperature_x]
        family = MONOMIAL
    []
    [AuxV_grad_temperature_y]
        family = MONOMIAL
    []
    [AuxV_grad_temperature_z]
        family = MONOMIAL
    []
    [AuxV_V_grad_temperature]
        family = MONOMIAL
    []
    [AuxV_viscosity_H2O_liq]
        order = CONSTANT
        family = MONOMIAL
    []
    [AuxV_density_H2O_liq]
        order = CONSTANT
        family = MONOMIAL
    []
    # Temperature related 
    [AuxV_int_V_grad_temperature]
        family = MONOMIAL
    []
[]

[AuxKernels]
  [porosity]
    type = PorousFlowPropertyAux
    property = porosity
    variable = porosity
  []
    [AuxK_density_H2O_liq]
        type = PorousFlowPropertyAux
        variable = AuxV_density_H2O_liq
        property = density
        phase = 0
        execute_on = 'initial timestep_end'
    []
    [AuxK_viscosity_H2O_liq]
        type = PorousFlowPropertyAux
        variable = AuxV_viscosity_H2O_liq
        property = viscosity
        phase = 0
        execute_on = 'initial timestep_end'
    []
    [AuxK_DarcyFlow_x]
        type = PorousFlowDarcyVelocityComponent
        variable = AuxV_DarcyFlow_x
        component = x
        gravity = '0 -9.81 0'
    []
    [AuxK_DarcyFlow_y]
        type = PorousFlowDarcyVelocityComponent
        variable = AuxV_DarcyFlow_y
        component = y
        gravity = '0 -9.81 0'
    []
    [AuxK_DarcyFlow_z]
        type = PorousFlowDarcyVelocityComponent
        variable = AuxV_DarcyFlow_z
        component = z
        gravity = '0 -9.81 0'
    []
    # Temperature, Grad T, V Grad T, Temporal integral of V Grad T
    [AuxK_temperature] # Temperature
        type = PorousFlowPropertyAux
        variable = AuxV_temperature
        property = temperature
        execute_on = 'initial timestep_end'
    []
    [AuxK_grad_temperature_x] # Grad T - x direction
        type = VariableGradientComponent
        variable = AuxV_grad_temperature_x
        component = x
        gradient_variable = AuxV_temperature
        execute_on = 'initial timestep_end'
    []
    [AuxK_grad_temperature_y] # Grad T - y direction
        type = VariableGradientComponent
        variable = AuxV_grad_temperature_y
        component = y
        gradient_variable = AuxV_temperature
        execute_on = 'initial timestep_end'
    []
    [AuxK_grad_temperature_z] # Grad T - z direction
        type = VariableGradientComponent
        variable = AuxV_grad_temperature_z
        component = z
        gradient_variable = AuxV_temperature
        execute_on = 'initial timestep_end'
    []
    [AuxK_V_grad_temperature] # Scalar product between Grad T and V
        type = ParsedAux
        coupled_variables = 'AuxV_grad_temperature_x AuxV_grad_temperature_y AuxV_grad_temperature_z AuxV_DarcyFlow_x AuxV_DarcyFlow_y AuxV_DarcyFlow_z'
        expression = 'AuxV_grad_temperature_x*AuxV_DarcyFlow_x+AuxV_grad_temperature_y*AuxV_DarcyFlow_y+AuxV_grad_temperature_z*AuxV_DarcyFlow_z'
        variable = 'AuxV_V_grad_temperature'
        execute_on = 'initial timestep_end'
    []
    [AuxK_int_V_grad_temperature] # Temporal integral of V Grad T
        type = VariableTimeIntegrationAux
        variable_to_integrate = AuxV_V_grad_temperature
        variable = AuxV_int_V_grad_temperature
        execute_on = 'initial timestep_end'
    []
[]

[Postprocessors]
  [porosity]
    type = PointValue
    variable = porosity
    point = '0 0 0'
  []
[]

[UserObjects]
  [dictator]
    type = PorousFlowDictator
    porous_flow_vars = 'porepressure temperature'
    number_fluid_phases = 1
    number_fluid_components = 1
  []
  [pc]
    type = PorousFlowCapillaryPressureConst
  []
[]

[FluidProperties]
  [simple_fluid]
    type = Water97FluidProperties
  []
[]

[Materials]
  [temperature]
    type = PorousFlowTemperature
    temperature = temperature
  []
  [eff_fluid_pressure]
    type = PorousFlowEffectiveFluidPressure
  []
  [total_strain]
    type = ComputeSmallStrain
  []
  [vol_strain]
    type = PorousFlowVolumetricStrain
  []
  [ppss]
    type = PorousFlow1PhaseP
    porepressure = porepressure
    capillary_pressure = pc
  []
  [porosity]
    type = PorousFlowPorosityAlteration
    mechanical = true
    fluid = true
    thermal = true
    alteration = true
    ensure_positive = false
    porosity_zero = 0.5
    solid_bulk = 0.3
    thermal_expansion_coeff = 0.5
    reference_porepressure = 3
    reference_temperature = 3.5
    alteration_index = AuxV_int_V_grad_temperature
  []
  [permeability]
    type = PorousFlowPermeabilityConst
    permeability = '1e-15 0 0 0 1e-15 0 0 0 1e-15'
  []
  [relperm0] # Taken from Andy
    type = PorousFlowRelativePermeabilityCorey
    n = 1
    s_res = 0.3
    sum_s_res = 0.3
    phase = 0
  []
[]

[Executioner]
  solve_type = Newton
  type = Transient
  num_steps = 1
[]

[Outputs]
  csv = true
[]
