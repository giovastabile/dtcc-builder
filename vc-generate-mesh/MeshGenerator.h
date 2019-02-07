// A simple, efficient and robust 3D mesh generator for VirtualCity@Chalmers.
// Copyright (C) 2018 Anders Logg.

#ifndef MESH_GENERATION_H
#define MESH_GENERATION_H

#include <iostream>
#include <vector>
#include <tuple>
#include <stack>
#include <cmath>

#include "Timer.h"
#include "Point.h"
#include "Mesh.h"
#include "CSV.h"
#include "XML.h"

extern "C"
{
#include <triangle.h>
}

namespace VirtualCity
{
class MeshGenerator
{
public:

    // Generate 2D mesh
    static Mesh2D GenerateMesh2D()
    {
        // FIXME: Test data
        //-----------------------------------------------------
        std::vector<Point2D> Boundary;
        int n = 16;
        double R = 10.0;
        for (int i = 0; i < n; i++)
        {
            double x = R * std::cos(double(i) / n * 2.0 * M_PI);
            double y = R * std::sin(double(i) / n * 2.0 * M_PI);
            Boundary.push_back(Point2D(x, y));
        }

        std::vector<std::vector<Point2D>> SubDomains;

        std::vector<Point2D> b0;
        b0.push_back(Point2D(-2, -2));
        b0.push_back(Point2D(-1, -2));
        b0.push_back(Point2D(-1, -1));
        b0.push_back(Point2D(-2, -1));
        SubDomains.push_back(b0);

        std::vector<Point2D> b1;
        b1.push_back(Point2D(2, 3));
        b1.push_back(Point2D(3, 3));
        b1.push_back(Point2D(3, 4));
        b1.push_back(Point2D(2, 4));
        SubDomains.push_back(b1);
        //-----------------------------------------------------

        // Generate 2D mesh
        Mesh2D m2D = CallTriangle(Boundary, SubDomains);

        // Mark subdomains
        m2D.DomainMarkers = ComputeDomainMarkers(m2D, SubDomains);

        // FIXME: Write test output
        CSV::Write(m2D, "Mesh2D");
        XML::Write(m2D, "Mesh2D");

        std::cout << "Generated " << m2D << std::endl;

        return m2D;
    }

    // Generate 3D mesh
    static Mesh3D GenerateMesh3D()
    {
        // FIXME: Test data
        //-----------------------------------------------------
        double H = 10.0; // Height
        double h = 0.5;  // Vertical resolution
        //-----------------------------------------------------

        // Generate 2D mesh
        Mesh2D m2D = GenerateMesh2D();

        // Create empty 3D mesh
        Mesh3D m3D;

        // Compute number of layers
        double dz = h;
        const size_t NumberOfLayers = int(std::ceil(H / h));
        dz = H / double(NumberOfLayers);
        const size_t LayerSize = m2D.Points.size();

        std::cout << "Number of layers: " << NumberOfLayers << std::endl;
        std::cout << "Layer size: " << LayerSize << std::endl;

        // Create marker/index array for used points
        const size_t NumberOfPoints = (NumberOfLayers + 1) * m2D.Points.size();
        std::vector<size_t> PointIndices(NumberOfPoints);
        std::fill(PointIndices.begin(), PointIndices.end(), NumberOfPoints);

        // Add tetrahedra for all layers
        size_t Offset = 0;
        for (size_t Layer = 0; Layer < NumberOfLayers; Layer++)
        {
            // Add tetrahedra for layer
            for (size_t i = 0; i < m2D.Cells.size(); i++)
            {
                // FIXME: Temporary hack here
                // Check if cell is inside subdomain
                if (m2D.DomainMarkers[i] == 1 && Layer >= NumberOfLayers / 2)
                {
                    continue;
                }
                else if (m2D.DomainMarkers[i] == 2 && Layer >= 2 * NumberOfLayers / 3)
                {
                    continue;
                }

                // // Get sorted vertex indices for bottom layer
                const size_t u0 = m2D.Cells[i].v0 + Offset;
                const size_t u1 = m2D.Cells[i].v1 + Offset;
                const size_t u2 = m2D.Cells[i].v2 + Offset;

                // // Get sorted vertices for top layer
                const size_t v0 = u0 + LayerSize;
                const size_t v1 = u1 + LayerSize;
                const size_t v2 = u2 + LayerSize;

                // Create three tetrahedra by connecting the first vertex
                // of each edge in the bottom layer with the second
                // vertex of the corresponding edge in the top layer.
                m3D.Cells.push_back(Simplex3D(u0, u1, u2, v2));
                m3D.Cells.push_back(Simplex3D(u0, u1, v1, v2));
                m3D.Cells.push_back(Simplex3D(u0, v0, v1, v2));

                // Indicate which points are used
                PointIndices[u0] = 0;
                PointIndices[u1] = 0;
                PointIndices[u2] = 0;
                PointIndices[v0] = 0;
                PointIndices[v1] = 0;
                PointIndices[v2] = 0;
            }

            // Add to offset
            Offset += LayerSize;
        }

        // Renumber and count points
        size_t k = 0;
        for (size_t i = 0; i < NumberOfPoints; i++)
        {
            if (PointIndices[i] != NumberOfPoints)
                PointIndices[i] = k++;
        }

        std::cout << "k = " << k << std::endl;

        // Add points
        m3D.Points.reserve(k);
        for (size_t i = 0; i < NumberOfPoints; i++)
        {
            if (PointIndices[i] != NumberOfPoints)
            {
                const Point2D& p2D = m2D.Points[i % LayerSize];
                const double z = (i / LayerSize) * dz;
                Point3D p3D(p2D.x, p2D.y, z);
                m3D.Points.push_back(p3D);
            }
        }

        // Assign renumbered indices to cells
        for (auto & T : m3D.Cells)
        {
            T.v0 = PointIndices[T.v0];
            T.v1 = PointIndices[T.v1];
            T.v2 = PointIndices[T.v2];
            T.v3 = PointIndices[T.v3];
        }

        // FIXME: Write test output
        CSV::Write(m3D, "Mesh3D");
        XML::Write(m3D, "Mesh3D");

        std::cout << "Generated " << m3D << std::endl;

        return m3D;
    }

private:

