// A simple, efficient and robust 3D mesh generator for DTCC@Chalmers.
// Copyright (C) 2018 Anders Logg.

#ifndef DTCC_MESH_GENERATOR_H
#define DTCC_MESH_GENERATOR_H

#include <cmath>
#include <iostream>
#include <stack>
#include <tuple>
#include <vector>

#include "CSV.h"
#include "CityModel.h"
#include "Geometry.h"
#include "HeightMap.h"
#include "Mesh.h"
#include "Point.h"
#include "Surface.h"
#include "Timer.h"

extern "C"
{
#include <triangle.h>
}

namespace DTCC
{

class MeshGenerator
{
public:
  // Generate 2D mesh. The mesh is a triangular mesh of the rectangular region
  // defined by (xMin, xMax) x (yMin, yMax). The edges of the mesh respect the
  // boundaries of the buildings.
  //
  // The domain markers label the triangles inside building footprints
  // with the number of the building (0, 1, 2, ...). Triangles that are
  // neighbors of building triangles (just outside buildings) are marked
  // as -1 and the remaining triangles (the ground) are marked as -2.
  static Mesh2D GenerateMesh2D(const CityModel &cityModel,
                               double xMin,
                               double yMin,
                               double xMax,
                               double yMax,
                               double resolution)
  {
    std::cout << "MeshGenerator: Generating 2D mesh..." << std::endl;

    // Extract subdomains (building footprints)
    std::vector<std::vector<Point2D>> subDomains;
    for (auto const &building : cityModel.Buildings)
      subDomains.push_back(building.Footprint.Points);

    // Generate boundary
    std::vector<Point2D> boundary;
    boundary.push_back(Point2D(xMin, yMin));
    boundary.push_back(Point2D(xMax, yMin));
    boundary.push_back(Point2D(xMax, yMax));
    boundary.push_back(Point2D(xMin, yMax));

    // Generate 2D mesh
    Mesh2D mesh2D = CallTriangle(boundary, subDomains, resolution);

    // Mark subdomains
    ComputeDomainMarkers(mesh2D, cityModel);

    return mesh2D;
  }

