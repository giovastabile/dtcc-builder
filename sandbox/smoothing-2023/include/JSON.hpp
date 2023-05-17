// Copyright (C) 2020-2021 Anders Logg, Anton J Olsson
// Licensed under the MIT License

#ifndef DTCC_JSON_H
#define DTCC_JSON_H

#include "datamodel/CityModel.h"
// #include <datamodel/District.h>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <utility>

#include "BoundingBox.h"
#include "Color.h"
// #include "ColorMap.h"
#include "Grid.h"
#include "GridField.h"
#include "GridVectorField.h"
#include "Mesh.h"
// #include "Parameters.h"
#include "PointCloud.h"
#include "Surface.h"
// #include "cityjson/CityJSON.h"
#include "datamodel/RoadNetwork.h"
// #include "/home/george/Documents/HPC/dtcc-builder/src/cpp/include/Mesh.h"

namespace DTCC
{

class JSON
{
public:
// Utility functions for JSON input/output
#include "JSONUtils.h"

  /// Serialize timings (special-case, only output supported)
  static void
  Serialize(std::map<std::string, std::pair<double, size_t>> timings,
            nlohmann::json &json)
  {
    for (const auto &it : timings)
    {
      const std::string task = it.first;
      const double total = it.second.first;
      const size_t count = it.second.second;
      const double mean = total / static_cast<double>(count);
      auto jsonTiming = nlohmann::json::object();
      jsonTiming["Total"] = total;
      jsonTiming["Count"] = count;
      jsonTiming["Mean"] = mean;
      json[task] = jsonTiming;
    }
  }

  /// Deserialize Mesh2D
  static void Deserialize(Mesh2D &mesh, const nlohmann::json &json)
  {
    CheckType("Mesh2D", json);
    const auto jsonVertices = json["Vertices"];
    mesh.Vertices.resize(jsonVertices.size() / 2);
    for (size_t i = 0; i < mesh.Vertices.size(); i++)
    {
      mesh.Vertices[i].x = jsonVertices[2 * i];
      mesh.Vertices[i].y = jsonVertices[2 * i + 1];
    }
    const auto jsonCells = json["Cells"];
    mesh.Cells.resize(jsonCells.size() / 3);
    for (size_t i = 0; i < mesh.Cells.size(); i++)
    {
      mesh.Cells[i].v0 = jsonCells[3 * i];
      mesh.Cells[i].v1 = jsonCells[3 * i + 1];
      mesh.Cells[i].v2 = jsonCells[3 * i + 2];
    }
    const auto jsonMarkers = json["Markers"];
    mesh.Markers.resize(jsonMarkers.size());
    for (size_t i = 0; i < mesh.Markers.size(); i++)
      mesh.Markers[i] = jsonMarkers[i];
  }

  /// Serialize Mesh2D and its offset/origin
  static void
  Serialize(const Mesh2D &mesh, nlohmann::json &json, const Point2D &origin)
  {
    auto jsonVertices = nlohmann::json::array();
    for (const auto p : mesh.Vertices)
    {
      jsonVertices.push_back(p.x);
      jsonVertices.push_back(p.y);
    }
    auto jsonCells = nlohmann::json::array();
    for (const auto T : mesh.Cells)
    {
      jsonCells.push_back(T.v0);
      jsonCells.push_back(T.v1);
      jsonCells.push_back(T.v2);
    }
    json["Type"] = "Mesh2D";
    json["Vertices"] = jsonVertices;
    json["Cells"] = jsonCells;
    json["Markers"] = mesh.Markers;
    json["Origin"] = {{"x", origin.x}, {"y", origin.y}};
  }

