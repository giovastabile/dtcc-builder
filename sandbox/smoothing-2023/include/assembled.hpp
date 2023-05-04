#ifndef ASSEMBLED_H
#define ASSEMBLED_H

#include <algorithm>
#include <chrono>
#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

#include "GridField.h"
#include "Mesh.h"
#include "Timer.h"
#include "datamodel/CityModel.h"

#include "boundaryConditions.hpp"
#include "error.hpp"
#include "sparse.hpp"
#include "stiffnessMatrix.hpp"

using std::chrono::duration;
using std::chrono::duration_cast;
using std::chrono::high_resolution_clock;
using std::chrono::milliseconds;

// Test function: Naive BC Application
void checkBoundaryPoints(bool *isBoundary,
                         double *boundaryValues,
                         const Mesh3D &m)
{
  const size_t nV = m.Vertices.size();
  const double epsilon = 1;
  const double boundary_line1 = 37.0;
  const double boundary_line2 = 100.0;

  for (size_t i = 0; i < nV; i++)
  {
    if ((abs(m.Vertices[i].z - boundary_line2) <
         epsilon)) //|| (abs(m->Vertices[i].z - boundary_line2) < epsilon))
    {
      isBoundary[i] = true;
      boundaryValues[i] = 3;
    }
  }
  return;
}

void assembleSparse(const Mesh3D &mesh,
                    double *A,
                    bool *isBoundary,
                    COO_array *assembled_A)
{
  size_t nC = mesh.Cells.size();
  size_t nV = mesh.Vertices.size();

  // double *assembled_A = new double[nV * nV];
  // std::cout << "\nNumber of Verticess: " << m.Vertices.size() << std::endl;
  std::array<uint, 4> I;

  for (size_t cn = 0; cn < nC; cn++)
  {
    I[0] = mesh.Cells[cn].v0;
    I[1] = mesh.Cells[cn].v1;
    I[2] = mesh.Cells[cn].v2;
    I[3] = mesh.Cells[cn].v3;

    for (size_t i = 0; i < 4; i++)
    {
      if (!isBoundary[I[i]])
      {
        assembled_A->add(I[i], I[0], A[16 * cn + i * 4 + 0]);
        assembled_A->add(I[i], I[1], A[16 * cn + i * 4 + 1]);
        assembled_A->add(I[i], I[2], A[16 * cn + i * 4 + 2]);
        assembled_A->add(I[i], I[3], A[16 * cn + i * 4 + 3]);
      }
    }
  }

  for (size_t i = 0; i < nV; i++)
  {
    if (isBoundary[i])
    {
      assembled_A->add(i, i, 1);
    }
  }
}

void assembled_GaussSeidel(const Mesh3D &mesh,
                           const CityModel &citymodel,
                           const GridField2D &dtm,
                           const size_t max_iterations)
{
  info("\nMesh Smoothing with Assembled Stiffness Matrix GS Method");

  const size_t nC = mesh.Cells.size();
  const size_t nV = mesh.Vertices.size();

  // Check Boundary Vertices
  bool *isBoundary = new bool[nV]();
  double *b = new double[nV]();

  checkBoundaryPoints(isBoundary, b, mesh);

  // Compute local Stiffness Matrices
  double *AK = new double[16 * nC];
  compute_transformation_matrix(AK, mesh);

  COO_array A_coo(nV, nV);
  assembleSparse(mesh, AK, isBoundary, &A_coo);

  CSR_array A(&A_coo);

  std::array<uint, 4> I = {0};
  // Diagonal d
  double *d = new double[nV]();

  for (size_t cn = 0; cn < nC; cn++)
  {
    // Global Index for each cell
    I[0] = mesh.Cells[cn].v0;
    I[1] = mesh.Cells[cn].v1;
    I[2] = mesh.Cells[cn].v2;
    I[3] = mesh.Cells[cn].v3;

    for (uint8_t i = 0; i < 4; i++)
    {
      if (isBoundary[I[i]] == true)
      {
        d[I[i]] = 1.0;
        memset(&AK[cn * 16 + i * 4], 0, 4 * sizeof(double));
      }
      else
      {
        d[I[i]] += AK[cn * 16 + i * 4 + i];
      }
    }
  }

  // Initial guess for solution 0 TBD
  double *u_new = new double[nV]();
  double *u_old = new double[nV]();
  std::memcpy(u_new, b, nV * sizeof(double));

  // Non-diagonal Elements sum
  double c;

  auto t1 = high_resolution_clock::now();
  for (size_t iterations = 0; iterations < max_iterations; iterations++)
  {

    std::memcpy(u_old, u_new, nV * sizeof(double));
    c = 0;

    for (size_t i = 0; i < nV; i++)
    {
      c = 0;
      for (size_t j = A.rowPtr[i]; j < A.rowPtr[i + 1]; j++)
      {
        if (i != A.colIdx[j])
        {
          c += A.data[j] * u_new[A.colIdx[j]];
        }
      }
      u_new[i] = (b[i] - c) / d[i];
    }
  }
  auto t2 = high_resolution_clock::now();
  duration<double, std::milli> ms_double = t2 - t1;
  std::cout << "Execution Time:" << ms_double.count() << "ms " << std::endl;

  // std::memset(u_new,-10,nV * sizeof(double));

  double error = calcError(&A, u_new, b);
  std::cout << "\nMean Absolute Error of  GS Iterative Method: \n Err = "
            << error << std::endl;

  delete[] isBoundary, b, d, u_new, u_old, AK;
}

