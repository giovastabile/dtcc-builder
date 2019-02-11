// Representation of 2D height maps.
// Copyright (C) 2019 Anders Logg.

#ifndef VC_HEIGHT_MAP_H
#define VC_HEIGHT_MAP_H

#include <vector>

#include "Point.h"
#include "GeoReference.h"

namespace VirtualCity
{

class HeightMap
{
public:

    // Grid width
    size_t Width;

    // Grid height
    size_t Height;

    // Grid data (flattened array of (x, y) coordinates)
    std::vector<double> GridData;

    // Grid map (transform of grid data)
    GeoReference GridMap;

    // Create empty height map
    HeightMap() : Width(0), Height(0) {}

    // Return height (z) at point p
    double operator() (const Point2D& p) const
    {
        return (*this)(p.x, p.y);
    }

    // Return height (z) at point (x, y)
    double operator() (double x, double y) const
    {
        return 0.0;
    }

    // Apply geo reference to height map
    double Apply(const GeoReference& geoReference)
    {
        GridMap = geoReference;
    }

};

}

#endif