  static void Deserialize(Mesh3D &mesh, const nlohmann::json &json)
  {
    CheckType("Mesh3D", json);
    const auto jsonVertices = json["Vertices"];
    mesh.Vertices.resize(jsonVertices.size() / 3);
    for (size_t i = 0; i < mesh.Vertices.size(); i++)
    {
      mesh.Vertices[i].x = jsonVertices[3 * i];
      mesh.Vertices[i].y = jsonVertices[3 * i + 1];
      mesh.Vertices[i].z = jsonVertices[3 * i + 2];
    }
    const auto jsonCells = json["Cells"];
    mesh.Cells.resize(jsonCells.size() / 4);
    for (size_t i = 0; i < mesh.Cells.size(); i++)
    {
      mesh.Cells[i].v0 = jsonCells[4 * i];
      mesh.Cells[i].v1 = jsonCells[4 * i + 1];
      mesh.Cells[i].v2 = jsonCells[4 * i + 2];
      mesh.Cells[i].v3 = jsonCells[4 * i + 3];
    }
    const auto jsonMarkers = json["Markers"];
    mesh.Markers.resize(jsonMarkers.size());
    for (size_t i = 0; i < mesh.Markers.size(); i++)
      mesh.Markers[i] = jsonMarkers[i];
  }

  /// Serialize Mesh3D and its offset/origin
  static void
  Serialize(const Mesh3D &mesh, nlohmann::json &json, const Point2D &origin)
  {
    auto jsonVertices = nlohmann::json::array();
    for (const auto p : mesh.Vertices)
    {
      jsonVertices.push_back(p.x);
      jsonVertices.push_back(p.y);
      jsonVertices.push_back(p.z);
    }
    auto jsonCells = nlohmann::json::array();
    for (const auto T : mesh.Cells)
    {
      jsonCells.push_back(T.v0);
      jsonCells.push_back(T.v1);
      jsonCells.push_back(T.v2);
      jsonCells.push_back(T.v3);
    }
    json["Type"] = "Mesh3D";
    json["Vertices"] = jsonVertices;
    json["Cells"] = jsonCells;
    json["Markers"] = mesh.Markers;
    json["Origin"] = {{"x", origin.x}, {"y", origin.y}};
  }

  /// Deserialize CityModel
  static void Deserialize(CityModel &cityModel, const nlohmann::json &json)
  {
    CheckType("CityModel", json);
    cityModel.Name = json["Name"];
    auto jsonBuildings = json["Buildings"];
    cityModel.Buildings.resize(jsonBuildings.size());
    for (size_t i = 0; i < jsonBuildings.size(); i++)
    {
      auto jsonBuilding = jsonBuildings[i];

      auto jsonFootprint = jsonBuilding["Footprint"];
      cityModel.Buildings[i].Footprint.Vertices.resize(jsonFootprint.size());
      for (size_t j = 0; j < jsonFootprint.size(); j++)
      {
        cityModel.Buildings[i].Footprint.Vertices[j].x = jsonFootprint[j]["x"];
        cityModel.Buildings[i].Footprint.Vertices[j].y = jsonFootprint[j]["y"];
      }
      auto jsonRoofPoints = jsonBuilding["RoofPoints"];
      cityModel.Buildings[i].RoofPoints.resize(jsonRoofPoints.size());
      for (size_t j = 0; j < jsonRoofPoints.size(); j++)
      {
        cityModel.Buildings[i].RoofPoints[j].x = jsonRoofPoints[j]["x"];
        cityModel.Buildings[i].RoofPoints[j].y = jsonRoofPoints[j]["y"];
        cityModel.Buildings[i].RoofPoints[j].z = jsonRoofPoints[j]["z"];
      }
      auto jsonGroundPoints = jsonBuilding["GroundPoints"];
      cityModel.Buildings[i].GroundPoints.resize(jsonGroundPoints.size());
      for (size_t j = 0; j < jsonGroundPoints.size(); j++)
      {
        cityModel.Buildings[i].GroundPoints[j].x = jsonGroundPoints[j]["x"];
        cityModel.Buildings[i].GroundPoints[j].y = jsonGroundPoints[j]["y"];
        cityModel.Buildings[i].GroundPoints[j].z = jsonGroundPoints[j]["z"];
      }
      cityModel.Buildings[i].Height = jsonBuilding["Height"];
      cityModel.Buildings[i].GroundHeight = jsonBuilding["GroundHeight"];
      cityModel.Buildings[i].UUID = jsonBuilding["UUID"];
      cityModel.Buildings[i].SHPFileID = jsonBuilding["SHPFileID"];
      cityModel.Buildings[i].error = jsonBuilding["Error"];
    }
  }

