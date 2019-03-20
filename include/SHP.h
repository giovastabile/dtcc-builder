// SHP I/O
// Anders Logg 2019

#ifndef VC_SHP_H
#define VC_SHP_H

#include <iostream>
#include <vector>
#include <shapefil.h>

#include "Polygon.h"

namespace VirtualCity
{

class SHP
{
public:

    // Read polygons from SHP file. Note that the corresponding
    // .shx and .dbf files must also be present in the same directory.
    static void Read(std::vector<Polygon>& polygons, std::string fileName)
    {
        std::cout << "SHP: " << "Reading polygons from file "
                  << fileName << std::endl;

        // Open file(s)
        SHPHandle handle = SHPOpen(fileName.c_str(), "r");

        // Get info
        int numEntities, shapeType;
        SHPGetInfo(handle, &numEntities, &shapeType, NULL, NULL);
        std::cout << "SHP: " << numEntities << " entities" << std::endl;
        switch (shapeType)
        {
        case SHPT_POINT:
            std::cout << "SHP: point type" << std::endl;
            break;
        case SHPT_ARC:
            std::cout << "SHP: arc type" << std::endl;
            break;
        case SHPT_POLYGON:
            std::cout << "SHP: polygon type" << std::endl;
            break;
        case SHPT_MULTIPOINT:
            std::cout << "SHP: multipoint type" << std::endl;
            break;
        default:
            std::cout << "SHP: unkown type" << std::endl;
        }

        // Check that we have polygon type
        if (shapeType != SHPT_POLYGON)
            throw std::runtime_error("Shapefile not of polygon type.");

        // Read footprints
        for (int i = 0; i < numEntities; i++)
        {
            // Create empty polygon
            Polygon polygon;

            // Get object
            SHPObject* object = SHPReadObject(handle, i);

            // Get vertices
            for (int j = 0; j < object->nVertices; j++)
            {
                const double x = object->padfX[j];
                const double y = object->padfY[j];
                Point2D p(x, y);
                polygon.Points.push_back(p);
            }

            // Add polygon
            polygons.push_back(polygon);
        }
    }

};

}

#endif
