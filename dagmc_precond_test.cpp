// std includes
#include <iostream>
#include <string>
// moab includes
#include "moab/ProgOptions.hpp"
#include "moab/ScdInterface.hpp"
#include "DagMC.hpp"

#define STRINGIFY_(X) #X
#define STRINGIFY(X) STRINGIFY_(X)

using namespace moab;

ErrorCode test_sphere();
ErrorCode test_cylinder();

//dagmc includes
int main() {

  ErrorCode rval;
  
  rval = test_sphere();
  MB_CHK_SET_ERR(rval, "DAGMC Preconditioner Sphere Test Failed");

  rval = test_cylinder();
  MB_CHK_SET_ERR(rval, "DAGMC Preconditioner Sphere Test Failed");
  
				    
  return 0;
}

ErrorCode dag_init_file(char *filename, DagMC* &dagmc, SignedDistanceField* &box) {
  //create a new dagmc instance and load the file
  ErrorCode rval;
  dagmc = new DagMC();
  rval = dagmc->load_file(filename);
  MB_CHK_SET_ERR(rval,"Could not load the preconditioner test file");

  //get the ScdBox of the first (and only) volume
  Range vols;
  const int three = 3;
  const void* const three_val[] = {&three};
  Tag geomTag = dagmc->geom_tag();
  rval = dagmc->moab_instance()->get_entities_by_type_and_tag( 0, MBENTITYSET, &geomTag, three_val, 1, vols );
  MB_CHK_SET_ERR(rval,"Could not get the model volumes");

  //there shoul only be one volume before we init the OBB tree
  if(1 != vols.size()) {
    MB_CHK_SET_ERR(MB_FAILURE, "Incorrect number of volumes found after loading the model");
  }
  //initalize the OBBTree (and the preconditioning datastructs)
  rval = dagmc->init_OBBTree();
  MB_CHK_SET_ERR(rval,"Could not initialize the OBBTree");

  rval = dagmc->build_preconditioner();
  MB_CHK_SET_ERR(rval,"Could not construct preconditioner.");
  //get the preconditioning box of interest
  box = dagmc->get_signed_distance_field(vols[0]);
  if (!box) return MB_ENTITY_NOT_FOUND;

  return rval;
};

// returns the nearest point on a sphere centered on the origin
CartVect nearest_on_sphere(CartVect point, double radius) {
  //exception for the origin
  if( point.length() == 0 ) {
    return radius*CartVect(1.0,0.0,0.0);
  }
  
  CartVect nearest_location = point;
  //get unit direction vector from origin
  nearest_location.normalize();
  //extend vector to sphere radius
  nearest_location *= radius;
  return nearest_location;
};