  // Generate 3D mesh. The mesh is a tetrahedral mesh generated by
  // extruding the 2D mesh in the vertical (z) direction to a height
  // determined by domainHeight and mesh size determined by meshResolution.
  // Tetrahedra are added outside and above buildings with building heights
  // snapped to the closest layer.
  //
  // The domain markers label the tetrahedra touching the roofs of buildings
  // (inside footprint) with the number of the building (0, 1, 2, ...).
  // Tetrahedra touching the ground and being neighbors of buildings are
  // marked as -1, tetrahedra touching the ground but not neighboring a
  // building are marked as -2, and remaining tetrahedra are marked as -3.
  static Mesh3D GenerateMesh3D(const Mesh2D &mesh2D,
                               const CityModel &cityModel,
                               double groundElevation,
                               double domainHeight,
                               double meshResolution)
  {
    // Create empty 3D mesh
    Mesh3D mesh3D;

    // Compute number of layers
    const size_t numLayers = int(std::ceil(domainHeight / meshResolution));
    const double dz = domainHeight / double(numLayers);
    const size_t layerSize = mesh2D.Points.size();

    std::cout << "MeshGenerator: Generating 3D mesh with " << numLayers
              << " layers..." << std::endl;

    // Create markers for used points
    const size_t numPoints = (numLayers + 1) * mesh2D.Points.size();
    std::vector<size_t> pointIndices(numPoints);
    std::fill(pointIndices.begin(), pointIndices.end(), numPoints);

    // Create markers for which triangles the first layer has been added
    std::vector<bool> firstLayerAdded(mesh2D.Cells.size());
    std::fill(firstLayerAdded.begin(), firstLayerAdded.end(), false);

    // Iterate over layers
    size_t offset = 0;
    for (size_t layer = 0; layer < numLayers; layer++)
    {
      // Compute height of base of layer
      const double z = layer * dz + groundElevation;

      // Iterate over triangles in layer
      for (size_t i = 0; i < mesh2D.Cells.size(); i++)
      {
        // Get 2D domain marker
        const int marker2D = mesh2D.DomainMarkers[i];

        // Set 3D domain marker
        const int marker3D = (firstLayerAdded[i] ? -3 : marker2D);

        // If inside a building, check height relative to grid
        const bool inside = marker2D >= 0;
        if (inside)
        {
          // Get building height
          const Building &building = cityModel.Buildings[marker2D];
          const double height = building.Height;

          // Note: || layer == 0 ensures that we skip at least one
          // layer for each building

          // Move to next layer of below building height
          if (z + 0.5 * dz < height || layer == 0)
            continue;
        }

        // Mark first layer as added
        firstLayerAdded[i] = true;

        // Get sorted vertex indices for bottom layer
        const size_t u0 = mesh2D.Cells[i].v0 + offset;
        const size_t u1 = mesh2D.Cells[i].v1 + offset;
        const size_t u2 = mesh2D.Cells[i].v2 + offset;

        // Get sorted vertices for top layer
        const size_t v0 = u0 + layerSize;
        const size_t v1 = u1 + layerSize;
        const size_t v2 = u2 + layerSize;

        // Create three tetrahedra by connecting the first vertex
        // of each edge in the bottom layer with the second
        // vertex of the corresponding edge in the top layer.
        mesh3D.Cells.push_back(Simplex3D(u0, u1, u2, v2));
        mesh3D.Cells.push_back(Simplex3D(u0, v1, u1, v2));
        mesh3D.Cells.push_back(Simplex3D(u0, v0, v1, v2));

        // Set domain markers
        mesh3D.DomainMarkers.push_back(marker3D);
        mesh3D.DomainMarkers.push_back(-3); // not touching bottom
        mesh3D.DomainMarkers.push_back(-3); // not touching bottom

        // Indicate which points are used
        pointIndices[u0] = 0;
        pointIndices[u1] = 0;
        pointIndices[u2] = 0;
        pointIndices[v0] = 0;
        pointIndices[v1] = 0;
        pointIndices[v2] = 0;
      }

      // Add to offset
      offset += layerSize;
    }

    // Renumber and count points
    size_t k = 0;
    for (size_t i = 0; i < numPoints; i++)
    {
      if (pointIndices[i] != numPoints)
        pointIndices[i] = k++;
    }

    // Add points
    mesh3D.Points.reserve(k);
    for (size_t i = 0; i < numPoints; i++)
    {
      if (pointIndices[i] != numPoints)
      {
        const Point2D &p2D = mesh2D.Points[i % layerSize];
        const double z = (i / layerSize) * dz + groundElevation;
        Point3D p3D(p2D.x, p2D.y, z);
        mesh3D.Points.push_back(p3D);
      }
    }

    // Assign renumbered indices to cells
    for (auto &T : mesh3D.Cells)
    {
      T.v0 = pointIndices[T.v0];
      T.v1 = pointIndices[T.v1];
      T.v2 = pointIndices[T.v2];
      T.v3 = pointIndices[T.v3];
    }

    return mesh3D;
  }

