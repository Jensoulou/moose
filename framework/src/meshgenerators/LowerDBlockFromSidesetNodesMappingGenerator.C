//* This file is part of the MOOSE framework
//* https://mooseframework.inl.gov
//*
//* All rights reserved, see COPYRIGHT for full restrictions
//* https://github.com/idaholab/moose/blob/master/COPYRIGHT
//*
//* Licensed under LGPL 2.1, please see LICENSE for details
//* https://www.gnu.org/licenses/lgpl-2.1.html

#include "LowerDBlockFromSidesetNodesMappingGenerator.h"
#include "InputParameters.h"
#include "MooseTypes.h"
#include "CastUniquePointer.h"
#include "MooseMeshUtils.h"

#include "libmesh/distributed_mesh.h"
#include "libmesh/elem.h"
#include "libmesh/parallel_elem.h"
#include "libmesh/parallel_node.h"
#include "libmesh/compare_elems_by_level.h"
#include "libmesh/mesh_communication.h"

#include "timpi/parallel_sync.h"

#include <set>
#include <typeinfo>

registerMooseObject("MooseApp", LowerDBlockFromSidesetNodesMappingGenerator);

InputParameters
LowerDBlockFromSidesetNodesMappingGenerator::validParams()
{
  InputParameters params = MeshGenerator::validParams();

  params.addRequiredParam<MeshGeneratorName>("input", "The mesh we want to modify");
  params.addParam<SubdomainID>("new_block_id", "The lower dimensional block id to create");
  params.addParam<SubdomainName>("new_block_name",
                                 "The lower dimensional block name to create (optional)");
  params.addRequiredParam<std::vector<BoundaryName>>(
      "sidesets", "The sidesets from which to create the new block");

  params.addClassDescription("Adds lower dimensional elements on the specified sidesets.");

  return params;
}

LowerDBlockFromSidesetNodesMappingGenerator::LowerDBlockFromSidesetNodesMappingGenerator(const InputParameters & parameters)
  : MeshGenerator(parameters),
    _input(getMesh("input")),
    _sideset_names(getParam<std::vector<BoundaryName>>("sidesets"))
{
}

// Used to temporarily store information about which lower-dimensional
// sides to add and what subdomain id to use for the added sides.
struct ElemSideDouble2
{
  ElemSideDouble2(Elem * elem_in, unsigned short int side_in) : elem(elem_in), side(side_in) {}

  Elem * elem;
  unsigned short int side;
};

