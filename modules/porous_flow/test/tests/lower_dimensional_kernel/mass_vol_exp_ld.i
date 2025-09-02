# 0phase heat conduction.
# apply a boundary condition of T=300 to a bar that
# is initially at T=200, and observe the expected
# error-function response
[Mesh]
    [gen]
        type = GeneratedMeshGenerator
        dim = 2
        nx = 4
        ny = 4
        xmin = 0
        xmax = 10
        ymin = 0
        ymax = 10
    []
    [sub]
        type = SubdomainBoundingBoxGenerator
        input = gen
        block_id = 1
        bottom_left = '0 0 0'
        top_right = '5 10 0'
    []
    [interface]
        type = SideSetsBetweenSubdomainsGenerator
        input = sub
        primary_block = '0'
        paired_block = '1'
        new_boundary = 'interface'
    []
    [fracture]
        type = LowerDBlockFromSidesetGenerator
        sidesets = 'interface'
        new_block_id = 2
        new_block_name = 'fracture'
        input = interface
    []
[]

[GlobalParams]
    displacements = 'disp_x disp_y'
    PorousFlowDictator = dictator
    # gravity = '0 0 0'
[]

[Variables]
    [disp_x]
    []
    [disp_y]
    []
    [pp]
    []
[]

# [ICs]
#     [pp]
#         type = FunctionIC
#         variable = pp
#         function = '10-y'
#     []
# []

[AuxVariables]
    [a]
        order = CONSTANT
        family = MONOMIAL
        initial_condition = 10.0
    []
    [AuxV_A] 
        # PorousFlow_volumetric_strain_rate_qp]
        family = MONOMIAL
        order = CONSTANT
    []
    [AuxV_B]
        # dPorousFlow_volumetric_strain_rate_qp_dvar]
        family = MONOMIAL
        order = CONSTANT
    []
    [AuxV_A_ld] 
        # PorousFlow_volumetric_strain_rate_qp_ld]
        family = MONOMIAL
        order = CONSTANT
    []
    [AuxV_B_ld]
        # dPorousFlow_volumetric_strain_rate_qp_dvar_ld]
        family = MONOMIAL
        order = CONSTANT
    []
    [AuxV_C]
        # interface_PorousFlow_volumetric_strain_rate_qp]
        family = MONOMIAL
        order = CONSTANT
    []
    [AuxV_D]
        # interface_dPorousFlow_volumetric_strain_rate_qp_dvar]
        family = MONOMIAL
        order = CONSTANT
    []
[]

[AuxKernels]
    [AuxK_A]
        # PorousFlow_volumetric_strain_rate_qp]
        type = MaterialRealAux
        property = PorousFlow_volumetric_strain_rate_qp
        variable = AuxV_A
        # block = '0 1'
    []
    [AuxK_A]
        # PorousFlow_volumetric_strain_rate_qp]
        type = MaterialRealAux
        property = PorousFlow_volumetric_strain_rate_qp
        variable = AuxV_A
        # block = '0 1'
    []
    [AuxK_B]
        # dPorousFlow_volumetric_strain_rate_qp_dvar]
        type = MaterialStdVectorRealGradientAux
        variable = AuxV_B
        # index = 0
        # component = 0
        property = dPorousFlow_volumetric_strain_rate_qp_dvar
        # block = 2
    []
    # [AuxK_A_ld]
    #     # PorousFlow_volumetric_strain_rate_qp]
    #     type = MaterialRealAux
    #     property = PorousFlow_volumetric_strain_rate_qp_ld
    #     variable = AuxV_A_ld
    #     block = '2'
    # []
    # [AuxK_B_ld]
    #     # dPorousFlow_volumetric_strain_rate_qp_dvar]
    #     type = MaterialStdVectorRealGradientAux
    #     variable = AuxV_B_ld
    #     # index = 0
    #     # component = 0
    #     property = dPorousFlow_volumetric_strain_rate_qp_dvar_ld
    #     block = '2'
    # []
    # [AuxK_C]
    #     # interface_PorousFlow_volumetric_strain_rate_qp]
    #     type = MaterialRealAux
    #     property = interface_PorousFlow_volumetric_strain_rate_qp_primary
    #     variable = AuxV_C
    #     boundary = 'interface'
    #     check_boundary_restricted = false
    # []
    # [AuxK_D]
    #     # interface_dPorousFlow_volumetric_strain_rate_qp_dvar]
    #     type = MaterialRealAux
    #     property = interface_dPorousFlow_volumetric_strain_rate_qp_dvar_primary
    #     variable = AuxV_D #interface_dPorousFlow_volumetric_strain_rate_qp_dvar
    #     boundary = 'interface'
    #     check_boundary_restricted = false
    # []
