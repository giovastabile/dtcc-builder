// Copyright (C) 2020 Anders Logg
// Licensed under the MIT License

#ifndef DTCC_GRID_FIELD_H
#define DTCC_GRID_FIELD_H

#include <cassert>

#include "Geometry.h"
#include "Grid.h"
#include "Logging.h"

namespace DTCC_BUILDER
{

/// GridField represents a scalar field on a uniform grid.
/// The field can be efficiently evaluated at arbitrary points inside
/// the grid domain. The value is computed by bilinear interpolation.
class GridField : public Printable
{
public:
  /// The grid
  Grid grid{};

  /// Array of values (vertex values)
  std::vector<double> Values{};

  /// Create empty field
  GridField() = default;

  /// Create zero field on given grid
  ///
  /// @param grid The grid
  explicit GridField(const Grid &grid) : grid(grid)
  {
    // Initialize values to zero
    Values.resize(grid.NumVertices());
    std::fill(Values.begin(), Values.end(), 0.0);
  }

  /// Evaluate field at given point
  ///
  /// @param p The point
  /// @return Value at point
  double operator()(const Point2D &p) const
  {
    // Map point to cell
    size_t i{};
    double x{}, y{};
    grid.Point2Cell(p, i, x, y);

    // Extract grid data
    const double v00 = Values[i];
    const double v10 = Values[i + 1];
    const double v01 = Values[i + grid.XSize];
    const double v11 = Values[i + grid.XSize + 1];

    // Compute value by bilinear interpolation
    return DTCC_BUILDER::Grid::Interpolate(x, y, v00, v10, v01, v11);
  }

  /// Evaluate field at given 3D point (using only x and y)
  double operator()(const Point3D &p) const
  {
    Point2D _p(p.x, p.y);
    return (*this)(_p);
  }

  double Nearest(const Point2D &p) const
  {
    size_t i{};
    double x{}, y{};
    grid.Point2Cell(p, i, x, y);
    return Values[i];
  }

  /// Interpolate given field at vertices.
  ///
  /// @param field The field to be interpolated
  void Interpolate(const GridField &field)
  {
    // Iterate over vertices and evaluate field
    for (size_t i = 0; i < grid.NumVertices(); i++)
      Values[i] = field(grid.Index2Point(i));
  }

  /// Compute minimal vertex value.
  ///
  /// @return Minimal vertex value
  double Min() const { return *std::min_element(Values.begin(), Values.end()); }

  /// Compute maximal of vertex value.
  ///
  /// @return Maximal vertex value
  double Max() const { return *std::max_element(Values.begin(), Values.end()); }

  /// Compute mean vertex value
  ///
  /// @return Mean vertex value
  double Mean() const
  {
    double mean = 0.0;
    for (const auto &value : Values)
      mean += value;
    return mean / static_cast<double>(Values.size());
  }

  /// Pretty-print
  std::string __str__() const { return "2D field on " + str(grid); }
};

} // namespace DTCC_BUILDER

#endif