    // Call Triangle to compute 2D mesh
    static Mesh2D
    CallTriangle(const std::vector<Point2D>& Boundary,
                 const std::vector<std::vector<Point2D>>& SubDomains)
    {
        // Set input switches for Triangle
        char triswitches[] = "zpq25";

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
        size_t NumberOfPoints = Boundary.size();
        for (auto const & InnerPolygon : SubDomains)
            NumberOfPoints += InnerPolygon.size();
        in.numberofpoints = NumberOfPoints;

        // Set points
        in.pointlist = new double[2 * NumberOfPoints];
        {
            size_t k = 0;
            for (auto const & p : Boundary)
            {
                in.pointlist[k++] = p.x;
                in.pointlist[k++] = p.y;
            }
            for (auto const & InnerPolygon : SubDomains)
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
            for (size_t j = 0; j < Boundary.size(); j++)
            {
                const size_t j0 = j;
                const size_t j1 = (j + 1) % Boundary.size();
                in.segmentlist[k++] = n + j0;
                in.segmentlist[k++] = n + j1;
            }
            n += Boundary.size();
            for (size_t i = 0; i < SubDomains.size(); i++)
            {
                for (size_t j = 0; j < SubDomains[i].size(); j++)
                {
                    const size_t j0 = j;
                    const size_t j1 = (j + 1) % SubDomains[i].size();
                    in.segmentlist[k++] = n + j0;
                    in.segmentlist[k++] = n + j1;
                }
                n += SubDomains[i].size();
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
        Mesh2D m2D;

        // Extract points
        m2D.Points.reserve(out.numberofpoints);
        for (size_t i = 0; i < out.numberofpoints; i++)
        {
            Point2D p(out.pointlist[2 * i],
                      out.pointlist[2 * i + 1]);
            m2D.Points.push_back(p);
        }

        // Extract triangles
        m2D.Cells.reserve(out.numberoftriangles);
        for (size_t i = 0; i < out.numberoftriangles; i++)
        {
            Simplex2D t(out.trianglelist[3 * i],
                        out.trianglelist[3 * i + 1],
                        out.trianglelist[3 * i + 2]);
            m2D.Cells.push_back(t);
        }

        // Free memory
        //trifree(&out);
        delete [] in.pointlist;
        delete [] in.segmentlist;
        delete [] in.holelist;

        return m2D;
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
    static std::vector<size_t>
    ComputeDomainMarkers(const Mesh2D& m,
                         const std::vector<std::vector<Point2D>>& SubDomains)
    {
        // Initialize markers
        std::vector<size_t> DomainMarkers;
        DomainMarkers.reserve(m.Cells.size());

        // Iterate over cells
        for (auto const & Cell : m.Cells)
        {
            // Compute midpoint of cell
            Point2D c = m.MidPoint(Cell);

            // Set default marker
            size_t Marker = 0;

            // Iterate over subdomains
            for (size_t i = 0; i < SubDomains.size(); i++)
            {
                // Compute total quadrant relative to subdomain. If the point
                // is inside the subdomain, the angle should be 4 (or -4).
                const int v = TotalQuadrantAngle2D(c, SubDomains[i]);

                // Check if point is inside the domain
                if (v != 0)
                {
                    Marker = i + 1;
                    break;
                }
            }

            // Set marker for subdomain
            DomainMarkers.push_back(Marker);
        }

        return DomainMarkers;
    }

    // Compute total quadrant angle of point p relative to polygon (2D)
    static int TotalQuadrantAngle2D(const Point2D& p,
                                    const std::vector<Point2D>& Polygon)
    {
        // Compute angle to first vertex
        Point2D q0 = Polygon[0];
        int v0 = QuadrantAngle2D(q0, p);

        // Sum up total angle
        int TotalAngle = 0;
        for (int i = 1; i < Polygon.size() + 1; i++)
        {
            // Compute angle increment
            Point2D q1 = Polygon[i % Polygon.size()];
            int v1 = QuadrantAngle2D(q1, p);
            int dv = v1 - v0;

            // Adjust angle increment for wrap-around
            if (dv == 3)
                dv = -1;
            else if (dv == -3)
                dv = 1;
            else if (dv == 2 || dv == -2)
            {
                double xx = q1.x - ((q1.y - p.y) * ((q0.x - q1.x) / (q0.y - q1.y)));
                if (xx > p.x)
                    dv = -dv;
            }

            // Add to total angle and update
            TotalAngle += dv;
            q0 = q1;
            v0 = v1;
        }

        return TotalAngle;
    }

    // Compute quadrant angle of point p relative to point q (2D)
    static int QuadrantAngle2D(const Point2D& p, const Point2D& q)
    {
        return ((p.x > q.x) ? ((p.y > q.y) ? 0 : 3) : ((p.y > q.y) ? 1 : 2));
    }

};

}

#endif
