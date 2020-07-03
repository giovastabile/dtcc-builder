// Copyright (C) 2020 Anders Logg
// Licensed under the MIT License

#ifndef DTCC_HEIGHT_MAP_GENERATOR_H
#define DTCC_HEIGHT_MAP_GENERATOR_H

#include <iomanip>
#include <iostream>
#include <vector>
#include <stack>

#include "Vector.h"
#include "PointCloud.h"
#include "GridField.h"
#include "Geometry.h"
#include "Timer.h"
#include "Logging.h"
#include "Parameters.h"

namespace DTCC
{

class HeightMapGenerator
{
public:
  // Generate height map from point cloud
  static void GenerateHeightMap(GridField2D &heightMap,
                                const PointCloud &pointCloud,
                                double x0,
                                double y0,
                                double xMin,
                                double yMin,
                                double xMax,
                                double yMax,
                                double heightMapResolution)
  {
    Info("HeightMapGenerator: Generating heightmap from point cloud...");
    Timer("GenerateHeightMap");

    // Check for empty data
    if (pointCloud.Points.size() == 0)
      Error("HeightMapGenerator: Empty point cloud");

    // Shortcut
    GridField2D& hm = heightMap;

    // Initialize grid dimensions
    hm.Grid.BoundingBox.P.x = xMin;
    hm.Grid.BoundingBox.P.y = yMin;
    hm.Grid.BoundingBox.Q.x = xMax;
    hm.Grid.BoundingBox.Q.y = yMax;

    // Initialize grid data
    hm.Grid.XSize = (hm.Grid.BoundingBox.Q.x - hm.Grid.BoundingBox.P.x) / heightMapResolution + 1;
    hm.Grid.YSize = (hm.Grid.BoundingBox.Q.y - hm.Grid.BoundingBox.P.y) / heightMapResolution + 1;
    hm.Values.resize(hm.Grid.XSize * hm.Grid.YSize);
    std::fill(hm.Values.begin(), hm.Values.end(), 0.0);
    hm.Grid.XStep = (hm.Grid.BoundingBox.Q.x - hm.Grid.BoundingBox.P.x) / (hm.Grid.XSize - 1);
    hm.Grid.YStep = (hm.Grid.BoundingBox.Q.y - hm.Grid.BoundingBox.P.y) / (hm.Grid.YSize - 1);

    std::cout << "HeightMapGenerator: Computing mean elevation" << std::endl;

    // Compute mean raw elevation (used for skipping outliers)
    double meanElevationRaw = 0.0;
    for (auto const &q3D : pointCloud.Points)
      meanElevationRaw += q3D.z;
    meanElevationRaw /= pointCloud.Points.size();

    // Initialize counters for number of points for local mean
    size_t numGridPoints = hm.Values.size();
    std::vector<size_t> numLocalPoints(numGridPoints);
    std::fill(numLocalPoints.begin(), numLocalPoints.end(), 0);

    std::cout << "HeightMapGenerator: Extracting point cloud data" << std::endl;

    // Iterate over point cloud and sum up heights
    size_t numOutliers = 0;
    double meanElevation = 0.0;
    std::vector<size_t> neighborIndices;
    neighborIndices.reserve(5);
    for (auto const &q3D : pointCloud.Points)
    {
      // Ignore outliers
      if (q3D.z - meanElevationRaw > Parameters::PointCloudOutlierThreshold)
      {
        numOutliers += 1;
        continue;
      }

      // Get 2D point and subtract origin
      const Vector2D q2D(q3D.x - x0, q3D.y - y0);

      // Recompute mean elevation (excluding outliers)
      meanElevation += q3D.z;

      // Iterate over closest stencil (including center of stencil)
      neighborIndices.clear();
      const size_t i = hm.Grid.Point2Index(q2D);
      neighborIndices.push_back(i);
      hm.Grid.Index2Boundary(i, neighborIndices);
      for (size_t j : neighborIndices)
      {
        hm.Values[j] += q3D.z;
        numLocalPoints[j] += 1;
      }
    }

    // Compute mean elevation
    meanElevation /= pointCloud.Points.size() - numOutliers;

    std::cout << "HeightMapGenerator: Computing local mean elevation"
              << std::endl;

    // Compute mean of elevations for each grid point
    std::vector<size_t> missingIndices;
    for (size_t i = 0; i < numGridPoints; i++)
    {
      if (numLocalPoints[i] > 0)
        hm.Values[i] /= numLocalPoints[i];
      else
        missingIndices.push_back(i);
    }

    // Check that we have at least one point (very loose check)
    const size_t numMissing = missingIndices.size();
    if (numMissing == numGridPoints)
      throw std::runtime_error("No points inside height map domain.");

    // Note: We fill in missing point by searching for the
    // closest existing value around each missing grid point.
    // It might be more efficient to do a flood fill.

    std::cout << "HeightMapGenerator: Filling in missing grid points ("
              << numMissing << "/" << numGridPoints << ")" << std::endl;

    // Reuse vector numLocalPoints to indicate which points have been
    // visited: 0 = empty, 1 = boundary, 2 = filled
    for (size_t i = 0; i < numGridPoints; i++)
      numLocalPoints[i] = (numLocalPoints[i] == 0 ? 0 : 2);

    // Create stack of boundary points neighboring unfilled regions by
    // examining the neighbors of all missing points. Note that we use
    // numLocalPoints to keep track of which boundary that have already
    // been added to the stack; only add neighbors that already contain
    // a value and only add neighbors that have not been added before.
    std::stack<size_t> boundaryIndices;
    for (size_t i : missingIndices)
    {
      neighborIndices.clear();
      hm.Grid.Index2Boundary(i, neighborIndices);
      for (size_t j : neighborIndices)
      {
        if (numLocalPoints[j] == 2)
        {
          boundaryIndices.push(j);
          numLocalPoints[j] = 1;
        }
      }
    }

    // Flood fill values until stack is empty
    size_t numFound = 0;
    while (!boundaryIndices.empty())
    {
      // Get boundary index from top of stack
      const size_t i = boundaryIndices.top();
      boundaryIndices.pop();

      // Propagate values to neighbors and add neighbor to stack
      neighborIndices.clear();
      hm.Grid.Index2Boundary(i, neighborIndices);
      for (size_t j : neighborIndices)
      {
        if (numLocalPoints[j] == 0)
        {
          hm.Values[j] = hm.Values[i];
          boundaryIndices.push(j);
          numLocalPoints[j] = 1;
          numFound++;
        }
      }
    }

    // Check that we found data for all grid points
    if (numFound != numMissing)
      throw std::runtime_error("Unable to find data for all grid points.");

    // Print some stats
    const double percentMissing =
        100.0 * static_cast<double>(numMissing) / numGridPoints;
    std::cout << "HeightMapGenerator: " << numOutliers << " outliers ignored"
              << std::endl;
    std::cout << "HeightMapGenerator: Mean elevation is "
              << std::setprecision(4) << meanElevation << "m" << std::endl;
    std::cout << "HeightMapGenerator: " << numGridPoints << " grid points"
              << std::endl;
    std::cout << "HeightMapGenerator: " << numMissing
              << " missing grid points (" << std::setprecision(3)
              << percentMissing << "%)" << std::endl;
    //    std::cout << "HeightMapGenerator: "
    //        << "Maximum search distance is " << maxStep << std::endl;

    // Test data for verifying orientation, bump in lower left corner
    // for (size_t i = 0; i < heightMap.Values.size(); i++)
    // {
    //     Vector2D p = heightMap.Index2Coordinate(i);
    //     const double dx = heightMap.XMax - heightMap.XMin;
    //     const double dy = heightMap.YMax - heightMap.YMin;
    //     const double x = (p.x - heightMap.XMin) / dx;
    //     const double y = (p.y - heightMap.YMin) / dy;
    //     heightMap.Values[i] = x * (1 - x) * (1 - x) * y * (1 - y) * (1 -
    //     y);
    // }
  }
};

} // namespace DTCC

#endif
