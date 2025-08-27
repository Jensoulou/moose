[Mesh]
    [gen]
        type = GeneratedMeshGenerator
        dim = 2
        nx = 2
        xmax = 2
        ny = 2
        ymax = 2
        elem_type = QUAD4
    []
    [./subdomain_id]
        input = gen
        type = SubdomainBoundingBoxGenerator
        bottom_left = '1 0 0'
        top_right = '2 2 0'
        block_id = 1
    [../]
    [./interface]
        type = SideSetsBetweenSubdomainsGenerator
        input = subdomain_id
        primary_block = '0'
        paired_block = '1'
        new_boundary = 'interface'
    [../]
    [fracture]
        type = LowerDBlockFromSidesetGenerator
        sidesets = 'interface'
        new_block_id = 2
        new_block_name = 'fracture'
        input = interface
    []
[]

[Variables]
    [./u]
        block = 0
    [../]
    [./v]
        block = 1
    [../]
    [./w]
        block = 2
    [../]
[]

[Kernels]
    [./diff_u]
        type = MaterialPropertyValue
        prop_name = 'diffusivity'
        variable = u
        block = 0
    [../]
    [./diff_v]
        type = MaterialPropertyValue
        variable = v
        prop_name = 'diffusivity'
        block = 1
    [../]
    [./diff_w]
        type = MaterialPropertyValue
        variable = w
        prop_name = 'diffusivity'
        block = 2
    [../]
[]

[BCs]
    [u_left]
        type = DirichletBC
        boundary = 'left'
        variable = u
        value = 1
    []
    [v_right]
        type = DirichletBC
        boundary = 'right'
        variable = v
        value = 2
    []
[]


[Materials]
    [./stateful1]
        type = StatefulMaterial
        block = 0
        initial_diffusivity = 1
        # outputs = all
    [../]
    [./stateful2]
        type = StatefulMaterial
        block = 1
        initial_diffusivity = 2
        # outputs = all
    [../]
    [./stateful3]
        type = StatefulMaterial
        block = 2
        initial_diffusivity = 3
        # outputs = all
    [../]
    [./interface_material_avg]
        type = InterfaceValueMaterial
        mat_prop_primary = diffusivity
        mat_prop_secondary = diffusivity
        var_primary = diffusivity_var
        var_secondary = diffusivity_var
        mat_prop_out_basename = diff
        boundary = interface #_boundary
        interface_value_type = secondary
        mat_prop_var_out_basename = diff_var
        nl_var_primary = u
        nl_var_secondary = w
    [../]
[]

# [InterfaceKernels]
#   [tied]
#     type = PenaltyInterfaceDiffusion
#     variable = u
#     neighbor_var = w
#     jump_prop_name = "primary_jump"
#     penalty = 1
#     boundary = 'interface'
#   []
# []

[AuxVariables]
    [diffusivity_var]
        family = MONOMIAL
        order = CONSTANT
    []
    [diffusivity]
        family = MONOMIAL
        order = CONSTANT
    []
    [diffusivity_secondary]
        family = MONOMIAL
        order = CONSTANT
    []
[]

[AuxKernels]
    [./interface_material_secondary]
        type = MaterialRealAux
        property = diff_secondary
        variable = diffusivity_var
        boundary = interface #_boundary
    []
    [./difu]
        type = MaterialRealAux
        property = 'diffusivity'
        variable = diffusivity
    []
    # [./diffu_primary]
    #     type = MaterialRealAux
    #     property = 'diff_var_primary'
    #     variable = diffusivity_primary
    # []
[]

[Executioner]
  type = Transient
  solve_type = NEWTON
  end_time = 10
  dt = 1
[]

[Outputs]
  exodus = true
[]