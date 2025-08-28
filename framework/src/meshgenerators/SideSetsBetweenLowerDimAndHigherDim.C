//* This file is part of the MOOSE framework
//* https://mooseframework.inl.gov
//*
//* All rights reserved, see COPYRIGHT for full restrictions
//* https://github.com/idaholab/moose/blob/master/COPYRIGHT
//*
//* Licensed under LGPL 2.1, please see LICENSE for details
//* https://www.gnu.org/licenses/lgpl-2.1.html

#include "SideSetsBetweenLowerDimAndHigherDim.h"
#include "Parser.h"
#include "InputParameters.h"
#include "MooseMesh.h"
#include "MooseMeshUtils.h"

#include "libmesh/mesh_generation.h"
#include "libmesh/mesh.h"
#include "libmesh/string_to_enum.h"
#include "libmesh/quadrature_gauss.h"
#include "libmesh/point_locator_base.h"
#include "libmesh/elem.h"
#include "libmesh/remote_elem.h"

registerMooseObject("MooseApp", SideSetsBetweenLowerDimAndHigherDim);

InputParameters
SideSetsBetweenLowerDimAndHigherDim::validParams()
{
  InputParameters params = MeshGenerator::validParams();
  params.addRequiredParam<MeshGeneratorName>("input", "The mesh we want to modify");
  params.addRequiredParam<std::vector<BoundaryName>>(
      "new_boundary", "The list of boundary names to create on the supplied subdomain");
  params.addParam<bool>("fixed_normal",
                        false,
                        "This Boolean determines whether we fix our normal "
                        "or allow it to vary to \"paint\" around curves");

  params.addParam<bool>("replace",
                        false,
                        "If true, replace the old sidesets. If false, the current sidesets (if "
                        "any) will be preserved.");

  params.addParam<std::vector<BoundaryName>>(
      "included_boundaries",
      "A set of boundary names or ids whose sides will be included in the new sidesets.  A side "
      "is only added if it also belongs to one of these boundaries.");
  params.addParam<std::vector<BoundaryName>>(
      "excluded_boundaries",
      "A set of boundary names or ids whose sides will be excluded from the new sidesets.  A side "
      "is only added if does not belong to any of these boundaries.");
  params.addParam<std::vector<SubdomainName>>(
      "included_subdomains",
      "A set of subdomain names or ids whose sides will be included in the new sidesets. A side "
      "is only added if the subdomain id of the corresponding element is in this set.");
  params.addParam<std::vector<SubdomainName>>("included_neighbors",
                                              "A set of neighboring subdomain names or ids. A face "
                                              "is only added if the subdomain id of the "
                                              "neighbor is in this set");
  params.addParam<bool>(
      "include_only_external_sides",
      false,
      "Whether to only include external sides when considering sides to add to the sideset");

  params.addParam<Point>("normal",
                         Point(),
                         "If supplied, only faces with normal equal to this, up to "
                         "normal_tol, will be added to the sidesets specified");
  params.addRangeCheckedParam<Real>("normal_tol",
                                    0.1,
                                    "normal_tol>=0 & normal_tol<=2",
                                    "If normal is supplied then faces are "
                                    "only added if face_normal.normal_hat >= "
                                    "1 - normal_tol, where normal_hat = "
                                    "normal/|normal|");
  params.addParam<Real>("variance", "The variance allowed when comparing normals");
  params.deprecateParam("variance", "normal_tol", "4/01/2025");

  // Sideset restriction param group
  params.addParamNamesToGroup(
      "included_boundaries excluded_boundaries included_subdomains included_neighbors "
      "include_only_external_sides normal normal_tol",
      "Sideset restrictions");

  return params;
}