std::unique_ptr<MeshBase>
LowerDBlockFromSidesetNodesMappingGenerator::generate()
{
  std::unique_ptr<MeshBase> mesh = std::move(_input);

  // Generate a new block id if one isn't supplied.
  SubdomainID new_block_id = isParamValid("new_block_id")
                                 ? getParam<SubdomainID>("new_block_id")
                                 : MooseMeshUtils::getNextFreeSubdomainID(*mesh);

  // Make sure our boundary info and parallel counts are setup
  if (!mesh->is_prepared())
  {
    const bool allow_remote_element_removal = mesh->allow_remote_element_removal();
    // We want all of our boundary elements available, so avoid removing them if they haven't
    // already been so
    mesh->allow_remote_element_removal(false);
    mesh->prepare_for_use();
    mesh->allow_remote_element_removal(allow_remote_element_removal);
  }

  // Check that the sidesets are present in the mesh
  for (const auto & sideset : _sideset_names)
    if (!MooseMeshUtils::hasBoundaryName(*mesh, sideset))
      paramError("sidesets", "The sideset '", sideset, "' was not found within the mesh");

  auto sideset_ids = MooseMeshUtils::getBoundaryIDs(*mesh, _sideset_names, true);
  std::set<boundary_id_type> sidesets(sideset_ids.begin(), sideset_ids.end());
  auto side_list = mesh->get_boundary_info().build_side_list();
  if (!mesh->is_serial() && mesh->comm().size() > 1)
  {
    std::vector<Elem *> elements_to_send;
    unsigned short i_need_boundary_elems = 0;
    for (const auto & [elem_id, side, bc_id] : side_list)
    {
      libmesh_ignore(side);
      if (sidesets.count(bc_id))
      {
        // Whether we have this boundary information through our locally owned element or a ghosted
        // element, we'll need the boundary elements for parallel consistent addition
        i_need_boundary_elems = 1;
        auto * elem = mesh->elem_ptr(elem_id);
        if (elem->processor_id() == mesh->processor_id())
          elements_to_send.push_back(elem);
      }
    }

    std::set<const Elem *, libMesh::CompareElemIdsByLevel> connected_elements(
        elements_to_send.begin(), elements_to_send.end());
    std::set<const Node *> connected_nodes;
    reconnect_nodes(connected_elements, connected_nodes);
    std::set<dof_id_type> connected_node_ids;
    for (auto * nd : connected_nodes)
      connected_node_ids.insert(nd->id());

    std::vector<unsigned short> need_boundary_elems(mesh->comm().size());
    mesh->comm().allgather(i_need_boundary_elems, need_boundary_elems);
    std::unordered_map<processor_id_type, decltype(elements_to_send)> push_element_data;
    std::unordered_map<processor_id_type, decltype(connected_nodes)> push_node_data;

    for (const auto pid : index_range(mesh->comm()))
      // Don't need to send to self
      if (pid != mesh->processor_id() && need_boundary_elems[pid])
      {
        if (elements_to_send.size())
          push_element_data[pid] = elements_to_send;
        if (connected_nodes.size())
          push_node_data[pid] = connected_nodes;
      }

    auto node_action_functor = [](processor_id_type, const auto &)
    {
      // Node packing specialization already has unpacked node into mesh, so nothing to do
    };
    Parallel::push_parallel_packed_range(
        mesh->comm(), push_node_data, mesh.get(), node_action_functor);
    auto elem_action_functor = [](processor_id_type, const auto &)
    {
      // Elem packing specialization already has unpacked elem into mesh, so nothing to do
    };
    TIMPI::push_parallel_packed_range(
        mesh->comm(), push_element_data, mesh.get(), elem_action_functor);

    // now that we've gathered everything, we need to rebuild the side list
    side_list = mesh->get_boundary_info().build_side_list();
  }

  std::vector<std::pair<dof_id_type, ElemSideDouble2>> element_sides_on_boundary;
  dof_id_type counter = 0;
  for (const auto & triple : side_list)
    if (sidesets.count(std::get<2>(triple)))
    {
      if (auto elem = mesh->query_elem_ptr(std::get<0>(triple)))
      {
        if (!elem->active())
          mooseError(
              "Only active, level 0 elements can be made interior parents of new level 0 lower-d "
              "elements. Make sure that ",
              type(),
              "s are run before any refinement generators");
        element_sides_on_boundary.push_back(
            std::make_pair(counter, ElemSideDouble2(elem, std::get<1>(triple))));
      }
      ++counter;
    }

  dof_id_type max_elem_id = mesh->max_elem_id();
  unique_id_type max_unique_id = mesh->parallel_max_unique_id();

  // Making an important assumption that at least our boundary elements are the same on all
  // processes even in distributed mesh mode (this is reliant on the correct ghosting functors
  // existing on the mesh)
  std::cout << " [DEBUG] Nodes MApping 1" << std::endl;
  // std::ofstream outfile("node_id_mapping.txt", std::ios::trunc);
  std::string filename = "node_id_mapping.txt";                
  std::fstream fs;
  fs.open(filename, std::ios::out | std::ios::trunc); // truncate mode
  if (!fs.is_open())
      std::cerr << "Error opening file " << filename << std::endl;
  fs.close();
  std::cout << " [DEBUG] Nodes MApping 2 " << std::endl;
  bool first = true; 
  for (auto & [i, elem_side] : element_sides_on_boundary)
  {
    Elem * elem = elem_side.elem;

    const auto side = elem_side.side;

    // Build a non-proxy element from this side.
    std::unique_ptr<Elem> side_elem(elem->build_side_ptr(side));

    // The side will be added with the same processor id as the parent.
    side_elem->processor_id() = elem->processor_id();

    // Add subdomain ID
    side_elem->subdomain_id() = new_block_id;

    // Also assign the side's interior parent, so it is always
    // easy to figure out the Elem we came from.
    side_elem->set_interior_parent(elem);

    // Add id
    side_elem->set_id(max_elem_id + i);
    side_elem->set_unique_id(max_unique_id + i);

    std::cout << " [DEBUG] Nodes MApping 3" << std::endl;
    for (unsigned int j = 0; j < elem->side_ptr(side)->n_nodes(); ++j){
      std::cout << " [DEBUG] Nodes MApping 7. First: " << first << std::endl;
      const Node * old_node = elem->side_ptr(side)->node_ptr(j);
      Node * new_node; 
      if (first){
        new_node = mesh->add_point(*old_node);
        first = false; 
      }
      else{
        fs.open(filename, std::ios::in); // open for reading
        if (!fs.is_open())
        {
            std::cerr << "Error opening file " << filename << " for reading.\n";
        }
        dof_id_type n1, n2;
        dof_id_type new_id = libMesh::DofObject::invalid_id; 
        while (fs >> n1 >> n2)
        {
            if (n1 == old_node->id())
            {
                new_id = n2;
                break; 
            }
        }
        fs.close();
        if(new_id == libMesh::DofObject::invalid_id){
          new_node = mesh->add_point(*old_node);
        }
        else{
          new_node = mesh->node_ptr(new_id);
        }
      }
      
      side_elem->set_node(j) = new_node; 
      std::cout << "side_elem: "
                << side_elem 
                << std::endl;
      std::cout << "old_node: "
                << old_node 
                << " | id: " << old_node->id()
                << " | x=" << (*old_node)(0)
                << " | y=" << (*old_node)(1)
                << " | z=" << (*old_node)(2) << std::endl;
      std::cout << "new_node: "
                << new_node
                << " | id: " << new_node->id()
                << " | x=" << (*new_node)(0)
                << " | y=" << (*new_node)(1)
                << " | z=" << (*new_node)(2) << std::endl;
      std::cout << "elem->side_ptr(side)->node_ptr(j): " << elem->side_ptr(side)->node_ptr(j)
                << " | side_elem->node_ptr(j): " << side_elem->node_ptr(j) << std::endl;

      fs.open(filename, std::ios::out | std::ios::app); // append mode
      if (!fs.is_open())
      {
          std::cerr << "Error opening file " << filename << " for writing.\n";
      }
      fs << old_node->id() << " " << new_node->id() << "\n";
      fs.close();
      std::cout << " [DEBUG] Nodes MApping 6" << std::endl;
    }

    // Finally, add the lower-dimensional element to the Mesh.
    mesh->add_elem(side_elem.release());
    std::cout << " [DEBUG] Nodes MApping 4" << std::endl;
  };

  // Assign block name, if provided
  if (isParamValid("new_block_name"))
    mesh->subdomain_name(new_block_id) = getParam<SubdomainName>("new_block_name");

  const bool skip_partitioning_old = mesh->skip_partitioning();
  mesh->skip_partitioning(true);
  mesh->prepare_for_use();
  mesh->skip_partitioning(skip_partitioning_old);

  std::cout << " [DEBUG] Nodes MApping 5" << std::endl;
  return mesh;
}
// Mapping of the LowerDim nodes to the corrsponding nodes from 
// their interior parent, but it isn't working (seg fault). I believe 
// it is because I have to force the creation of new nodes before 
// returning the mesh (the problem comes from after the class I worked on). 
// Here is what it is coded: 
// let's say we have 2D elements along them we wish to create a lower dim one
// the ld elem will be created on the side of the 2D elem, aka interior parent elem
// side_elem will have two nodes, at exactly the same position as the ones on the side of
// interior_parent_elem. 
// side A: nodes 5 and 4
// elem_side A': nodes 25 and 26 
// --> Mapping 5->25 & 4->26
// side B: nodes 12 and 5
// elem_side B': nodes 27 and 25: Indeed, the corresponding node of 5 already exists, So we need 
// to appropriately set the correct existing node to be also part of elem_side B'
// --> Mapping 12->27 & 5->25

// but it is not working, seg fault at the 'return mesh' line


