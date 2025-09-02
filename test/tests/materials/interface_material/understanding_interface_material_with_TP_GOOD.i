[Mesh]
    [gen]
        type = GeneratedMeshGenerator
        dim = 2
        nx = 20
        xmax = 2
        ny = 20
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
[]

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


[Materials]
    [mat1]
        type = GenericConstantMaterial
        prop_names = mat
        prop_values = 7
        block = 0
    []
    [mat2]
        type = GenericConstantMaterial
        prop_names = mat
        prop_values = 20
        block = 1
    []
    [./interface_material_avg]
        type = InterfaceValueMaterial
        mat_prop_primary = mat
        mat_prop_secondary = mat
        var_primary = u
        var_secondary = u
        mat_prop_out_basename = interface_mat
        boundary = interface
        interface_value_type = secondary
        mat_prop_var_out_basename = interface_mat_var
        nl_var_primary = u
        nl_var_secondary = u
    [../]
[]


[AuxVariables]
    [AuxV_mat]
        family = MONOMIAL
        order = CONSTANT
    []
    [AuxV_interface_mat]
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
    [AuxK_interface_mat]
        type = MaterialRealAux
        property = interface_mat_secondary
        variable = AuxV_interface_mat
        boundary = interface
    []
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