// returns the nearest point on a z-axis aligned cylinder centered on the origin
CartVect nearest_on_cylinder(CartVect point, double height, double radius) {
  CartVect nearest_location;

  // find point on nearest lid plane
  CartVect nearest_to_lids;
  if(point[2] >= 0) {
    nearest_to_lids = CartVect(point[0],point[1],height/2);
  }
  else {
    nearest_to_lids = CartVect(point[0],point[1],-height/2);
  }

  // find nearest point on an infinite barrel
  CartVect nearest_to_barrel = (point[0] == 0 && point[1] == 0) ? CartVect(1,1,0) : CartVect(point[0],point[1],0);
  nearest_to_barrel.normalize();
  nearest_to_barrel*=5;
  nearest_to_barrel[2] = point[2];

  if ( fabs(nearest_to_barrel[2]) > height/2.0 ) {
    nearest_to_barrel[2] = nearest_to_barrel[2] > 0.0 ? height/2.0 : -height/2.0;
  }

  // if (nearest_to_lids.length() < nearest_to_barrel.length()) {
  //   return nearest_to_lids;
  // }
  // else {
  //   return nearest_to_barrel;
  // }
        
  // check if point is inside or outside the barrel
  if(CartVect(point[0],point[1],0).length() <= radius) {
    // point is inside the barrel, then check for the minimum between the barrel and lids
    if((nearest_to_barrel-point).length() < (nearest_to_lids-point).length()) {
      nearest_location = nearest_to_barrel;
    }
    else {
      nearest_location = nearest_to_lids;
    }
  }
  else {
    // point is outside the barrel, return the inf barrel location
    nearest_location = nearest_to_barrel;
  }
  return nearest_location;
};

  
ErrorCode test_sphere(){

  ErrorCode rval;
  DagMC* dagmc;
  SignedDistanceField* box = NULL;

  char *filename = STRINGIFY(MESHDIR) "/sphere_rad5.h5m";
  rval = dag_init_file(filename, dagmc, box);
  MB_CHK_SET_ERR(rval,"Could not initalize DAGMC instance and retrieve preconditioner box");

  //get the signed distance field tag
  Tag sdfTag = dagmc->sdf_tag();
  int xints, yints, zints;
  box->get_dims(xints,yints,zints);
  for(unsigned int i = 0 ; i < xints; i++){
    for(unsigned int j = 0 ; j < yints; j++){
      for(unsigned int k = 0 ; k < zints; k++){

	//get the vertex coordinates
	CartVect vert_coords = box->get_coords(i,j,k);
	
	//get the distance to the nearest point on the sphere
	double sphere_radius = 5;
	CartVect expected_location = nearest_on_sphere(vert_coords, sphere_radius);
	double expected_distance = fabs((vert_coords-expected_location).length());	
	
	//retrieve the stored distance value of this vertex
	double distance;
	void *ptr = &distance;
	distance = box->get_data(i,j,k);
	distance = fabs(distance);
	
	//use facet tolerance as maximal error
	double facet_tol = dagmc->faceting_tolerance();
	//compare the values - they should be off by no more than the faceting tolerance
	if(fabs(expected_distance-distance) > facet_tol) {
	  std::cout << vert_coords << std::endl;
	  std::cout << expected_distance << std::endl;
	  std::cout << distance << std::endl;
	  MB_CHK_SET_ERR(MB_FAILURE, "Incorrect distance value found");	  
	}
	
      }
    }
  }
  return rval;
};

ErrorCode test_cylinder(){
  ErrorCode rval;
  DagMC* dagmc;
  SignedDistanceField* box = NULL;

  char *filename = STRINGIFY(MESHDIR) "/cyl_h5_r5.h5m";
  rval = dag_init_file(filename, dagmc, box);
  MB_CHK_SET_ERR(rval,"Could not initalize DAGMC instance and retrieve preconditioner box");

  //get the signed distance field tag
  Tag sdfTag = dagmc->sdf_tag();
  int xints, yints, zints;
  box->get_dims(xints,yints,zints);
  for(unsigned int i = 0 ; i < xints; i++){
    for(unsigned int j = 0 ; j < yints; j++){
      for(unsigned int k = 0 ; k < zints; k++){

	//get the vertex coordinates
	CartVect vert_coords = box->get_coords(i,j,k);

	//get the distance to the nearest point on the sphere
	double cylinder_radius = 5, cylinder_height = 5;
	CartVect expected_location = nearest_on_cylinder(vert_coords, cylinder_radius, cylinder_height);
	double expected_distance = fabs((vert_coords-expected_location).length());
	
	//retrieve the stored distance value of this vertex
	double distance = box->get_data(i,j,k);
	distance = fabs(distance);
	
	//use facet tolerance as maximal error
	double facet_tol = dagmc->faceting_tolerance();
	
	//compare the values - they should be off by no more than the faceting tolerance
	if(fabs(expected_distance-distance) > facet_tol) {
	  std::cout << std::endl;
	  std::cout <<  "[" << i << ", " << j <<  ", " << k << "]" << std::endl;
	  std::cout << vert_coords << std::endl;
	  std::cout << expected_location << std::endl;
	  std::cout << expected_distance << std::endl;
	  std::cout << distance << std::endl;
	  MB_CHK_SET_ERR(MB_FAILURE, "Incorrect distance value found");	  
	}
	
      }
    }
  }
  return rval;
};