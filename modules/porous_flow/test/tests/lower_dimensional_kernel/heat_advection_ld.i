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
    gravity = '0 0 0'
[]

[Variables]
    [temp]
        initial_condition = 200
    []
    [pp]
    []
[]

[ICs]
    [pp]
        type = FunctionIC
        variable = pp
        function = '10-y'
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
    [mass_dot]
        type = PorousFlowMassTimeDerivative
        fluid_component = 0
        variable = pp
    []
    [advection]
        type = PorousFlowAdvectiveFlux
        fluid_component = 0
        variable = pp
    []
    [energy_dot]
        type = PorousFlowEnergyTimeDerivative
        variable = temp
    []
    [heat_advection]
        type = PorousFlowHeatAdvection
        variable = temp
        block = '0 1'
    []
    [heat_advection_lower_dim]
        type = PorousFlowHeatAdvectionLowerDimensional
        variable = temp
        aperture = a
        block = 3
    []
[]

[UserObjects]
    [dictator]
        type = PorousFlowDictator
        porous_flow_vars = 'temp pp'
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
        temperature = temp
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
    [permeability]
        type = PorousFlowPermeabilityConst
        permeability = '1.1 0 0 0 2 0 0 0 3'
    []
    [relperm]
        type = PorousFlowRelativePermeabilityCorey
        n = 2
        phase = 0
    []
    [massfrac]
        type = PorousFlowMassFraction
    []
    [PS]
        type = PorousFlow1PhaseP
        porepressure = pp
        capillary_pressure = pc
    []
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
    [spit_heat]
        type = DirichletBC
        variable = temp
        boundary = bottom
        value = 300
    []
    [suck_heat]
        type = DirichletBC
        variable = temp
        boundary = top
        value = 200
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
    file_base = heat_advection_ld
  []
  exodus = true
[]