  // Generate 3D surface meshes for visualization. The first surface is
  // the ground (height map) and the remaining surfaces are the extruded
  // building footprints. Note that meshes are non-conforming; the ground
  // and building meshes are non-matching and the building meshes may
  // contain hanging nodes.
  static std::vector<Surface3D> GenerateSurfaces3D(const CityModel &cityModel,
      const HeightMap &heightMap,
      double xMin,
      double yMin,
      double xMax,
      double yMax,
      double resolution,
      bool flatGround)
  {
    std::cout << "MeshGenerator: Generating 3D surface meshes..." << std::endl;

    // Create empty list of surfaces
    std::vector<Surface3D> surfaces;

    // Generate empty subdomains for Triangle mesh generation
    std::vector<std::vector<Point2D>> subDomains;

    // Generate boundary for Triangle mesh generation
    std::vector<Point2D> boundary;
    boundary.push_back(Point2D(xMin, yMin));
    boundary.push_back(Point2D(xMax, yMin));
    boundary.push_back(Point2D(xMax, yMax));
    boundary.push_back(Point2D(xMin, yMax));

    // Generate 2D mesh of domain
    std::cout << "MeshGenerator: Generating ground mesh" << std::endl;
    Mesh2D mesh2D = CallTriangle(boundary, subDomains, resolution);

    // Compute domain markers
    ComputeDomainMarkers(mesh2D, cityModel);

    // Create ground surface with zero height
    Surface3D surface3D;
    surface3D.Cells = mesh2D.Cells;
    surface3D.Points.resize(mesh2D.Points.size());
    for (size_t i = 0; i < mesh2D.Points.size(); i++)
    {
      const Point2D &p2D = mesh2D.Points[i];
      Point3D p3D(p2D.x, p2D.y, 0.0);
      surface3D.Points[i] = p3D;
    }

    // Displace ground surface
    std::cout << "MeshGenerator: Displacing ground mesh" << std::endl;
    if (flatGround)
    {
      // If ground is flat, just iterate over vertices and set height
      const double z = heightMap.Min();
      for (size_t i = 0; i < mesh2D.Points.size(); i++)
        surface3D.Points[i].z = z;
    }
    else
    {
      // Fill all points with maximum height. This is used to
      // always choose the smallest height for each point since
      // each point may be visited multiple times.
      const double zMax = heightMap.Max();
      for (size_t i = 0; i < mesh2D.Points.size(); i++)
        surface3D.Points[i].z = zMax;

      // If ground is not float, iterate over the triangles
      for (size_t i = 0; i < mesh2D.Cells.size(); i++)
      {
        // Get cell marker
        const int cellMarker = mesh2D.DomainMarkers[i];

        // Get triangle
        const Simplex2D& T = mesh2D.Cells[i];

        // Check cell marker
        if (cellMarker != -2) // not ground
        {
          // Compute minimum height of vertices
          double zMin = std::numeric_limits<double>::max();
          zMin = std::min(zMin, heightMap(mesh2D.Points[T.v0]));
          zMin = std::min(zMin, heightMap(mesh2D.Points[T.v1]));
          zMin = std::min(zMin, heightMap(mesh2D.Points[T.v2]));

          // Set minimum height for all vertices
          setMin(surface3D.Points[T.v0].z, zMin);
          setMin(surface3D.Points[T.v1].z, zMin);
          setMin(surface3D.Points[T.v2].z, zMin);
        }
        else
        {
          // Sample height map at vertex position for all vertices
          setMin(surface3D.Points[T.v0].z, heightMap(mesh2D.Points[T.v0]));
          setMin(surface3D.Points[T.v1].z, heightMap(mesh2D.Points[T.v1]));
          setMin(surface3D.Points[T.v2].z, heightMap(mesh2D.Points[T.v2]));
        }
      }
    }

    // Add ground surface to array of surfaces
    surfaces.push_back(surface3D);

    // Get ground height (minimum)
    const double groundHeight = heightMap.Min();

    // Iterate over buildings to generate surfaces
    std::cout << "MeshGenerator: Generating building meshes" << std::endl;
    for (auto const &building : cityModel.Buildings)
    {
      // Generate 2D mesh of building footprint
      Mesh2D mesh2D =
        CallTriangle(building.Footprint.Points, subDomains, resolution);

      // Create empty 3D surface
      Surface3D surface3D;

      // Note: The generated 2D mesh contains all the input boundary
      // points with the same numbers as in the footprint polygon, but
      // may also contain new points (Steiner points) added during
      // mesh generation. We add the top points (including any Steiner
      // points) first, then the points at the bottom (the footprint).

      // Set height of building
      const double buildingHeight = building.Height;

      // Set total number of points
      const size_t numMeshPoints = mesh2D.Points.size();
      const size_t numBoundaryPoints = building.Footprint.Points.size();
      surface3D.Points.resize(numMeshPoints + numBoundaryPoints);

      // Set total number of triangles
      const size_t numMeshTriangles = mesh2D.Cells.size();
      const size_t numBoundaryTriangles = 2 * numBoundaryPoints;
      surface3D.Cells.resize(numMeshTriangles + numBoundaryTriangles);

      // Add points at top
      for (size_t i = 0; i < numMeshPoints; i++)
      {
        const Point2D &p2D = mesh2D.Points[i];
        const Point3D p3D(p2D.x, p2D.y, buildingHeight);
        surface3D.Points[i] = p3D;
      }

      // Add points at bottom
      for (size_t i = 0; i < numBoundaryPoints; i++)
      {
        const Point2D &p2D = mesh2D.Points[i];
        const Point3D p3D(p2D.x, p2D.y, groundHeight);
        surface3D.Points[numMeshPoints + i] = p3D;
      }

      // Add triangles on top
      for (size_t i = 0; i < numMeshTriangles; i++)
        surface3D.Cells[i] = mesh2D.Cells[i];

      // Add triangles on boundary
      for (size_t i = 0; i < numBoundaryPoints; i++)
      {
        const size_t v0 = i;
        const size_t v1 = (i + 1) % numBoundaryPoints;
        const size_t v2 = v0 + numMeshPoints;
        const size_t v3 = v1 + numMeshPoints;
        Simplex2D t0(v0, v2, v1); // Outward-pointing normal
        Simplex2D t1(v1, v2, v3); // Outward-pointing normal
        surface3D.Cells[numMeshTriangles + 2 * i] = t0;
        surface3D.Cells[numMeshTriangles + 2 * i + 1] = t1;
      }

      // Add surface
      surfaces.push_back(surface3D);
    }

    return surfaces;
  }

private:
// Call Triangle to compute 2D mesh
  static Mesh2D
  CallTriangle(const std::vector<Point2D> &boundary,
               const std::vector<std::vector<Point2D>> &subDomains,
               double h)
  {
    // Set area constraint to control mesh size
    const double maxArea = 0.5 * h * h;

    // Set input switches for Triangle
    char triswitches[64];
    sprintf(triswitches, "zpq25a%.16f", maxArea);
    std::cout << "MeshGenerator: triangle parameters = " << triswitches
              << std::endl;

    // z = use zero-based numbering
    // p = use polygon input (segments)
    // q = control mesh quality
    //
    // Note that the minimum angle (here 25) should be
    // as large as possible for high quality meshes but
    // it should be less than 28.6 degrees to guarantee
    // that Triangle terminates. Default is 20 degrees.

    // Create input data structure for Triangle
    struct triangulateio in = CreateTriangleIO();

    // Set number of points
    size_t NumberOfPoints = boundary.size();
    for (auto const &InnerPolygon : subDomains)
      NumberOfPoints += InnerPolygon.size();
    in.numberofpoints = NumberOfPoints;

    // Set points
    in.pointlist = new double[2 * NumberOfPoints];
    {
      size_t k = 0;
      for (auto const &p : boundary)
      {
        in.pointlist[k++] = p.x;
        in.pointlist[k++] = p.y;
      }
      for (auto const &InnerPolygon : subDomains)
      {
        for (auto const &p : InnerPolygon)
        {
          in.pointlist[k++] = p.x;
          in.pointlist[k++] = p.y;
        }
      }
    }

    // Set number of segments
    const size_t NumberOfSegments = NumberOfPoints;
    in.numberofsegments = NumberOfSegments;

    // Set segments
    in.segmentlist = new int[2 * NumberOfSegments];
    {
      size_t k = 0;
      size_t n = 0;
      for (size_t j = 0; j < boundary.size(); j++)
      {
        const size_t j0 = j;
        const size_t j1 = (j + 1) % boundary.size();
        in.segmentlist[k++] = n + j0;
        in.segmentlist[k++] = n + j1;
      }
      n += boundary.size();
      for (size_t i = 0; i < subDomains.size(); i++)
      {
        for (size_t j = 0; j < subDomains[i].size(); j++)
        {
          const size_t j0 = j;
          const size_t j1 = (j + 1) % subDomains[i].size();
          in.segmentlist[k++] = n + j0;
          in.segmentlist[k++] = n + j1;
        }
        n += subDomains[i].size();
      }
    }

    // Note: This is how set holes but it's not used here since we
    // need the triangles for the interior *above* the buildings.

    /*
    // Set number of holes
    const size_t NumberOfHoles = SubDomains.size();
    in.numberofholes = NumberOfHoles;

    // Set holes. Note that we assume that we can get an
    // interior point of each hole (inner polygon) by computing
    // its center of mass.
    in.holelist = new double[2 * NumberOfHoles];
    {
    size_t k = 0;
    Point2D c;
    for (auto const & InnerPolygon : SubDomains)
    {
    for (auto const & p : InnerPolygon)
    {
    c += p;
    }
    c /= InnerPolygon.size();
    in.holelist[k++] = c.x;
    in.holelist[k++] = c.y;
    }
    }
    */

    // Prepare output data for Triangl;e
    struct triangulateio out = CreateTriangleIO();
    struct triangulateio vorout = CreateTriangleIO();

    // Call Triangle
    triangulate(triswitches, &in, &out, &vorout);

    // Uncomment for debugging
    //PrintTriangleIO(out);
    //std::cout << std::endl;
    //PrintTriangleIO(vorout);

    // Create empty mesh
    Mesh2D mesh2D;

    // Extract points
    mesh2D.Points.reserve(out.numberofpoints);
    for (int i = 0; i < out.numberofpoints; i++)
    {
      Point2D p(out.pointlist[2 * i], out.pointlist[2 * i + 1]);
      mesh2D.Points.push_back(p);
    }

    // Extract triangles
    mesh2D.Cells.reserve(out.numberoftriangles);
    for (int i = 0; i < out.numberoftriangles; i++)
    {
      Simplex2D t(out.trianglelist[3 * i],
                  out.trianglelist[3 * i + 1],
                  out.trianglelist[3 * i + 2]);
      mesh2D.Cells.push_back(t);
    }

    // Free memory
    // trifree(&out); // causes segfault
    delete[] in.pointlist;
    delete[] in.segmentlist;
    delete[] in.holelist;

    return mesh2D;
  }