SideSetsBetweenLowerDimAndHigherDim::SideSetsBetweenLowerDimAndHigherDim(const InputParameters & parameters)
  : MeshGenerator(parameters),
    _input(getMesh("input")),
    _boundary_names(std::vector<BoundaryName>()),
    _fixed_normal(getParam<bool>("fixed_normal")),
    _replace(getParam<bool>("replace")),
    _check_included_boundaries(isParamValid("included_boundaries")),
    _check_excluded_boundaries(isParamValid("excluded_boundaries")),
    _check_subdomains(isParamValid("included_subdomains")),
    _check_neighbor_subdomains(isParamValid("included_neighbors")),
    _included_boundary_ids(std::vector<boundary_id_type>()),
    _excluded_boundary_ids(std::vector<boundary_id_type>()),
    _included_subdomain_ids(std::vector<subdomain_id_type>()),
    _included_neighbor_subdomain_ids(std::vector<subdomain_id_type>()),
    _include_only_external_sides(getParam<bool>("include_only_external_sides")),
    _using_normal(isParamSetByUser("normal")),
    _normal(_using_normal ? Point(getParam<Point>("normal") / getParam<Point>("normal").norm())
                          : getParam<Point>("normal")),
    _normal_tol(getParam<Real>("normal_tol"))
{
  if (isParamValid("new_boundary"))
    _boundary_names = getParam<std::vector<BoundaryName>>("new_boundary");
  

}

SideSetsBetweenLowerDimAndHigherDim::~SideSetsBetweenLowerDimAndHigherDim() {}




void
SideSetsBetweenLowerDimAndHigherDim::setup(MeshBase & mesh)
{
  mooseAssert(_fe_face == nullptr, "FE Face has already been initialized");

  // To know the dimension of the mesh
  if (!mesh.is_prepared())
    mesh.prepare_for_use();
  // const auto dim = mesh.mesh_dimension();
  // std::cout << "[DEBUG_SSGB9] dim mesh" << mesh.mesh_dimension() << std::endl;
  

  // // Setup the FE Object so we can calculate normals
  // libMesh::FEType fe_type(Utility::string_to_enum<Order>("CONSTANT"),
  //                         Utility::string_to_enum<libMesh::FEFamily>("MONOMIAL"));
  // _fe_face = libMesh::FEBase::build(dim, fe_type);
  // _qface = std::make_unique<libMesh::QGauss>(dim - 1, FIRST);
  // _fe_face->attach_quadrature_rule(_qface.get());
  // // Must always pre-request quantities you want to compute
  // _fe_face->get_normals();
  std::cout << "[DEBUG_SSGB10] dim mesh" << mesh.mesh_dimension() << std::endl;

  // Handle incompatible parameters
  if (_include_only_external_sides && _check_neighbor_subdomains)
    paramError("include_only_external_sides", "External sides dont have neighbors");

  if (_check_included_boundaries)
  {
    const auto & included_boundaries = getParam<std::vector<BoundaryName>>("included_boundaries");
    for (const auto & boundary_name : _boundary_names)
      if (std::find(included_boundaries.begin(), included_boundaries.end(), boundary_name) !=
          included_boundaries.end())
        paramError(
            "new_boundary",
            "A boundary cannot be both the new boundary and be included in the list of included "
            "boundaries. If you are trying to restrict an existing boundary, you must use a "
            "different name for 'new_boundary', delete the old boundary, and then rename the "
            "new boundary to the old boundary.");

    _included_boundary_ids = MooseMeshUtils::getBoundaryIDs(mesh, included_boundaries, false);

    // Check that the included boundary ids/names exist in the mesh
    for (const auto i : index_range(_included_boundary_ids))
      if (_included_boundary_ids[i] == Moose::INVALID_BOUNDARY_ID)
        paramError("included_boundaries",
                   "The boundary '",
                   included_boundaries[i],
                   "' was not found within the mesh");
  }

  if (_check_excluded_boundaries)
  {
    const auto & excluded_boundaries = getParam<std::vector<BoundaryName>>("excluded_boundaries");
    for (const auto & boundary_name : _boundary_names)
      if (std::find(excluded_boundaries.begin(), excluded_boundaries.end(), boundary_name) !=
          excluded_boundaries.end())
        paramError(
            "new_boundary",
            "A boundary cannot be both the new boundary and be excluded in the list of excluded "
            "boundaries.");
    _excluded_boundary_ids = MooseMeshUtils::getBoundaryIDs(mesh, excluded_boundaries, false);

    // Check that the excluded boundary ids/names exist in the mesh
    for (const auto i : index_range(_excluded_boundary_ids))
      if (_excluded_boundary_ids[i] == Moose::INVALID_BOUNDARY_ID)
        paramError("excluded_boundaries",
                   "The boundary '",
                   excluded_boundaries[i],
                   "' was not found within the mesh");

    if (_check_included_boundaries)
    {
      // Check that included and excluded boundary lists do not overlap
      for (const auto & boundary_id : _included_boundary_ids)
        if (std::find(_excluded_boundary_ids.begin(), _excluded_boundary_ids.end(), boundary_id) !=
            _excluded_boundary_ids.end())
          paramError("excluded_boundaries",
                     "'included_boundaries' and 'excluded_boundaries' lists should not overlap");
    }
  }

  // Get the boundary ids from the names
  if (parameters().isParamValid("included_subdomains"))
  {
    // check that the subdomains exist in the mesh
    const auto subdomains = getParam<std::vector<SubdomainName>>("included_subdomains");
    for (const auto & name : subdomains)
      if (!MooseMeshUtils::hasSubdomainName(mesh, name))
        paramError("included_subdomains", "The block '", name, "' was not found in the mesh");

    _included_subdomain_ids = MooseMeshUtils::getSubdomainIDs(mesh, subdomains);
    std::cout << "[DEBUG_SSGB8] included_subdomains, wich are the primary_block: ";
    for (const auto & id : _included_subdomain_ids)
      std::cout << id << " ";
    std::cout << std::endl;
  }

  if (parameters().isParamValid("included_neighbors"))
  {
    // check that the subdomains exist in the mesh
    const auto subdomains = getParam<std::vector<SubdomainName>>("included_neighbors");
    for (const auto & name : subdomains)
      if (!MooseMeshUtils::hasSubdomainName(mesh, name))
        paramError("included_neighbors", "The block '", name, "' was not found in the mesh");

    _included_neighbor_subdomain_ids = MooseMeshUtils::getSubdomainIDs(mesh, subdomains);
    std::cout << "[DEBUG_SSGB7] included_neighbors, wich are the paired_block: ";
    for (const auto & id : _included_neighbor_subdomain_ids)
      std::cout << id << " ";
    std::cout << std::endl;
  }
  std::cout << "[DEBUG_SSGB11] dim mesh" << mesh.mesh_dimension() << std::endl;

  // We will want to Change the below code when we have more fine-grained control over advertising
  // what we need and how we satisfy those needs. For now we know we need to have neighbors per
  // #15823...and we do have an explicit `find_neighbors` call...but we don't have a
  // `neighbors_found` API and it seems off to do:
  //
  // if (!mesh.is_prepared())
  //   mesh.find_neighbors()
}

