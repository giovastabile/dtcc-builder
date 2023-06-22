// Copyright (C) 2023 George Spaias
// Licensed under the MIT License

#ifndef DTCC_SMOOTHER_H
#define DTCC_SMOOTHER_H

#include "BoundaryConditions.h"
#include "StiffnessMatrix.h"
#include "Timer.h"
#include "model/Mesh.h"

namespace DTCC_BUILDER
{

class Smoother
{
public:
  // Smooth mesh using Laplacian smoothing
  static void smooth_volume_mesh(VolumeMesh &volume_mesh,
                                 const City &city,
                                 const GridField &dem,
                                 double top_height,
                                 bool fix_buildings,
                                 size_t max_iterations,
                                 double relative_tolerance)
  {
    info("Smoother: Smoothing volume mesh...");
    info(volume_mesh.__str__());

    // Compute (local) stifness matrices
    StiffnessMatrix AK(volume_mesh);

    // Create solution vector and load vector
    std::vector<double> u(volume_mesh.Vertices.size(), 0);
    std::vector<double> b(volume_mesh.Vertices.size(), 0);

    // Apply boundary conditions
    BoundaryConditions bc(volume_mesh, city, dem, top_height, fix_buildings);
    bc.apply(AK);
    bc.apply(b);

    // Set initial guess
    if (!fix_buildings)
      set_initial_guess(u, volume_mesh, dem, top_height, bc);
    else
      u = b;

    // Solve linear system
    solve_unassembled_gauss_seidel(volume_mesh, AK, b, u, max_iterations,
                                   relative_tolerance);

    // Update mesh coordinates
    for (std::size_t i = 0; i < volume_mesh.Vertices.size(); i++)
      volume_mesh.Vertices[i].z += u[i];
  }

private:
  // Solve linear system using unassembled Gauss-Seidel iterations
  static void solve_unassembled_gauss_seidel(const VolumeMesh &volume_mesh,
                                             StiffnessMatrix &AK,
                                             std::vector<double> &b,
                                             std::vector<double> &u,
                                             const size_t max_iterations,
                                             const double relative_tolerance)
  {
    info("Smoother: Solving linear system using unassembled Gauss-Seidel");

    // Sum of non-diagonal elements
    std::vector<double> C(volume_mesh.Vertices.size());

    // Vertex indices of current cell
    std::array<uint, 4> I = {0};

    // Compute the number of cells that each vertex belongs
    std::vector<uint> vertex_degrees(volume_mesh.Vertices.size());
    std::vector<uint> _vertex_degrees(volume_mesh.Vertices.size());
    compute_vertex_degrees(vertex_degrees, volume_mesh);

    size_t iterations;
    double residual;
    for (iterations = 0; iterations < max_iterations; iterations++)
    {
      C = b;
      _vertex_degrees = vertex_degrees;
      residual = 0;
      for (size_t c = 0; c < volume_mesh.Cells.size(); c++)
      {
        I[0] = volume_mesh.Cells[c].v0;
        I[1] = volume_mesh.Cells[c].v1;
        I[2] = volume_mesh.Cells[c].v2;
        I[3] = volume_mesh.Cells[c].v3;
        for (u_int8_t i = 0; i < 4; i++)
        {
          C[I[i]] -=
              AK._data[c * 16 + i * 4 + (i + 1) % 4] * u[I[(i + 1) % 4]] +
              AK._data[c * 16 + i * 4 + (i + 2) % 4] * u[I[(i + 2) % 4]] +
              AK._data[c * 16 + i * 4 + (i + 3) % 4] * u[I[(i + 3) % 4]];

          _vertex_degrees[I[i]]--;
          if (_vertex_degrees[I[i]] == 0)
          {
            double res = u[I[i]];
            u[I[i]] = C[I[i]] / AK.diagonal[I[i]];
            res = std::abs(res - u[I[i]]);
            residual = std::max(residual, res);
          }
        }
      }

      // Check convergance
      if (residual < relative_tolerance)
        break;
    }

    info("Smoother: Converged in " + str(iterations) + "/" +
         str(max_iterations) + " iterations with residual " + str(residual));
  }

  // Set initial guess for solution vector
  static void set_initial_guess(std::vector<double> &u,
                                const VolumeMesh &volume_mesh,
                                const GridField &dem,
                                double top_height,
                                BoundaryConditions &bc)
  {
    info("Smoother: Setting initial guess for solution vector");

    for (size_t i = 0; i < volume_mesh.Vertices.size(); i++)
    {
      if (bc.vertex_markers[i] == -4)
      {
        const Vector2D p(volume_mesh.Vertices[i].x, volume_mesh.Vertices[i].y);
        u[i] = dem(p) * (1 - volume_mesh.Vertices[i].z / top_height);
      }
      else
        u[i] = 0.0;
    }
  }

  // Compute the number of cells to which each vertex belongs
  static void compute_vertex_degrees(std::vector<uint> &vertex_degrees,
                                     const VolumeMesh &volume_mesh)
  {
    for (size_t c = 0; c < volume_mesh.Cells.size(); c++)
    {
      vertex_degrees[volume_mesh.Cells[c].v0]++;
      vertex_degrees[volume_mesh.Cells[c].v1]++;
      vertex_degrees[volume_mesh.Cells[c].v2]++;
      vertex_degrees[volume_mesh.Cells[c].v3]++;
    }
  }
};

} // namespace DTCC_BUILDER

#endif // DTCC_LAPLACIAN_SMOOTHER_NEW_H