  // Create and reset Triangle I/O data structure
  static struct triangulateio CreateTriangleIO()
  {
    struct triangulateio io;

    io.pointlist = 0;
    io.pointmarkerlist = 0;
    io.pointmarkerlist = 0;
    io.numberofpoints = 0;
    io.numberofpointattributes = 0;
    io.trianglelist = 0;
    io.triangleattributelist = 0;
    io.trianglearealist = 0;
    io.neighborlist = 0;
    io.numberoftriangles = 0;
    io.numberofcorners = 0;
    io.numberoftriangleattributes = 0;
    io.segmentlist = 0;
    io.segmentmarkerlist = 0;
    io.numberofsegments = 0;
    io.holelist = 0;
    io.numberofholes = 0;
    io.regionlist = 0;
    io.numberofregions = 0;
    io.edgelist = 0;
    io.edgemarkerlist = 0;
    io.normlist = 0;
    io.numberofedges = 0;

    return io;
  }

  // Print triangle I/O data
  static void PrintTriangleIO(const struct triangulateio& io)
  {
    std::cout << "Triangle I/O data: " << std::endl;
    std::cout << "  pointlist = " << io.pointlist << std::endl;
    std::cout << "  pointmarkerlist = " << io.pointmarkerlist << std::endl;
    if (io.pointmarkerlist)
    {
      std::cout << "   ";
      for (int i = 0; i < io.numberofpoints; i++)
        std::cout << " " << io.pointmarkerlist[i];
      std::cout << std::endl;
    }
    std::cout << "  numberofpoints = " << io.numberofpoints << std::endl;
    std::cout << "  numberofpointattributes = " << io.numberofpointattributes << std::endl;
    std::cout << "  trianglelist = " << io.trianglelist << std::endl;
    std::cout << "  triangleattributelist = " << io.triangleattributelist << std::endl;
    std::cout << "  trianglearealist = " << io.trianglearealist << std::endl;
    std::cout << "  neighborlist = " << io.neighborlist << std::endl;
    std::cout << "  numberoftriangles = " << io.numberoftriangles << std::endl;
    std::cout << "  numberofcorners = " << io.numberofcorners << std::endl;
    std::cout << "  numberoftriangleattributes = " << io.numberoftriangleattributes << std::endl;
    std::cout << "  segmentlist = " << io.segmentlist << std::endl;
    std::cout << "  segmentmarkerlist = " << io.segmentmarkerlist << std::endl;
    if (io.segmentmarkerlist)
    {
      std::cout << "   ";
      for (int i = 0; i < io.numberofsegments; i++)
        std::cout << " " << io.segmentmarkerlist[i];
      std::cout << std::endl;
    }
    std::cout << "  numberofsegments = " << io.numberofsegments << std::endl;
    std::cout << "  holelist = " << io.holelist << std::endl;
    std::cout << "  numberofholes = " << io.numberofholes << std::endl;
    std::cout << "  regionlist = " << io.regionlist << std::endl;
    std::cout << "  numberofregions = " << io.numberofregions << std::endl;
    std::cout << "  edgelist = " << io.edgelist << std::endl;
    std::cout << "  edgemarkerlist = " << io.edgemarkerlist << std::endl;
    std::cout << "  normlist = " << io.normlist << std::endl;
    std::cout << "  numberofedges = " << io.numberofedges << std::endl;
  }