void
SideSetsBetweenLowerDimAndHigherDim::finalize()
{
  _qface.reset();
  _fe_face.reset();
}

void
SideSetsBetweenLowerDimAndHigherDim::flood(const Elem * elem,
                             const boundary_id_type & side_id,
                             MeshBase & mesh) //const Point & normal,
{
  if (elem == nullptr || elem == remote_elem ||
      (_visited[side_id].find(elem) != _visited[side_id].end()))
    return;

  // Skip if element is not in specified subdomains
  if (_check_subdomains && !elementSubdomainIdInList(elem, _included_subdomain_ids)){
    std::cout << "[DEBUG_FLOOD] Element subdomain ID: " << elem->subdomain_id() << std::endl;
    std::cout << "[DEBUG_FLOOD] Allowed subdomain IDs: ";
    for (const auto & id : _included_subdomain_ids)
      std::cout << id << " ";
    std::cout << std::endl;
    return;
  }
  _visited[side_id].insert(elem);

  // Request to compute normal vectors
  // const std::vector<Point> & face_normals = _fe_face->get_normals();

  for (const auto side : make_range(elem->n_sides()))
  {

    // _fe_face->reinit(elem, side);
    // // We'll just use the normal of the first qp
    // const Point face_normal = face_normals[0];

    if (!elemSideSatisfiesRequirements(elem, side, mesh))//, normal, face_normal))
      continue;

    if (_replace)
      mesh.get_boundary_info().remove_side(elem, side);

    mesh.get_boundary_info().add_side(elem, side, side_id);
    for (const auto neighbor : make_range(elem->n_sides()))
    {
      // Flood to the neighboring elements using the current matching side normal from this
      // element.
      // This will allow us to tolerate small changes in the normals so we can "paint" around a
      // curve.
      flood(elem->neighbor_ptr(neighbor), //_fixed_normal ? normal : face_normal, 
                          side_id, mesh);
    }
  }
}

