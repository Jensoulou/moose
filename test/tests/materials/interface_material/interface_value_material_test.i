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
  [./diff]
    type = MatDiffusion
    variable = u
    diffusivity = 'diffusivity'
    block = 0
  [../]
  [./diff_fracture]
    type = MatDiffusion
    variable = w
    diffusivity = 'diffusivity'
    block = 2
  [../]

  [./diff_v]
    type = MatDiffusion
    variable = v
    diffusivity = 'diffusivity'
    block = 1
  [../]
[]

# [InterfaceKernels]
#   [tied]
#     type = PenaltyInterfaceDiffusion
#     variable = u
#     neighbor_var = v
#     jump_prop_name = "average_jump"
#     penalty = 1e6
#     boundary = 'interface'
#   []
# []

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
    value = 0
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
  [./interface_material_primary]
      type = InterfaceValueMaterial_test
      mat_prop_primary = diffusivity
      boundary = interface
  [../]
[]

[AuxKernels]
  # [./interface_material_avg]
  #   type = MaterialRealAux
  #   property = diffusivity
  #   variable = diffusivity_average
  #   boundary = interface
  # []
  # [./interface_material_jump_primary_minus_secondary]
  #   type = MaterialRealAux
  #   property = diffusivity
  #   variable = diffusivity_jump_primary_minus_secondary
  #   boundary = interface
  # []
  # [./interface_material_jump_secondary_minus_primary]
  #   type = MaterialRealAux
  #   property = diffusivity
  #   variable = diffusivity_jump_secondary_minus_primary
  #   boundary = interface
  # []
  # [./interface_material_jump_abs]
  #   type = MaterialRealAux
  #   property = diffusivity
  #   variable = diffusivity_jump_abs
  #   boundary = interface
  # []
  # [./interface_material_primary]
  #   type = MaterialRealAux
  #   property = diffusivity
  #   variable = diffusivity_primary
  #   boundary = interface
  # []
  # [./interface_material_secondary]
  #   type = MaterialRealAux
  #   property = diffusivity
  #   variable = diffusivity_secondary
  #   boundary = interface
  # []
  [diffusivity_var]
    type = MaterialRealAux
    property = diffusivity
    variable = diffusivity_var
  []
[]

[AuxVariables]
  [diffusivity_var]
    family = MONOMIAL
    order = CONSTANT
  []
  [./diffusivity_average]
    family = MONOMIAL
    order = CONSTANT
  []
  [./diffusivity_jump_primary_minus_secondary]
    family = MONOMIAL
    order = CONSTANT
  []
  [./diffusivity_jump_secondary_minus_primary]
    family = MONOMIAL
    order = CONSTANT
  []
  [./diffusivity_jump_abs]
    family = MONOMIAL
    order = CONSTANT
  []
  [./diffusivity_primary]
    family = MONOMIAL
    order = CONSTANT
  []
  [./diffusivity_secondary]
    family = MONOMIAL
    order = CONSTANT
  []
[]


[Executioner]
  type = Steady
  solve_type = NEWTON
[]

[Outputs]
  exodus = true
[]