// Laplacian Smoother using Sparse format Stiffness Matrix
// for Gauss-Seidel
class SparseSmoother
{
public:
  static void SmoothMesh3D(Mesh3D &mesh3D,
                           const CityModel &cityModel,
                           const GridField2D &dem,
                           double topHeight,
                           bool fixBuildings)
  {
    info("LaplacianSmoother: Smoothing mesh (Laplacian smoothing NEW)...");
    Timer timer("SmoothMesh3DNew: Sparse");

    const size_t nV = mesh3D.Vertices.size();

    std::vector<double> b(nV, 0);

    BoundaryConditions bc(mesh3D, cityModel, dem, topHeight, fixBuildings);
    bc.apply(b);

    stiffnessMatrix AK(mesh3D);

    COO_array A_coo(nV, nV);
    assemble(AK, bc.vMarkers, A_coo);

    CSR_array A(&A_coo);

    std::vector<double> u = b;

    GaussSeidel(mesh3D, A, b, u);

    // Update mesh coordinates
    for (std::size_t i = 0; i < mesh3D.Vertices.size(); i++)
      mesh3D.Vertices[i].z += u[i];
  }

  static void assemble(const stiffnessMatrix &A,
                       const std::vector<int> &VerticeMarkers,
                       COO_array &assembled_A)
  {
    info("Assembling Stiffness Matrix ");

    size_t nC = A._mesh.Cells.size();
    size_t nV = A._mesh.Vertices.size();

    std::array<uint, 4> I;

    for (size_t i = 0; i < nV; i++)
    {
      if (VerticeMarkers[i] > -4)
      {
        assembled_A.add(i, i, 1);
      }
    }

    std::cout << "Inserterd Boundary Vertices" << std::endl;

    for (size_t cn = 0; cn < nC; cn++)
    {
      I[0] = A._mesh.Cells[cn].v0;
      I[1] = A._mesh.Cells[cn].v1;
      I[2] = A._mesh.Cells[cn].v2;
      I[3] = A._mesh.Cells[cn].v3;

      for (size_t i = 0; i < 4; i++)
      {
        if (VerticeMarkers[I[i]] == -4)
        {
          assembled_A.add(I[i], I[0], A(cn, i, 0));
          assembled_A.add(I[i], I[1], A(cn, i, 1));
          assembled_A.add(I[i], I[2], A(cn, i, 2));
          assembled_A.add(I[i], I[3], A(cn, i, 3));
        }
      }
    }
  }

  static void GaussSeidel(const Mesh3D &mesh3D,
                          CSR_array &A,
                          std::vector<double> &b,
                          std::vector<double> &u,
                          const size_t max_iterations = 100,
                          const double tolerance = 1e-9)
  {
    info("Sparse Gauss Seidel Solver");
    Timer timer("Sparse GS");

    const size_t nV = mesh3D.Vertices.size();
    const size_t nC = mesh3D.Cells.size();

    size_t iterations;
    double error;
    for (iterations = 0; iterations < max_iterations; iterations++)
    {

      for (size_t i = 0; i < nV; i++)
      {
        double d = 1;
        double c = 0;
        for (size_t j = A.rowPtr[i]; j < A.rowPtr[i + 1]; j++)
        {
          if (i != A.colIdx[j])
          {
            c += A.data[j] * u[A.colIdx[j]];
          }
          else
          {
            d = A.data[j];
          }
        }
        double res = u[i];
        u[i] = (b[i] - c) / d;
        res = std::abs(res - u[i]);
        error = std::max(error, res);
      }
      // Check Convergance
      if (error < tolerance)
        break;
    }

    timer.Stop();
    timer.Print();

    std::cout << "Jacobi finished after " << iterations << " / "
              << max_iterations << " iterations" << std::endl;
    std::cout << "With error: " << error << std::endl;
  }
};

#endif

void assemble(const stiffnessMatrix &A,
              const std::vector<int> &VerticeMarkers,
              COO_array &assembled_A)
{
  info("Assembling Stiffness Matrix (COO Sparse) ");

  size_t nC = A._mesh.Cells.size();
  size_t nV = A._mesh.Vertices.size();

  std::array<uint, 4> I;

  for (size_t i = 0; i < nV; i++)
  {
    if (VerticeMarkers[i] > -4)
    {
      assembled_A.add(i, i, 1);
    }
  }

  std::cout << "Inserterd Boundary Vertices" << std::endl;

  for (size_t cn = 0; cn < nC; cn++)
  {
    I[0] = A._mesh.Cells[cn].v0;
    I[1] = A._mesh.Cells[cn].v1;
    I[2] = A._mesh.Cells[cn].v2;
    I[3] = A._mesh.Cells[cn].v3;

    for (size_t i = 0; i < 4; i++)
    {
      if (VerticeMarkers[I[i]] == -4)
      {
        assembled_A.add(I[i], I[0], A(cn, i, 0));
        assembled_A.add(I[i], I[1], A(cn, i, 1));
        assembled_A.add(I[i], I[2], A(cn, i, 2));
        assembled_A.add(I[i], I[3], A(cn, i, 3));
      }
    }
  }
}