bool
SideSetsBetweenLowerDimAndHigherDim::normalsWithinTol(const Point & normal_1,
                                        const Point & normal_2,
                                        const Real & tol) const
{
  return (1.0 - normal_1 * normal_2) <= tol;
}

bool
SideSetsBetweenLowerDimAndHigherDim::elementSubdomainIdInList(
    const Elem * const elem, const std::vector<subdomain_id_type> & subdomain_id_list) const
{
  subdomain_id_type curr_subdomain = elem->subdomain_id();

  return std::find(subdomain_id_list.begin(), subdomain_id_list.end(), curr_subdomain) !=
         subdomain_id_list.end();
}

bool
SideSetsBetweenLowerDimAndHigherDim::elementSideInIncludedBoundaries(const Elem * const elem,
                                                       const unsigned int side,
                                                       const MeshBase & mesh) const
{
  for (const auto & bid : _included_boundary_ids)
    if (mesh.get_boundary_info().has_boundary_id(elem, side, bid))
      return true;
  return false;
}

bool
SideSetsBetweenLowerDimAndHigherDim::elementSideInExcludedBoundaries(const Elem * const elem,
                                                       const unsigned int side,
                                                       const MeshBase & mesh) const
{
  for (const auto bid : _excluded_boundary_ids)
    if (mesh.get_boundary_info().has_boundary_id(elem, side, bid))
      return true;
  return false;
}

bool
SideSetsBetweenLowerDimAndHigherDim::elemSideSatisfiesRequirements(const Elem * const elem,
                                                     const unsigned int side,
                                                     const MeshBase & mesh)//,//  const Point & desired_normal,//  const Point & face_normal)
{  
  // const Elem * const neighbor = elem->neighbor_ptr(side);
  // Print current element's subdomain ID
  std::cout << "[DEBUG] Element neighbor subdomain ID: " << elem->neighbor_ptr(side)->subdomain_id() << std::endl;
  std::cout << "[DEBUG] Element subdomain ID: " << elem->subdomain_id() << std::endl;

  // Print the full list of allowed subdomain IDs
  std::cout << "[DEBUG] Allowed neighbor subdomain IDs: ";
  for (const auto & id : _included_neighbor_subdomain_ids)
    std::cout << id << " ";
  std::cout << std::endl;
    // Print the full list of allowed subdomain IDs
  std::cout << "[DEBUG] Allowed subdomain IDs: ";
  for (const auto & id : _included_subdomain_ids)
    std::cout << id << " ";
  std::cout << std::endl;
  // Skip if side has neighbor and we only want external sides
  if ((elem->neighbor_ptr(side) && _include_only_external_sides))
  {
    std::cout << "SSGB1: Hello from SideSetsBetweenLowerDimAndHigherDim, 'has neighbor and we only want external sides'" << std::endl;
    return false;
  }
  // Skip if side is not part of included boundaries
  if (_check_included_boundaries && !elementSideInIncludedBoundaries(elem, side, mesh))
  {
    std::cout << "SSGB2: Hello from SideSetsBetweenLowerDimAndHigherDim, 'Skip if side is not part of included boundaries'" << std::endl;
    return false;
  }
  // Skip if side is part of excluded boundaries
  if (_check_excluded_boundaries && elementSideInExcludedBoundaries(elem, side, mesh))
  {
    std::cout << "SSGB3: Hello from SideSetsBetweenLowerDimAndHigherDim, 'Skip if side is part of excluded boundaries'" << std::endl;
    return false;
  }
  // Skip if element does not have neighbor in specified subdomains
  if (_check_neighbor_subdomains)
  {
    const Elem * const neighbor = elem->neighbor_ptr(side);
    // if the neighbor does not exist, then skip this face; we only add sidesets
    // between existing elems if _check_neighbor_subdomains is true
    


    if (!(neighbor && elementSubdomainIdInList(neighbor, _included_neighbor_subdomain_ids)))
    {
      bool result = neighbor;
      std::cout << "SSGB4: Hello from SideSetsBetweenLowerDimAndHigherDim, 'if the neighbor does not exist, then skip this face'" << std::endl;
      std::cout << "SSGB4: neighbor  "<< neighbor<< "  elementSubdomainIdInList(neighbor, _included_neighbor_subdomain_ids)  "<< elementSubdomainIdInList(neighbor, _included_neighbor_subdomain_ids) << std::endl;
      std::cout << "SSGB4: Result of if condition on neighbor: " << std::boolalpha << result << std::endl;

    
      return false;
    }
  }

  // if (_using_normal && !normalsWithinTol(desired_normal, face_normal, _normal_tol))
  // {
  //   std::cout << "SSGB5: Hello from SideSetsBetweenLowerDimAndHigherDim, 'normal outside tol'" << std::endl;
  //   return false;
  // }

  std::cout << "SSGB6: Hello from SideSetsBetweenLowerDimAndHigherDim, 'elemSideSatisfiesRequirements == true'" << std::endl;
  return true;
}


