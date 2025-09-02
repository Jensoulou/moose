interface = 'joint'
[Mesh]
    [gen]
        type = GeneratedMeshGenerator
        dim = 2
        nx = 4
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
        show_info = true
    [../]
    [./interface]
        type = SideSetsBetweenSubdomainsGenerator
        input = subdomain_id
        primary_block = '1'
        paired_block = '0'
        new_boundary = 'interface'
        show_info = true
    [../]
    [fracture]
        type = LowerDBlockFromSidesetGenerator
        sidesets = 'interface'
        new_block_id = 2
        new_block_name = 'fracture'
        input = interface
        show_info = true
    []
    [./interface2]
        type = SideSetsBetweenLowerDimAndHigherDim
        input = fracture
        included_subdomains = '2'
        included_neighbors = '0'
        excluded_boundaries = 'bottom right left top'
        new_boundary = 'joint'
        replace = true
        show_info = true
        #included_boundaries = 'interface'
    [../]
[]

# [MeshModifiers]
#   [sideset_updater]
#     type = SidesetAroundSubdomainUpdater
#     inner_subdomains = 'fracture'
#     outer_subdomains = '1'
#     update_boundary_name = interface
#     assign_outer_surface_sides = false
#   []
# []

[Variables]
    [./u]
    [../]
[]

[Kernels]
    [diff_t]
        type = TimeDerivative
        variable = u
    []
    [./diff_u]
        type = Diffusion
        variable = u
    [../]
[]

[BCs]
    [u_left]
        type = DirichletBC
        boundary = 'left'
        variable = u
        value = 1
    []
    [u_right]
        type = DirichletBC
        boundary = 'right'
        variable = u
        value = 2
    []
[]

[Functions]
    [fcn_mat_1]
        type = ParsedFunction
        expression = '6+y'
    []
    [fcn_mat_2]
        type = ParsedFunction
        expression = '20+y'
    []
[]

[Materials]
    [mat1]
        type = GenericFunctionMaterial
        prop_names = mat
        prop_values = fcn_mat_1
        block = 0
    []
    [mat2]
        type = GenericFunctionMaterial
        prop_names = mat
        prop_values = fcn_mat_2
        block = 1
    []
    [mat3]
        type = GenericConstantMaterial
        prop_names = mat
        prop_values = 30
        block = 2
    []
    [mat4]
        type = CopyPasteMaterial
        prop_names = 'mat2'
        neighbor_prop_names = 'mat'
        # prop_values = 50
        block = 2
    []
    [./interface_material_prim]
        type = InterfaceValueMaterial
        mat_prop_primary = mat
        mat_prop_secondary = mat
        var_primary = u
        var_secondary = u
        mat_prop_out_basename = interface_mat
        boundary = ${interface}
        interface_value_type = primary
        mat_prop_var_out_basename = interface_mat_var
        nl_var_primary = u
        nl_var_secondary = u
    [../]
    # [./interface_material_av]
    #     type = InterfaceValueMaterial
    #     mat_prop_primary = mat
    #     mat_prop_secondary = mat
    #     var_primary = u
    #     var_secondary = u
    #     mat_prop_out_basename = interface_mat
    #     boundary = ${interface}
    #     interface_value_type = average
    #     mat_prop_var_out_basename = interface_mat_var
    #     nl_var_primary = u
    #     nl_var_secondary = u
    # [../]
    # [./interface_material_sec]
    #     type = InterfaceValueMaterial
    #     mat_prop_primary = mat
    #     mat_prop_secondary = mat
    #     var_primary = u
    #     var_secondary = u
    #     mat_prop_out_basename = interface_mat
    #     boundary = ${interface}
    #     interface_value_type = secondary
    #     mat_prop_var_out_basename = interface_mat_var
    #     nl_var_primary = u
    #     nl_var_secondary = u
    # [../]
[]


[AuxVariables]
    [AuxV_mat]
        family = MONOMIAL
        order = CONSTANT
    []
    [AuxV_mat_bis]
        family = MONOMIAL
        order = CONSTANT
    []
    [AuxV_mat2]
        family = MONOMIAL
        order = CONSTANT
    []
    [AuxV_interface_mat_prim]
        family = MONOMIAL
        order = CONSTANT
    []
    [AuxV_interface_mat_av]
        family = MONOMIAL
        order = CONSTANT
    []
    [AuxV_interface_mat_sec]
        family = MONOMIAL
        order = CONSTANT
    []
    [AuxV_mat_copypasted]
        family = MONOMIAL
        order = CONSTANT
    []
[]

[AuxKernels]
    [AuxK_mat]
        type = MaterialRealAux
        property = mat
        variable = AuxV_mat
    []
    [AuxK_mat2]
        type = MaterialRealAux
        property = mat2
        variable = AuxV_mat2
        block = 2
    []
    [AuxK_mat_bis]
        type = MaterialRealAux
        property = mat
        variable = AuxV_mat_bis
        block = '0 1'
    []
    # [AuxK_mat_copypasted]
    #     type = NearestNodeValueAux
    #     variable = AuxV_mat_copypasted
    #     block = 2
    #     boundary = joint
    #     paired_variable = 'AuxV_mat_bis'
    #     paired_boundary = 'interface'
    # []
    [AuxK_mat_copypasted]
        type = CopyValueAux
        variable = AuxV_mat_copypasted
        source = 'AuxV_mat_bis'
        block = 2
        # state = OLDER
        # execute_on = 'initial timestep_end'
    []
    [AuxK_interface_mat_prim]
        type = MaterialRealAux
        property = interface_mat_primary
        variable = AuxV_interface_mat_prim
        boundary = ${interface}
        check_boundary_restricted = false
    []
    # [AuxK_interface_mat_av]
    #     type = MaterialRealAux
    #     property = interface_mat_average
    #     variable = AuxV_interface_mat_av
    #     boundary = ${interface}
    #     check_boundary_restricted = false
    # []
    # [AuxK_interface_mat_sec]
    #     type = MaterialRealAux
    #     property = interface_mat_secondary
    #     variable = AuxV_interface_mat_sec
    #     boundary = ${interface}
    #     check_boundary_restricted = false
    # []
[]

[Executioner]
  type = Transient
  solve_type = NEWTON
  end_time = 2
  dt = 1
[]

[Outputs]
  exodus = true
[]