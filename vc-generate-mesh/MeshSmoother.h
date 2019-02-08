// PDE based mesh smoothing
// Copyright (C) 2018 Anders Logg.

// Developer note: The implementation of this class does not follow
// UE4 Coding Standard since the code makes heavy use of FEniCS/DOLFIN
// which uses a quite different coding standard and the mix looks horrible.

#ifndef MESH_SMOOTHER_H
#define MESH_SMOOTHER_H

#include <iostream>
#include <cmath>
#include <dolfin.h>

#include "LaplacianSmoother.h"

namespace VirtualCity
{

class MeshSmoother
{
public:

    // Smooth mesh using default method (Laplacian smoothing)
    static void SmoothMesh()
    {
        SmoothMeshLaplacian();
    }

    // Smooth mesh using Laplacian smoothing
    static void SmoothMeshLaplacian()
    {
        std::cout << "Smoothing mesh (Laplacian smoothing)..." << std::endl;

        // FIXME: Test data
        //-----------------------------------------------------
        auto mesh = std::make_shared<dolfin::Mesh>("Mesh3D.xml");
        //-----------------------------------------------------

        // Create function space and bilinear form
        auto V = std::make_shared<LaplacianSmoother::FunctionSpace>(mesh);
        auto a = std::make_shared<LaplacianSmoother::BilinearForm>(V, V);

        // Assemble matrix
        auto A = std::make_shared<dolfin::Matrix>();
        dolfin::assemble(*A, *a);

        // Initialize vectors
        auto x = std::make_shared<dolfin::Vector>();
        auto b = std::make_shared<dolfin::Vector>();
        A->init_vector(*x, 0);
        A->init_vector(*b, 0);

        // Create boundary conditions
        auto bc0 = std::make_shared<dolfin::DirichletBC>
                   (V,
                    std::make_shared<dolfin::Constant>(0),
                    std::make_shared<Top>());
        auto bcz = std::make_shared<dolfin::DirichletBC>
                   (V,
                    std::make_shared<HeightMap>(),
                    std::make_shared<Bottom>());

        // Apply boundary conditions
        bc0->apply(*A, *b);
        bcz->apply(*A, *b);

        // Create linear solver
        dolfin::KrylovSolver solver(mesh->mpi_comm(), "bicgstab", "amg");
        solver.parameters["nonzero_initial_guess"] = true;
        solver.set_operator(A);

        // Solve linear system
        *x = *b;
        solver.solve(*x, *b);

        // Get coordinate displacements
        const std::vector<dolfin::la_index> v2d = vertex_to_dof_map(*V);
        const size_t num_vertices = mesh->num_vertices();
        std::vector<double> displacements(num_vertices);
        x->get_local(displacements.data(), num_vertices, v2d.data());

        // Update mesh coordinates
        double coordinates[3];
        for (std::size_t i = 0; i < num_vertices; i++)
        {
            coordinates[0] = mesh->geometry().x(i, 0);
            coordinates[1] = mesh->geometry().x(i, 1);
            coordinates[2] = mesh->geometry().x(i, 2) + displacements[i];
            mesh->geometry().set(i, coordinates);
        }

        // FIXME: Write test output
        dolfin::BoundaryMesh _b(*mesh, "exterior");
        dolfin::File _f("Mesh3DSmoothed.pvd");
        dolfin::File _g("Mesh3DBoundarySmoothed.pvd");
        _f << *mesh;
        _g << _b;

        dolfin::File f("Mesh3DSmoothed.pvd");
        f << *mesh;
    }

    static void SmoothMeshElastic()
    {
        std::cout << "Elastic smoothing not (yet) implemented." << std::endl;
    }

private:

    // Boundary definition for top
    class Top : public dolfin::SubDomain
    {
        bool inside(const dolfin::Array<double>& x, bool on_boundary) const
        {
            // FIXME: Test data
            return std::abs(x[2] - 10.0) < DOLFIN_EPS;
        }
    };

    // Boundary definition for bottom
    class Bottom : public dolfin::SubDomain
    {
        bool inside(const dolfin::Array<double>& x, bool on_boundary) const
        {
            // FIXME: Test data
            return std::abs(x[2] - 0.0) < DOLFIN_EPS;
        }
    };

    // Boundary value for height map
    class HeightMap : public dolfin::Expression
    {
        void eval(dolfin::Array<double>& values,
                  const dolfin::Array<double>& x) const
        {
            // FIXME: Test data
            values[0] = 0.5 * std::sin(x[0]) + 0.25 * std::cos(2.5 * x[1]);
        }
    };

};

}

#endif