std::unique_ptr<MeshBase>
SideSetsBetweenLowerDimAndHigherDim::generate()
{
  std::unique_ptr<MeshBase> mesh = std::move(_input);

  // construct the FE object so we can compute normals of faces
  setup(*mesh); //[so this i don't care, I modified it just to have the mesh prepared]

  std::vector<boundary_id_type> boundary_ids =
      MooseMeshUtils::getBoundaryIDs(*mesh, _boundary_names, true);

  // Get a reference to our BoundaryInfo object for later use
  BoundaryInfo & boundary_info = mesh->get_boundary_info();

  // printFullBoundaryInfo(mesh.getMesh());
  // Prepare to query about sides adjacent to remote elements if we're
  // on a distributed mesh
  const processor_id_type my_n_proc = mesh->n_processors();
  const processor_id_type my_proc_id = mesh->processor_id();
  typedef std::vector<std::pair<dof_id_type, unsigned int>> vec_type;
  std::vector<vec_type> queries(my_n_proc);

  // // Request to compute normal vectors
  // const std::vector<Point> & face_normals = _fe_face->get_normals();

  for (const auto & elem : mesh->active_element_ptr_range())
  {
    // We only need to loop over elements in the primary subdomain
    // std::cout << "[DEBUG_GENERATE_A] Element subdomain ID: " << elem->subdomain_id()<< std::endl;
    // std::cout  << "   ELEM: " << elem << std::endl;
    // std::cout << "[DEBUG_GENERATE_A] Allowed subdomain IDs: ";
    // for (const auto & id : _included_subdomain_ids)
    //   std::cout << id << " ";
    // std::cout << std::endl;
    // std::cout << "[DEBUG_GENERATE_A] Element in subdomain ID?: " << std::boolalpha << elementSubdomainIdInList(elem, _included_subdomain_ids) << std::endl;
    if (_check_subdomains && !elementSubdomainIdInList(elem, _included_subdomain_ids))
      continue;

    // std::cout << "[DEBUG_GENERATE_B]   ELEM: " << elem << "   elem->n_sides(): " << elem->n_sides() << std::endl;
    // // const Elem * neighbor = neighbor_element(elem, mesh);
    auto [neighbor, side] = neighbor_element(elem,  mesh.get());

    // std::cout << "[DEBUG_GENERATE_D] ELEM: " << elem <<  "  MATCHING_NEIGHBOR: " << neighbor << std::endl;
    // for (unsigned int i = 0; i < elem->n_nodes(); ++i)
    // {
    //   const Point & pt = elem->point(i);
    //   std::cout << "[DEBUG_GENERATE_D] Node ELEM " << i << ": (" << pt(0) << ", " << pt(1)<< ", " << pt(2)<< ")" << std::endl;
    // }
    // for (unsigned int i = 0; i < neighbor->n_nodes(); ++i)
    // {
    //   const Point & pt = neighbor->point(i);
    //   std::cout << "[DEBUG_GENERATE_D] Node MATCHING_NEIGHBOR " << i << ": (" << pt(0) << ", " << pt(1)<< ", " << pt(2)<< ")" << std::endl;
    // }

    if (neighbor)
    {
      // mesh->set_isnt_prepared();
      if (elem->dim() == 1)
      {
        std::cout << "[DEBUG_GENERATE_J.1] NEIGHBOR BEFORE CHANGE "<< std::endl;
        print_elem_sides(neighbor); 
        std::cout << "[DEBUG_GENERATE_J.1] ELEM BEFORE CHANGE "<< std::endl;
        print_elem_sides(elem);
        // const_cast<Elem *>(neighbor)->set_neighbor(side, const_cast<Elem *>(elem));

        if (_replace)
          boundary_info.remove_side(neighbor, side);
        for (const auto & boundary_id : boundary_ids)
          boundary_info.add_side(neighbor, side, boundary_id);

        std::cout << "[DEBUG_GENERATE_J.1] NEIGHBOR AFTER CHANGE "<< std::endl;
        print_elem_sides(neighbor); 
        std::cout << "[DEBUG_GENERATE_J.1] ELEM AFTER CHANGE "<< std::endl;
        print_elem_sides(elem);
      }
      else
      {
        std::cout << "[DEBUG_GENERATE_J.2] NEIGHBOR BEFORE CHANGE "<< std::endl;
        print_elem_sides(neighbor); 
        std::cout << "[DEBUG_GENERATE_J.2] ELEM BEFORE CHANGE "<< std::endl;
        print_elem_sides(elem);
        // const_cast<Elem *>(elem)->set_neighbor(side, const_cast<Elem *>(neighbor));

        if (_replace)
          boundary_info.remove_side(elem, side);
        for (const auto & boundary_id : boundary_ids)
          boundary_info.add_side(elem, side, boundary_id);

        std::cout << "[DEBUG_GENERATE_J.2] NEIGHBOR AFTER CHANGE "<< std::endl;
        print_elem_sides(neighbor); 
        std::cout << "[DEBUG_GENERATE_J.2] ELEM AFTER CHANGE "<< std::endl;
        print_elem_sides(elem);
        // printFullBoundaryInfo(mesh.getMesh());
      }
      // mesh->prepare_for_use();
      // std::cout << "[DEBUG_GENERATE_J.3] NEIGHBOR AFTER prepareforuse "<< std::endl;
      // print_elem_sides(neighbor); 
      // std::cout << "[DEBUG_GENERATE_J.3] ELEM AFTER prepareforuse "<< std::endl;
      // print_elem_sides(elem);

    }

  
    // for (const auto & side : make_range(elem->n_sides()))
    // {
    //   const Elem * neighbor = elem->neighbor_ptr(side);
    //   // std::cout << "[DEBUG_GENERATE_B] ELEM: " << elem << " sides: " << side << "  NEIGHBOR: " << neighbor << std::endl;
    //   // for (unsigned int i = 0; i < elem->n_nodes(); ++i)
    //   // {
    //   //   const Point & pt = elem->point(i);
    //   //   std::cout << "[DEBUG_GENERATE_B] Node ELEM " << i << ": (" << pt(0) << ", " << pt(1)<< ", " << pt(2)<< ")" << std::endl;
    //   // }

    //   // On a replicated mesh, we add all subdomain sides ourselves.
    //   // On a distributed mesh, we may have missed sides which
    //   // neighbor remote elements.  We should query any such cases.
    //   if (neighbor == remote_elem)
    //   {
    //     queries[elem->processor_id()].push_back(std::make_pair(elem->id(), side));
    //   }
    //   else if (neighbor != NULL)
    //   {
    //   //   _fe_face->reinit(elem, side);// Can only loop over elem of dim 2 or higher
    //   //   // We'll just use the normal of the first qp
    //   //   const Point & face_normal = face_normals[0];
    //   //   // Add the boundaries, if appropriate
    //     if (elemSideSatisfiesRequirements(elem, side, *mesh))//, _normal, face_normal))
    //     {
    //       // Add the boundaries
    //       if (_replace)
    //         boundary_info.remove_side(elem, side);
    //       for (const auto & boundary_id : boundary_ids)
    //         boundary_info.add_side(elem, side, boundary_id);
    //     }
    //   }
    // }
  }
  std::cout << "[DEBUG_SideSetBetweenLowerDimAndHigherDim] Out of big FOR" << std::endl;
  if (!mesh->is_serial())
  {
    const auto queries_tag = mesh->comm().get_unique_tag(),
               replies_tag = mesh->comm().get_unique_tag();

    std::vector<Parallel::Request> side_requests(my_n_proc - 1), reply_requests(my_n_proc - 1);

    // Make all requests
    for (const auto & p : make_range(my_n_proc))
    {
      if (p == my_proc_id)
        continue;

      Parallel::Request & request = side_requests[p - (p > my_proc_id)];

      mesh->comm().send(p, queries[p], request, queries_tag);
    }

    // Reply to all requests
    std::vector<vec_type> responses(my_n_proc - 1);

    for (const auto & p : make_range(uint(1), my_n_proc))
    {
      vec_type query;

      Parallel::Status status(mesh->comm().probe(Parallel::any_source, queries_tag));
      const processor_id_type source_pid = cast_int<processor_id_type>(status.source());

      mesh->comm().receive(source_pid, query, queries_tag);

      Parallel::Request & request = reply_requests[p - 1];

      for (const auto & q : query)
      {
        const Elem * elem = mesh->elem_ptr(q.first);
        const unsigned int side = q.second;
        const Elem * neighbor = elem->neighbor_ptr(side);

        std::cout << "[DEBUG_SideSetBetweenLowerDimAndHigherDim] side "<< side << std::endl;
        std::cout << "[DEBUG_SideSetBetweenLowerDimAndHigherDim] elem id "<< elem->id() << "neighbor id "<< neighbor->id()  << std::endl;

        if (neighbor != NULL)
        {
          // _fe_face->reinit(elem, side);
          // // We'll just use the normal of the first qp
          // const Point & face_normal = _fe_face->get_normals()[0];
          // Add the boundaries, if appropriate
          if (elemSideSatisfiesRequirements(elem, side, *mesh))//, _normal, face_normal))
            responses[p - 1].push_back(std::make_pair(elem->id(), side));
        }
      }

      mesh->comm().send(source_pid, responses[p - 1], request, replies_tag);
    }

    // Process all incoming replies
    for (processor_id_type p = 1; p != my_n_proc; ++p)
    {
      Parallel::Status status(this->comm().probe(Parallel::any_source, replies_tag));
      const processor_id_type source_pid = cast_int<processor_id_type>(status.source());

      vec_type response;

      this->comm().receive(source_pid, response, replies_tag);

      for (const auto & r : response)
      {
        const Elem * elem = mesh->elem_ptr(r.first);
        const unsigned int side = r.second;

        if (_replace)
          boundary_info.remove_side(elem, side);
        for (const auto & boundary_id : boundary_ids)
          boundary_info.add_side(elem, side, boundary_id);
      }
    }

    Parallel::wait(side_requests);
    Parallel::wait(reply_requests);
  }
  std::cout << "[DEBUG_SideSetBetweenLowerDimAndHigherDim] Before small FOR + boundary_ids.size()" << boundary_ids.size() << std::endl;
  std::cout << "Boundary ID to Name Mapping:\n";
  for (const auto & i : make_range(boundary_ids.size())){
    boundary_info.sideset_name(boundary_ids[i]) = _boundary_names[i];
    std::cout << "  ID " << i << " → Name: \"" << boundary_info.sideset_name(boundary_ids[i]) << "\"\n";
  }
  // printFullBoundaryInfo(mesh.getMesh());
  std::cout << " " << std::endl;
  std::cout << "[DEBUG_SideSetBetweenLowerDimAndHigherDim] Out of small FOR" << std::endl;
  mesh->set_isnt_prepared();
  std::cout << "[DEBUG_SideSetBetweenLowerDimAndHigherDim] Mesh isnt prepapred" << std::endl;
  
  return dynamic_pointer_cast<MeshBase>(mesh);
}


