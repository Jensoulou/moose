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
        new_block_id = 3
        new_block_name = 'fracture'
        input = interface
    []
[]

[GlobalParams]
    PorousFlowDictator = dictator
[]

[Variables]
    [temp]
        initial_condition = 200
    []
[]

[AuxVariables]
    [a]
        order = CONSTANT
        family = MONOMIAL
        initial_condition = 10.0
    []
[]

[Kernels]
    [energy_dot]
        type = PorousFlowEnergyTimeDerivative
        variable = temp
    []
    [heat_conduction]
        type = PorousFlowHeatConduction
        variable = temp
        block = '0 1'
    []
    [heat_conduction_lower_dim]
        type = PorousFlowHeatConductionLowerDimensional
        variable = temp
        aperture = a
        block = 3
    []
[]

[UserObjects]
    [dictator]
        type = PorousFlowDictator
        porous_flow_vars = 'temp'
        number_fluid_phases = 0
        number_fluid_components = 0
    []
[]

[Materials]
    [temperature]
        type = PorousFlowTemperature
        temperature = temp
    []
    [thermal_conductivity]
        type = PorousFlowThermalConductivityIdeal
        dry_thermal_conductivity = '2 0 0  0 2 0  0 0 2'
    []
    [porosity]
        type = PorousFlowPorosityConst
        porosity = 0.1
    []
    [rock_heat]
        type = PorousFlowMatrixInternalEnergy
        specific_heat_capacity = 2.2
        density = 0.5
    []
[]

[BCs]
    [bottom]
        type = DirichletBC
        boundary = bottom
        value = 300
        variable = temp
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
    [mid_fracture]
        type = NodalVariableValue
        variable = temp
        nodeid = 12
    []
    [mid_block0]
        type = NodalVariableValue
        variable = temp
        nodeid = 13
    []
[]

[Outputs]
  [csv]
    type = CSV
    file_base = heat_conduction_ld
  []
  exodus = true
[]