[]

[Kernels]
    [mass_time]
        type = PorousFlowMassTimeDerivative
        fluid_component = 0
        variable = pp
        # block = '0 1'
    []
    [mass_dot]
        type = PorousFlowMassVolumetricExpansion
        fluid_component = 0
        variable = pp
        block = '0 1'
    []
    [mass_dot_lower_dim]
        type = PorousFlowMassVolumetricExpansionLowerDimensional
        fluid_component = 0
        variable = pp
        # aperture = a
        block = 2
    []
    [grad_stress_x]
        type = StressDivergenceTensors
        variable = disp_x
        displacements = 'disp_x disp_y'
        component = 0
    []
    [grad_stress_y]
        type = StressDivergenceTensors
        variable = disp_y
        displacements = 'disp_x disp_y'
        component = 1
    []
[]

[UserObjects]
    [dictator]
        type = PorousFlowDictator
        porous_flow_vars = 'pp disp_x disp_y'
        number_fluid_phases = 1
        number_fluid_components = 1
    []
    [pc]
        type = PorousFlowCapillaryPressureVG
        m = 0.6
        alpha = 1.3
    []
[]

[FluidProperties]
    [simple_fluid]
        type = SimpleFluidProperties
        bulk_modulus = 100
        density0 = 1000
        viscosity = 4.4
        thermal_expansion = 0
        cv = 2
    []
[]

[Materials]
    [temperature]
        type = PorousFlowTemperature
    []
    [porosity]
        type = PorousFlowPorosityConst
        porosity = 0.2
    []
    [rock_heat]
        type = PorousFlowMatrixInternalEnergy
        specific_heat_capacity = 1.0
        density = 125
    []
    [simple_fluid]
        type = PorousFlowSingleComponentFluid
        fp = simple_fluid
        phase = 0
    []
    # [permeability]
    #     type = PorousFlowPermeabilityConst
    #     permeability = '1.1 0 0 0 2 0 0 0 3'
    # []
    # [relperm]
    #     type = PorousFlowRelativePermeabilityCorey
    #     n = 2
    #     phase = 0
    # []
    [massfrac]
        type = PorousFlowMassFraction
    []
    [PS]
        type = PorousFlow1PhaseP
        porepressure = pp
        capillary_pressure = pc
    []

    [elasticity_tensor]
        type = ComputeElasticityTensor
        C_ijkl = '2 3'
        fill_method = symmetric_isotropic
    []
    [strain]
        type = ComputeSmallStrain
    []
    [stress]
        type = ComputeLinearElasticStress
    []
    [vol_strain]
        type = PorousFlowVolumetricStrain
        # block = '0 1'
    []
    [CopyPaste_VolStrain]
        type = CopyPasteMaterial
        prop_names = 'PorousFlow_volumetric_strain_rate_qp_ld'
        neighbor_prop_names = 'PorousFlow_volumetric_strain_rate_qp'
        block = 2
    []
    [CopyPaste_dVolStrain_dvar]
        type = CopyPasteMaterialVecVec
        prop_names = 'dPorousFlow_volumetric_strain_rate_qp_dvar_ld'
        neighbor_prop_names = 'dPorousFlow_volumetric_strain_rate_qp_dvar'
        block = 2
    []
    # [vol_strain_rate_1D]
    #     type = InterfaceValueMaterial_test
    #     mat_prop_primary = 'PorousFlow_volumetric_strain_rate_qp'
    #     #need_gradient = true
    #     boundary = interface
    #     # block = 3
    # []
    # [dvol_strain_dvar_1D]
    #     type = InterfaceValueMaterial_test_gradient
    #     mat_prop_primary = 'dPorousFlow_volumetric_strain_rate_qp_dvar'
    #     boundary = interface
    #     # block = 3
    # []
    [interface_material_GHOSTVolStrain]
        type = InterfaceValueMaterial
        mat_prop_primary = PorousFlow_volumetric_strain_rate_qp
        mat_prop_secondary = PorousFlow_volumetric_strain_rate_qp
        var_primary = pp
        var_secondary = pp
        mat_prop_out_basename = interface_PorousFlow_volumetric_strain_rate_qp
        boundary = interface
        interface_value_type = primary
        mat_prop_var_out_basename = interface_PorousFlow_volumetric_strain_rate_qp_var
        nl_var_primary = pp
        nl_var_secondary = pp
    []
    # [interface_material_GHOSTDVolStrainDVar]
    #     type = InterfaceValueMaterial
    #     mat_prop_primary = dPorousFlow_volumetric_strain_rate_qp_dvar
    #     mat_prop_secondary = dPorousFlow_volumetric_strain_rate_qp_dvar
    #     var_primary = pp
    #     var_secondary = pp
    #     mat_prop_out_basename = interface_dPorousFlow_volumetric_strain_rate_qp_dvar
    #     boundary = interface
    #     interface_value_type = primary
    #     mat_prop_var_out_basename = interface_dPorousFlow_volumetric_strain_rate_qp_dvar_var
    #     nl_var_primary = pp
    #     nl_var_secondary = pp
    # []