  /// Serialize CityModel and its offset/origin
  static void Serialize(const CityModel &cityModel,
                        nlohmann::json &json,
                        const Point2D &origin)
  {
    auto jsonBuildings = nlohmann::json::array();
    for (auto const &building : cityModel.Buildings)
    {
      auto jsonBuilding = nlohmann::json::object();
      jsonBuilding["Footprint"] = nlohmann::json::array();
      for (auto const &point : building.Footprint.Vertices)
      {
        auto jsonPoint = nlohmann::json::object();
        jsonPoint["x"] = point.x;
        jsonPoint["y"] = point.y;
        jsonBuilding["Footprint"].push_back(jsonPoint);
      }
      jsonBuilding["RoofPoints"] = nlohmann::json::array();
      for (auto const &point : building.RoofPoints)
      {
        auto jsonPoint = nlohmann::json::object();
        jsonPoint["x"] = point.x;
        jsonPoint["y"] = point.y;
        jsonPoint["z"] = point.z;
        jsonBuilding["RoofPoints"].push_back(jsonPoint);
      }
      jsonBuilding["GroundPoints"] = nlohmann::json::array();
      for (auto const &point : building.GroundPoints)
      {
        auto jsonPoint = nlohmann::json::object();
        jsonPoint["x"] = point.x;
        jsonPoint["y"] = point.y;
        jsonPoint["z"] = point.z;
        jsonBuilding["GroundPoints"].push_back(jsonPoint);
      }
      jsonBuilding["Height"] = building.Height;
      jsonBuilding["GroundHeight"] = building.GroundHeight;
      jsonBuilding["UUID"] = building.UUID;
      // Uncomment for debugging
      // jsonBuilding["debugID"] = building.debugID;
      jsonBuilding["SHPFileID"] = building.SHPFileID;
      jsonBuilding["Error"] = building.error;
      jsonBuildings.push_back(jsonBuilding);
    }
    json["Type"] = "CityModel";
    json["Name"] = cityModel.Name;
    json["Buildings"] = jsonBuildings;
    json["Origin"] = {{"x", origin.x}, {"y", origin.y}};
  }

  /// Deserialize BoundingBox2D
  static void Deserialize(BoundingBox2D &boundingBox,
                          const nlohmann::json &json)
  {
    CheckType("BoundingBox2D", json);
    boundingBox.P.x = ToDouble("x", json["P"]);
    boundingBox.P.y = ToDouble("y", json["P"]);
    boundingBox.Q.x = ToDouble("x", json["Q"]);
    boundingBox.Q.y = ToDouble("y", json["Q"]);
  }

  /// Serialize BoundingBox2D
  static void Serialize(const BoundingBox2D &boundingBox, nlohmann::json &json)
  {
    auto jsonP = nlohmann::json::object();
    jsonP["x"] = boundingBox.P.x;
    jsonP["y"] = boundingBox.P.y;
    auto jsonQ = nlohmann::json::object();
    jsonQ["x"] = boundingBox.Q.x;
    jsonQ["y"] = boundingBox.Q.y;
    json["Type"] = "BoundingBox2D";
    json["P"] = jsonP;
    json["Q"] = jsonQ;
  }

  /// Deserialize BoundingBox3D
  static void Deserialize(BoundingBox3D &boundingBox,
                          const nlohmann::json &json)
  {
    CheckType("BoundingBox3D", json);
    boundingBox.P.x = ToDouble("x", json["P"]);
    boundingBox.P.y = ToDouble("y", json["P"]);
    boundingBox.P.z = ToDouble("z", json["P"]);
    boundingBox.Q.x = ToDouble("x", json["Q"]);
    boundingBox.Q.y = ToDouble("y", json["Q"]);
    boundingBox.Q.z = ToDouble("z", json["Q"]);
  }

  /// Serialize BoundingBox3D
  static void Serialize(const BoundingBox3D &boundingBox, nlohmann::json &json)
  {
    auto jsonP = nlohmann::json::object();
    jsonP["x"] = boundingBox.P.x;
    jsonP["y"] = boundingBox.P.y;
    jsonP["z"] = boundingBox.P.z;
    auto jsonQ = nlohmann::json::object();
    jsonQ["x"] = boundingBox.Q.x;
    jsonQ["y"] = boundingBox.Q.y;
    jsonQ["z"] = boundingBox.Q.z;
    json["Type"] = "BoundingBox3D";
    json["P"] = jsonP;
    json["Q"] = jsonQ;
  }