  // Compute domain markers for subdomains
  static void ComputeDomainMarkers(Mesh2D & mesh, const CityModel & cityModel)
  {
    std::cout << "MeshGenerator: Computing domain markers" << std::endl;

    // Initialize domain markers and set all markers to -2 (ground)
    mesh.DomainMarkers.resize(mesh.Cells.size());
    std::fill(mesh.DomainMarkers.begin(), mesh.DomainMarkers.end(), -2);

    // Initialize markers for vertices belonging to a building
    std::vector<bool> isBuildingVertex(mesh.Points.size());
    std::fill(isBuildingVertex.begin(), isBuildingVertex.end(), false);

    // Iterate over cells to mark buildings
    for (size_t i = 0; i < mesh.Cells.size(); i++)
    {
      // Find building containg midpoint of cell (if any)
      const Point2D c = mesh.MidPoint(mesh.Cells[i]);
      const int marker = cityModel.FindBuilding(c);

      // Get triangle
      const Simplex2D &T = mesh.Cells[i];

      // Check if we are inside a building
      if (marker >= 0)
      {
        // Set domain marker to building number
        mesh.DomainMarkers[i] = marker;

        // Mark all cell vertices as belonging to a building
        isBuildingVertex[T.v0] = true;
        isBuildingVertex[T.v1] = true;
        isBuildingVertex[T.v2] = true;
      }

      // Check if individual vertices are inside a building
      // (not only midpoint). Necessary for when generating
      // visualization meshes that are not boundary-fitted.
      if (cityModel.FindBuilding(mesh.Points[T.v0]))
        isBuildingVertex[T.v0] = true;
      if (cityModel.FindBuilding(mesh.Points[T.v1]))
        isBuildingVertex[T.v1] = true;
      if (cityModel.FindBuilding(mesh.Points[T.v2]))
        isBuildingVertex[T.v2] = true;
    }

    // Iterate over cells to mark building halos
    for (size_t i = 0; i < mesh.Cells.size(); i++)
    {
      // Check if any of the cell vertices belongs to a building
      const Simplex2D &T = mesh.Cells[i];
      const bool touchesBuilding =
        (isBuildingVertex[T.v0] || isBuildingVertex[T.v1] ||
         isBuildingVertex[T.v2]);

      // Mark as halo (-1) if the cell touches a building but is not
      // itself inside footprint (not marked in the previous step)
      if (touchesBuilding && mesh.DomainMarkers[i] == -2)
        mesh.DomainMarkers[i] = -1;
    }
  }

  // Set x = min(x, y)
  static void setMin(double& x, double y)
  {
    if (y < x)
      x = y;
  }
};

} // namespace DTCC

#endif
