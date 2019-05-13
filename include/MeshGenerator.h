// A simple, efficient and robust 3D mesh generator for VirtualCity@Chalmers.
// Copyright (C) 2018 Anders Logg.

#ifndef VC_MESH_GENERATOR_H
#define VC_MESH_GENERATOR_H

#include <iostream>
#include <vector>
#include <tuple>
#include <stack>
#include <cmath>

#include "CityModel.h"
#include "HeightMap.h"
#include "Timer.h"
#include "Point.h"
#include "Geometry.h"
#include "Mesh.h"
#include "CSV.h"

extern "C"
{
#include <triangle.h>
}

namespace VirtualCity
{

class MeshGenerator
{
public:

    // Generate 2D mesh. The mesh is a triangular mesh of a disc with
    // radius R centered at C with mesh size h. The edges of the mesh
    // respect the boundaries of the buildings and the domain markers
    // label the triangles inside building footprints as 0, 1, 2, etc
    // with outside triangles (ground) being labeled -1.
    static Mesh2D GenerateMesh2D(const CityModel& cityModel,
                                 double xMin, double yMin,
                                 double xMax, double yMax,
                                 double resolution)
    {
        std::cout << "MeshGenerator: Generating 2D mesh..." << std::endl;

        // Extract subdomains (building footprints)
        std::vector<std::vector<Point2D>> subDomains;
        for (auto const & building : cityModel.Buildings)
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
    // extruding the 2D mesh in the vertical (z) direction to height H
    // with mesh size h. Tetrahedra are added outside and above buildings
    // with building heights snapped to the closest layer. The domain
    // markers label tetrahedra just above buildings as 0, 1, 2, etc.
    // Tetrahedra just above ground level are labeled -1 and remaining
    // tetrahedra are labeld -2.
    static Mesh3D GenerateMesh3D(const Mesh2D& mesh2D,
                                 const CityModel& cityModel,
                                 double domainHeight,
                                 double meshResolution)
    {
        // Create empty 3D mesh
        Mesh3D mesh3D;

        // Compute number of layers
        const size_t numLayers = int(std::ceil(domainHeight / meshResolution));
        const double dz = domainHeight / double(numLayers);
        const size_t layerSize = mesh2D.Points.size();

        std::cout << "MeshGenerator: Generating 3D mesh with "
                  << numLayers << " layers..." << std::endl;

        // Create markers for used points
        const size_t numPoints = (numLayers + 1) * mesh2D.Points.size();
        std::vector<size_t> pointIndices(numPoints);
        std::fill(pointIndices.begin(), pointIndices.end(), numPoints);

        // Create markers for which triangles the first layer has been added
        std::vector<bool> firstLayerAdded(mesh2D.Cells.size());
        std::fill(firstLayerAdded.begin(), firstLayerAdded.end(), false);

        // Add tetrahedra for all layers
        size_t offset = 0;
        for (size_t layer = 0; layer < numLayers; layer++)
        {
            // Height of base of layer
            const double z = layer * dz;

            // Add tetrahedra for layer
            for (size_t i = 0; i < mesh2D.Cells.size(); i++)
            {
                // Set default marker for 3D cells
                int marker3D = (layer == 0 ? -1 : -2);

                // Check if we are inside a building
                const int marker2D = mesh2D.DomainMarkers[i];
                const bool inside = marker2D >= 0;

                // If inside a building, check height relative to grid
                if (inside)
                {
                    // Get building height
                    const Building& building = cityModel.Buildings[marker2D];
                    const double height = building.Height;

                    // Case 0: below building height, don't add cells
                    if (z + 0.5 * dz < height)
                        continue;

                    // Case 1: close to building, add cells and set marker
                    else if (!firstLayerAdded[i])
                    {
                        marker3D = marker2D;
                        firstLayerAdded[i] = true;
                    }

                    // Case 2: above building height, add cells
                    else
                    {
                        // Do nothing (add cells with default marker)
                    }
                }

                // // Get sorted vertex indices for bottom layer
                const size_t u0 = mesh2D.Cells[i].v0 + offset;
                const size_t u1 = mesh2D.Cells[i].v1 + offset;
                const size_t u2 = mesh2D.Cells[i].v2 + offset;

                // // Get sorted vertices for top layer
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
                mesh3D.DomainMarkers.push_back(-2); // not touching bottom
                mesh3D.DomainMarkers.push_back(-2); // not touching bottom

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
                const Point2D& p2D = mesh2D.Points[i % layerSize];
                const double z = (i / layerSize) * dz;
                Point3D p3D(p2D.x, p2D.y, z);
                mesh3D.Points.push_back(p3D);
            }
        }

        // Assign renumbered indices to cells
        for (auto & T : mesh3D.Cells)
        {
            T.v0 = pointIndices[T.v0];
            T.v1 = pointIndices[T.v1];
            T.v2 = pointIndices[T.v2];
            T.v3 = pointIndices[T.v3];
        }

        return mesh3D;
    }

private:

    // Call Triangle to compute 2D mesh
    static Mesh2D
    CallTriangle(const std::vector<Point2D>& boundary,
                 const std::vector<std::vector<Point2D>>& subDomains,
                 double h)
    {
        // Set area constraint to control mesh size (rough estimate)
        const double maxArea = h * h;

        // Set input switches for Triangle
        char triswitches[64];
        sprintf(triswitches, "zpq25a%.16f", maxArea);
        std::cout << "MeshGenerator: triangle parameters = "
                  << triswitches << std::endl;

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
        for (auto const & InnerPolygon : subDomains)
            NumberOfPoints += InnerPolygon.size();
        in.numberofpoints = NumberOfPoints;

        // Set points
        in.pointlist = new double[2 * NumberOfPoints];
        {
            size_t k = 0;
            for (auto const & p : boundary)
            {
                in.pointlist[k++] = p.x;
                in.pointlist[k++] = p.y;
            }
            for (auto const & InnerPolygon : subDomains)
            {
                for (auto const & p : InnerPolygon)
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

        // Create empty mesh
        Mesh2D mesh2D;

        // Extract points
        mesh2D.Points.reserve(out.numberofpoints);
        for (size_t i = 0; i < out.numberofpoints; i++)
        {
            Point2D p(out.pointlist[2 * i],
                      out.pointlist[2 * i + 1]);
            mesh2D.Points.push_back(p);
        }

        // Extract triangles
        mesh2D.Cells.reserve(out.numberoftriangles);
        for (size_t i = 0; i < out.numberoftriangles; i++)
        {
            Simplex2D t(out.trianglelist[3 * i],
                        out.trianglelist[3 * i + 1],
                        out.trianglelist[3 * i + 2]);
            mesh2D.Cells.push_back(t);
        }

        // Free memory
        //trifree(&out); // causes segfault
        delete [] in.pointlist;
        delete [] in.segmentlist;
        delete [] in.holelist;

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

    // Compute domain markers for subdomains
    static void ComputeDomainMarkers(Mesh2D& mesh, const CityModel& cityModel)
    {
        // Initialize markers
        mesh.DomainMarkers.reserve(mesh.Cells.size());

        // Iterate over cells
        for (auto const & Cell : mesh.Cells)
        {
            // Find building containg midpoint of cell (if any)
            const Point2D c = mesh.MidPoint(Cell);
            const int marker = cityModel.FindBuilding(c);

            // Set marker for subdomain
            mesh.DomainMarkers.push_back(marker);
        }
    }

};

}

#endif