  /// Deserialize Grid2D
  static void Deserialize(Grid2D &grid, const nlohmann::json &json)
  {
    CheckType("Grid2D", json);
    Deserialize(grid.BoundingBox, json["BoundingBox"]);
    grid.XSize = ToUnsignedInt("XSize", json);
    grid.YSize = ToUnsignedInt("YSize", json);
    grid.XStep = ToDouble("XStep", json);
    grid.YStep = ToDouble("YStep", json);
  }

  /// Serialize Grid2D
  static void Serialize(const Grid2D &grid, nlohmann::json &json)
  {
    auto jsonBoundingBox = nlohmann::json::object();
    Serialize(grid.BoundingBox, jsonBoundingBox);
    json["Type"] = "Grid2D";
    json["BoundingBox"] = jsonBoundingBox;
    json["XSize"] = grid.XSize;
    json["YSize"] = grid.YSize;
    json["XStep"] = grid.XStep;
    json["YStep"] = grid.YStep;
  }

  /// Deserialize Grid3D
  static void Deserialize(Grid3D &grid, const nlohmann::json &json)
  {
    CheckType("Grid3D", json);
    Deserialize(grid.BoundingBox, json["BoundingBox"]);
    grid.XSize = ToUnsignedInt("XSize", json);
    grid.YSize = ToUnsignedInt("YSize", json);
    grid.ZSize = ToUnsignedInt("ZSize", json);
    grid.XStep = ToDouble("XStep", json);
    grid.YStep = ToDouble("YStep", json);
    grid.ZStep = ToDouble("ZStep", json);
  }

  /// Serialize Grid3D
  static void Serialize(const Grid3D &grid, nlohmann::json &json)
  {
    auto jsonBoundingBox = nlohmann::json::object();
    Serialize(grid.BoundingBox, jsonBoundingBox);
    json["Type"] = "Grid3D";
    json["BoundingBox"] = jsonBoundingBox;
    json["XSize"] = grid.XSize;
    json["YSize"] = grid.YSize;
    json["ZSize"] = grid.ZSize;
    json["XStep"] = grid.XStep;
    json["YStep"] = grid.YStep;
    json["ZStep"] = grid.ZStep;
  }

  /// Deserialize GridField2D
  static void Deserialize(GridField2D &field, const nlohmann::json &json)
  {
    CheckType("GridField2D", json);
    Deserialize(field.Grid, json["Grid"]);
    const auto jsonValues = json["Values"];
    field.Values.resize(jsonValues.size());
    for (size_t i = 0; i < field.Values.size(); i++)
      field.Values[i] = jsonValues[i];
  }

  /// Serialize GridField2D and its offset/origin
  static void Serialize(const GridField2D &field,
                        nlohmann::json &json,
                        const Point2D &origin)
  {
    auto jsonGrid = nlohmann::json::object();
    Serialize(field.Grid, jsonGrid);
    json["Type"] = "GridField2D";
    json["Grid"] = jsonGrid;
    json["Values"] = field.Values;
    json["Origin"] = {{"x", origin.x}, {"y", origin.y}};
  }

  /// Deserialize GridField3D
  static void Deserialize(GridField3D &field, const nlohmann::json &json)
  {
    CheckType("GridField3D", json);
    Deserialize(field.Grid, json["Grid"]);
    const auto jsonValues = json["Values"];
    field.Values.resize(jsonValues.size());
    for (size_t i = 0; i < field.Values.size(); i++)
      field.Values[i] = jsonValues[i];
  }

  /// Serialize GridField3D and its offset/origin
  static void Serialize(const GridField3D &field,
                        nlohmann::json &json,
                        const Point2D &origin)
  {
    auto jsonGrid = nlohmann::json::object();
    Serialize(field.Grid, jsonGrid);
    json["Type"] = "GridField3D";
    json["Grid"] = jsonGrid;
    json["Values"] = field.Values;
    json["Origin"] = {{"x", origin.x}, {"y", origin.y}};
  }
};

} // namespace DTCC

#endif