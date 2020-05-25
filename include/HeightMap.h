// Copyright (C) 2020 Anders Logg
// Licensed under the MIT License

#ifndef DTCC_HEIGHT_MAP_H
#define DTCC_HEIGHT_MAP_H

#include <algorithm>
#include <assert.h>
#include <vector>

#include "Point.h"

namespace DTCC
{

class HeightMap
{
public:
  // Grid dimensions
  double XMin, YMin, XMax, YMax;

  // Number of grid points
  size_t XSize, YSize;

  // Grid resolution
  double XStep, YStep;

  // Grid data (flattened row-major starting at (XMin, YMin))
  std::vector<double> GridData;

  // Create empty height map
  HeightMap()
      : XMin(0), YMin(0), XMax(0), YMax(0), XSize(0), YSize(0), XStep(0),
        YStep(0)
  {
  }

  // Return height (z) at 2D point p
  double operator()(const Point2D &p) const { return (*this)(p.x, p.y); }

  // Return height (z) at 3D point p (z-coordinate ignored)
  double operator()(const Point3D &p) const { return (*this)(p.x, p.y); }

  // Return height (z) at 2D point p
  double operator()(double x, double y) const
  {
    // Check that point is inside domain
    if (x < XMin || x > XMax || y < YMin || y > YMax)
    {
      std::cout << "(x, y) = (" << x << ", " << y << ")" << std::endl;
      throw std::runtime_error("Point outside of height map domain.");
    }

    // Compute grid cell containing point (lower left corner)
    const double _x = x - XMin;
    const double _y = y - YMin;
    const size_t ix = std::min(static_cast<size_t>(std::floor(_x / XStep)), XSize - 2);
    const size_t iy = std::min(static_cast<size_t>(std::floor(_y / YStep)), YSize - 2);
    const size_t i = iy * XSize + ix;

    // Map coordinates to [0, 1] x [0, 1] within grid squares
    const double X = (_x - ix * XStep) / XStep;
    const double Y = (_y - iy * YStep) / YStep;
    assert(X >= 0.0);
    assert(Y >= 0.0);
    assert(X <= 1.0);
    assert(Y <= 1.0);

    // Extract grid data
    const double z00 = GridData[i];
    const double z10 = GridData[i + 1];
    const double z01 = GridData[i + XSize];
    const double z11 = GridData[i + XSize + 1];

    // Compute value by bilinear interpolation
    const double z = (1.0 - X) * (1.0 - Y) * z00 + (1.0 - X) * Y * z01 +
                     X * (1.0 - Y) * z10 + X * Y * z11;

    return z;
  }

  // Compute minimal height
  double Min() const
  {
    return *std::min_element(GridData.begin(), GridData.end());
  }

  // Compute maximal height
  double Max() const
  {
    return *std::max_element(GridData.begin(), GridData.end());
  }

  // Compute mean height
  double Mean() const
  {
    return 0.5*(Min() + Max());
  }

  // Map index to boundary at distance 1 in grid.
  // This is the in-place version. Reserve vector to size 4 for efficiency.
  void Index2Boundary(std::vector<size_t>& indices, size_t i) const
  {
    const size_t ix = i % XSize;
    const size_t iy = i / XSize;
    if (ix > 0)
      indices.push_back(i - 1);
    if (ix < XSize - 1)
      indices.push_back(i + 1);
    if (iy > 0)
      indices.push_back(i - XSize);
    if (iy < YSize - 1)
      indices.push_back(i + XSize);
  }

  // Map index to boundary at distance 1 in grid
  std::vector<size_t> Index2Boundary(size_t i) const
  {
    std::vector<size_t> indices;
    indices.reserve(4);
    Index2Boundary(indices, i);
    return indices;
  }

  // Map index to boundary at distance step in grid
  std::vector<size_t> Index2Boundary(size_t i, size_t step) const
  {
    // Compute center (assume it is inside domain)
    const long int _ix = i % XSize;
    const long int _iy = i / YSize;
    const long int d = step;

    // Needed for signed comparison below
    const long int xsize = XSize;
    const long int ysize = YSize;

    // Initialize empty list of indices
    std::vector<size_t> indices;

    // Iterate for x in (-step, step)
    for (long int dx = -d; dx <= d; dx++)
    {
      // Skip if outside grid
      const long int ix = _ix + dx;
      if (ix < 0 || ix >= xsize)
        continue;

      // Iterate for y in (-step, step)
      for (long int dy = -d; dy <= d; dy++)
      {
        // Skip if outside grid
        const long int iy = _iy + dy;
        if (iy < 0 || iy >= ysize)
          continue;

        // Skip if not on boundary
        const bool bx = dx == -d || dx == d;
        const bool by = dy == -d || dy == d;
        if (!bx && !by)
          continue;

        // Add point
        indices.push_back(iy * XSize + ix);
      }
    }

    return indices;
  }

  // Map index to coordinate
  Point2D Index2Coordinate(size_t i) const
  {
    const size_t ix = i % XSize;
    const size_t iy = i / XSize;
    return Point2D(XMin + ix * XStep, YMin + iy * YStep);
  }

  // Map coordinate to index (closest point)
  size_t Coordinate2Index(const Point2D &p) const
  {
    long int _ix = std::lround((p.x - XMin) / XStep);
    long int _iy = std::lround((p.y - YMin) / YStep);
    size_t ix = (_ix < 0 ? 0 : _ix);
    size_t iy = (_iy < 0 ? 0 : _iy);
    if (ix >= XSize)
      ix = XSize - 1;
    if (iy >= YSize)
      iy = YSize - 1;
    return iy * XSize + ix;
  }

};

std::ostream &operator<<(std::ostream &stream, const HeightMap &heightMap)
{
  stream << "Height map with grid size " << heightMap.XSize << " x "
         << heightMap.YSize << " on domain [" << heightMap.XMin << ", "
         << heightMap.XMax << "] x [" << heightMap.YMin << ", "
         << heightMap.YMax << "]";
  return stream;
}

} // namespace DTCC

#endif