std::pair<const Elem *, unsigned int> 
SideSetsBetweenLowerDimAndHigherDim::neighbor_element(const Elem * const elem, const MeshBase * mesh) const
{
  if (elem->dim() == 1)
  {
    for (const auto & neighbor : mesh->active_element_ptr_range())
    {
      if (_check_neighbor_subdomains && !elementSubdomainIdInList(neighbor, _included_neighbor_subdomain_ids))
        continue;

      for (unsigned int side = 0; side < neighbor->n_sides(); ++side) 
      {
        unsigned int shared_points = 0; 
        for (unsigned int k = 0; k < neighbor->side_ptr(side)->n_nodes(); ++k)
        {
          const Point & coord_neighbor = neighbor->side_ptr(side)->point(k);
          for (unsigned int i = 0; i < elem->n_nodes(); ++i)
          {
            const Point & coord_elem = elem->point(i);
            std::cout << "[DEBUG_GENERATE_C.1] coord_elem: " << coord_elem << std::endl;
            std::cout << "[DEBUG_GENERATE_C.1] coord_neighbor: " << coord_neighbor << std::endl;
            std::cout << "[DEBUG_GENERATE_C.1] coord==coord?: " << std::boolalpha << (coord_neighbor == coord_elem) << std::endl;
            if (coord_elem == coord_neighbor)
            {
              shared_points = shared_points + 1; 
            }
          }
        }
        if (shared_points == 2)
        {
          return std::make_pair(neighbor, side); 
        }
      }
    }
  } 
  else
  {
    for (const auto & neighbor : mesh->active_element_ptr_range())
    {
      if (_check_neighbor_subdomains && !elementSubdomainIdInList(neighbor, _included_neighbor_subdomain_ids))
        continue; 
      for (unsigned int side = 0; side < elem->n_sides(); ++side)  
      {
        unsigned int shared_points = 0;
        for (unsigned int k = 0; k < elem->side_ptr(side)->n_nodes(); ++k)
        {
          const Point & coord_elem = elem->side_ptr(side)->point(k);
          for (unsigned int i = 0; i < neighbor->n_nodes(); ++i)
          {
            const Point & coord_neighbor = neighbor->point(i);
            std::cout << "[DEBUG_GENERATE_C.2] coord_elem: " << coord_elem << std::endl;
            std::cout << "[DEBUG_GENERATE_C.2] coord_neighbor: " << coord_neighbor << std::endl;
            std::cout << "[DEBUG_GENERATE_C.2] coord==coord?: " << std::boolalpha << (coord_neighbor == coord_elem) << std::endl;
            if (coord_elem == coord_neighbor)
            {
              shared_points = shared_points + 1; 
            }
          }
        }

        if (shared_points == 2)
        {
          return std::make_pair(neighbor, side); 
        }
      }
    }
  }
  return std::make_pair(nullptr, 0); // No neighbor+side found
}