[]

[BCs]
    [pp0]
        type = DirichletBC
        variable = pp
        boundary = bottom
        value = 10
    []
    [pp1]
        type = DirichletBC
        variable = pp
        boundary = top
        value = 0
    []
    [disp_bot]
        type = DirichletBC
        variable = disp_y
        boundary = bottom
        value = 0
    []
    [disp_sides]
        type = DirichletBC
        variable = disp_x
        boundary = 'left'# right'
        value = 0
    []
    # [disp_top]
    #     type = FunctionDirichletBC
    #     variable = disp_y
    #     boundary = 'top'
    #     function = '-t'
    # []
    [disp_right]
        type = FunctionDirichletBC
        variable = disp_x
        boundary = 'right'
        function = '-t'
    []
[]


[Preconditioning]
    [andy]
        type = SMP
        full = true
    []
[]

[Executioner]
    type = Transient
    solve_type = Newton
    dt = 0.5
    end_time = 0.5
[]

[Postprocessors]
    [pp_mid_fracture]
        type = NodalVariableValue
        variable = pp
        nodeid = 12
    []
    [pp_mid_block0]
        type = NodalVariableValue
        variable = pp
        nodeid = 13
    []
    [pp_mid_block1]
        type = NodalVariableValue
        variable = pp
        nodeid = 10
    []
    [disp_x_mid_fracture]
        type = NodalVariableValue
        variable = disp_x
        nodeid = 12
    []
    [disp_x_mid_block0]
        type = NodalVariableValue
        variable = disp_x
        nodeid = 13
    []
    [disp_x_mid_block1]
        type = NodalVariableValue
        variable = disp_x
        nodeid = 10
    []
    [disp_y_mid_fracture]
        type = NodalVariableValue
        variable = disp_y
        nodeid = 12
    []
    [disp_y_mid_block0]
        type = NodalVariableValue
        variable = disp_y
        nodeid = 13
    []
    [disp_y_mid_block1]
        type = NodalVariableValue
        variable = disp_y
        nodeid = 10
    []
[]

[Outputs]
  #file_base = 'test/tests/lower_dimensional_kernel/mass_vol_exp_ld' #'lower_dimensional_kernel/mass_vol_exp_ld'
  [csv]
    type = CSV
    file_base = mass_vol_exp_ld
  []
  exodus = true
[]