void
SideSetsBetweenLowerDimAndHigherDim::print_elem_sides(const Elem * const elem) const
{
  std::cout << "Number of elem sides " << elem->n_sides() << std::endl;
    for (unsigned int s = 0; s < elem->n_sides(); ++s)
    {
      const Elem * neig = elem->neighbor_ptr(s);

      if (neig != nullptr && neig != remote_elem)
      {
        std::cout << "Neighbor on side " << s << ": Elem ID = " << neig->id() << std::endl;
      }
      else if (neig == remote_elem)
      {
        std::cout << "Neighbor on side " << s << " is remote." << std::endl;
      }
      else
      {
        std::cout << "No neighbor on side " << s << std::endl;
      }
    }    
  std::cout << "Printing routine finisehd" << std::endl;
}


// void printFullBoundaryInfo(const libMesh::MeshBase & mesh)
// {
//   const libMesh::BoundaryInfo & boundary_info = mesh.get_boundary_info();

//   std::set<libMesh::boundary_id_type> boundary_ids;
//   boundary_info.sideset_ids(boundary_ids);

//   std::cout << "\n=== Full Boundary Info ===\n";
//   for (auto bid : boundary_ids)
//   {
//     std::cout << "Boundary ID: " << bid;

//     // Print name if available
//     if (boundary_info.has_sideset_name(bid))
//       std::cout << "  (Name: \"" << boundary_info.sideset_name(bid) << "\")";

//     std::cout << "\n";

//     // Get all element-side pairs for this boundary ID
//     const auto & elem_side_pairs = boundary_info.boundary_element_sides(bid);
//     for (const auto & pair : elem_side_pairs)
//     {
//       const libMesh::Elem * elem = pair.first;
//       unsigned int side = pair.second;

//       std::cout << "  - Elem ID " << elem->id()
//                 << ", side " << side
//                 << ", elem dim: " << elem->dim()
//                 << ", type: " << libMesh::Utility::enum_to_string(elem->type()) << "\n";
//     }

//     // Optional: print node associations
//     const auto & node_ids = boundary_info.nodeset_nodes(bid);
//     if (!node_ids.empty())
//     {
//       std::cout << "  [Nodeset Nodes: ";
//       for (auto node_id : node_ids)
//         std::cout << node_id << " ";
//       std::cout << "]\n";
//     }

//     std::cout << "\n";
//   }

//   std::cout << "===========================\n\n";
// }