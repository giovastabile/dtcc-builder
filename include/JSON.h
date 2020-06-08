// Copyright (C) 2020 Anders Logg
// Licensed under the MIT License

#ifndef DTCC_JSON_H
#define DTCC_JSON_H

#include <fstream>
#include <iostream>
#include <json.hpp>

#include "Parameters.h"
#include "BoundingBox.h"
#include "Grid.h"
#include "Mesh.h"
#include "Surface.h"
#include "GridField.h"
#include "CityModel.h"

namespace DTCC
{

  class JSON
  {
  public:

    // Utility functions for JSON input/output
    #include "JSONUtils.h"

    /// Deserialize Parameters
    static void Deserialize(Parameters& parameters, const nlohmann::json& json)
    {
      CheckType("Parameters", json);
      parameters.DataDirectory = ToString("DataDirectory", json);
      parameters.X0 = ToDouble("X0", json);
      parameters.Y0 = ToDouble("Y0", json);
      parameters.XMin = ToDouble("XMin", json);
      parameters.YMin = ToDouble("YMin", json);
      parameters.XMax = ToDouble("XMax", json);
      parameters.YMax = ToDouble("YMax", json);
      parameters.AutoDomain =ToBool("AutoDomain", json);
      parameters.DomainHeight = ToDouble("DomainHeight", json);
      parameters.HeightMapResolution = ToDouble("HeightMapResolution", json);
      parameters.MeshResolution = ToDouble("MeshResolution", json);
      parameters.MinimalBuildingDistance = ToDouble("MinimalBuildingDistance", json);
      parameters.FlatGround = ToBool("FlatGround", json);
      parameters.GroundSmoothing = ToInt("GroundSmoothing", json);
    };

    /// Serialize Parameters
    static void Serialize(const Parameters& parameters, nlohmann::json& json)
    {
      json["Type"] = "Parameters";
      json["DataDirectory"] = parameters.DataDirectory;
      json["X0"] = parameters.X0;
      json["Y0"] = parameters.Y0;
      json["YMin"] = parameters.YMin;
      json["XMin"] = parameters.XMin;
      json["YMin"] = parameters.YMin;
      json["XMax"] = parameters.XMax;
      json["YMax"] = parameters.YMax;
      json["AutoDomain"] = parameters.AutoDomain;
      json["DomainHeight"] = parameters.DomainHeight;
      json["HeightMapResolution"] = parameters.HeightMapResolution;
      json["MeshResolution"] = parameters.MeshResolution;
      json["MinimalBuildingDistance"] = parameters.MinimalBuildingDistance;
      json["FlatGround"] = parameters.FlatGround;
      json["GroundSmoothing"] = parameters.GroundSmoothing;
    }

    /// Deserialize BoundingBox2D
    static void Deserialize(BoundingBox2D& boundingBox, const nlohmann::json& json)
    {
      CheckType("BoundingBox2D", json);
      boundingBox.P.x = ToDouble("x", json["P"]);
      boundingBox.P.y = ToDouble("y", json["P"]);
      boundingBox.Q.x = ToDouble("x", json["Q"]);
      boundingBox.Q.y = ToDouble("y", json["Q"]);
    }

    /// Serialize BoundingBox2D
    static void Serialize(const BoundingBox2D& boundingBox, nlohmann::json& json)
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
    static void Deserialize(BoundingBox3D& boundingBox, const nlohmann::json& json)
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
    static void Serialize(const BoundingBox3D& boundingBox, nlohmann::json& json)
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
    static void Deserialize(Grid2D& grid, const nlohmann::json& json)
    {
      CheckType("Grid2D", json);
      Deserialize(grid.BoundingBox, json["BoundingBox"]);
      grid.XSize = ToUnsignedInt("XSize", json);
      grid.YSize = ToUnsignedInt("YSize", json);
      grid.XStep = ToDouble("XStep", json);
      grid.YStep = ToDouble("YStep", json);
    }

    /// Serialize Grid2D
    static void Serialize(const Grid2D& grid, nlohmann::json& json)
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
    static void Deserialize(Grid3D& grid, const nlohmann::json& json)
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
    static void Serialize(const Grid3D& grid, nlohmann::json& json)
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

    /// Deserialize Mesh2D
    static void Deserialize(Mesh2D& mesh, const nlohmann::json& json)
    {
      CheckType("Mesh3D", json);
      const auto jsonVertices = json["Vertices"];
      mesh.Vertices.resize(jsonVertices.size() / 2);
      for (size_t i = 0; i < mesh.Vertices.size(); i++)
      {
        mesh.Vertices[i].x = jsonVertices[2*i];
        mesh.Vertices[i].y = jsonVertices[2*i + 1];
      }
      const auto jsonCells = json["Cells"];
      mesh.Cells.resize(jsonCells.size() / 3);
      for (size_t i = 0; i < mesh.Cells.size(); i++)
      {
        mesh.Cells[i].v0 = jsonCells[3*i];
        mesh.Cells[i].v1 = jsonCells[3*i + 1];
        mesh.Cells[i].v2 = jsonCells[3*i + 2];
      }
    }

    /// Serialize Mesh2D
    static void Serialize(const Mesh2D& mesh, nlohmann::json& json)
    {
      auto jsonVertices = nlohmann::json::array();
      for (const auto p: mesh.Vertices)
      {
        jsonVertices.push_back(p.x);
        jsonVertices.push_back(p.y);
      }
      auto jsonCells = nlohmann::json::array();
      for (const auto T: mesh.Cells)
      {
        jsonCells.push_back(T.v0);
        jsonCells.push_back(T.v1);
        jsonCells.push_back(T.v2);
      }
      json["Type"] = "Mesh2D";
      json["Vertices"] = jsonVertices;
      json["Cells"] = jsonCells;
    }

    /// Deserialize Mesh3D
    static void Deserialize(Mesh3D& mesh, const nlohmann::json& json)
    {
      CheckType("Mesh3D", json);
      const auto jsonVertices = json["Vertices"];
      mesh.Vertices.resize(jsonVertices.size() / 3);
      for (size_t i = 0; i < mesh.Vertices.size(); i++)
      {
        mesh.Vertices[i].x = jsonVertices[3*i];
        mesh.Vertices[i].y = jsonVertices[3*i + 1];
        mesh.Vertices[i].z = jsonVertices[3*i + 2];
      }
      const auto jsonCells = json["Cells"];
      mesh.Cells.resize(jsonCells.size() / 4);
      for (size_t i = 0; i < mesh.Cells.size(); i++)
      {
        mesh.Cells[i].v0 = jsonCells[4*i];
        mesh.Cells[i].v1 = jsonCells[4*i + 1];
        mesh.Cells[i].v2 = jsonCells[4*i + 2];
        mesh.Cells[i].v3 = jsonCells[4*i + 3];
      }
    }

    /// Serialize Mesh3D
    static void Serialize(const Mesh3D& mesh, nlohmann::json& json)
    {
      auto jsonVertices = nlohmann::json::array();
      for (const auto p: mesh.Vertices)
      {
        jsonVertices.push_back(p.x);
        jsonVertices.push_back(p.y);
        jsonVertices.push_back(p.z);
      }
      auto jsonCells = nlohmann::json::array();
      for (const auto T: mesh.Cells)
      {
        jsonCells.push_back(T.v0);
        jsonCells.push_back(T.v1);
        jsonCells.push_back(T.v2);
        jsonCells.push_back(T.v3);
      }
      json["Type"] = "Mesh3D";
      json["Vertices"] = jsonVertices;
      json["Cells"] = jsonCells;
    }

    /// Deserialize Surface3D
    static void Deserialize(Surface2D &surface, const nlohmann::json& json)
    {
      CheckType("Surface3D", json);
      const auto jsonVertices = json["Vertices"];
      surface.Vertices.resize(jsonVertices.size() / 2);
      for (size_t i = 0; i < surface.Vertices.size(); i++)
      {
        surface.Vertices[i].x = jsonVertices[2*i];
        surface.Vertices[i].y = jsonVertices[2*i + 1];
      }
      const auto jsonCells = json["Cells"];
      surface.Cells.resize(jsonCells.size() / 2);
      for (size_t i = 0; i < surface.Cells.size(); i++)
      {
        surface.Cells[i].v0 = jsonCells[2*i];
        surface.Cells[i].v1 = jsonCells[2*i + 1];
      }
    }

    /// Serialize Surface3D
    static void Serialize(const Surface2D& surface, nlohmann::json& json)
    {
      auto jsonVertices = nlohmann::json::array();
      for (const auto p: surface.Vertices)
      {
        jsonVertices.push_back(p.x);
        jsonVertices.push_back(p.y);
      }
      auto jsonCells = nlohmann::json::array();
      for (const auto T: surface.Cells)
      {
        jsonCells.push_back(T.v0);
        jsonCells.push_back(T.v1);
      }
      json["Type"] = "Surface2D";
      json["Vertices"] = jsonVertices;
      json["Cells"] = jsonCells;
    }

    /// Deserialize Surface3D
    static void Deserialize(Surface3D &surface, const nlohmann::json& json)
    {
      CheckType("Surface3D", json);
      const auto jsonVertices = json["Vertices"];
      surface.Vertices.resize(jsonVertices.size() / 3);
      for (size_t i = 0; i < surface.Vertices.size(); i++)
      {
        surface.Vertices[i].x = jsonVertices[3*i];
        surface.Vertices[i].y = jsonVertices[3*i + 1];
        surface.Vertices[i].z = jsonVertices[3*i + 2];
      }
      const auto jsonCells = json["Cells"];
      surface.Cells.resize(jsonCells.size() / 3);
      for (size_t i = 0; i < surface.Cells.size(); i++)
      {
        surface.Cells[i].v0 = jsonCells[3*i];
        surface.Cells[i].v1 = jsonCells[3*i + 1];
        surface.Cells[i].v2 = jsonCells[3*i + 2];
      }
    }

    /// Serialize Surface3D
    static void Serialize(const Surface3D& surface, nlohmann::json& json)
    {
      auto jsonVertices = nlohmann::json::array();
      for (const auto p: surface.Vertices)
      {
        jsonVertices.push_back(p.x);
        jsonVertices.push_back(p.y);
        jsonVertices.push_back(p.z);
      }
      auto jsonCells = nlohmann::json::array();
      for (const auto T: surface.Cells)
      {
        jsonCells.push_back(T.v0);
        jsonCells.push_back(T.v1);
        jsonCells.push_back(T.v2);
      }
      json["Type"] = "Surface3D";
      json["Vertices"] = jsonVertices;
      json["Cells"] = jsonCells;
    }

    /// Deserialize GridField2D
    static void Deserialize(GridField2D& field, const nlohmann::json& json)
    {
      CheckType("GridField2D", json);
      Deserialize(field.Grid, json["Grid"]);
      const auto jsonValues = json["Values"];
      field.Values.resize(jsonValues.size());
      for (size_t i = 0; i < field.Values.size(); i++)
        field.Values[i] = jsonValues[i];
    }

    /// Serialize GridField2D
    static void Serialize(const GridField2D& field, nlohmann::json& json)
    {
      auto jsonGrid = nlohmann::json::object();
      Serialize(field.Grid, jsonGrid);
      json["Type"] = "GridField2D";
      json["Grid"] = jsonGrid;
      json["Values"] = field.Values;
    }

    /// Deserialize GridField3D
    static void Deserialize(GridField3D& field, const nlohmann::json& json)
    {
      CheckType("GridField3D", json);
      Deserialize(field.Grid, json["Grid"]);
      const auto jsonValues = json["Values"];
      field.Values.resize(jsonValues.size());
      for (size_t i = 0; i < field.Values.size(); i++)
        field.Values[i] = jsonValues[i];
    }

    /// Serialize GridField3D
    static void Serialize(const GridField3D& field, nlohmann::json& json)
    {
      auto jsonGrid = nlohmann::json::object();
      Serialize(field.Grid, jsonGrid);
      json["Type"] = "GridField3D";
      json["Grid"] = jsonGrid;
      json["Values"] = field.Values;
    }

    /// Serialize CityModel
    static void Deserialize(CityModel &cityModel, const nlohmann::json& json)
    {
      CheckType("CityModel", json);
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
        cityModel.Buildings[i].Height = jsonBuilding["Height"];
      }
    }

    /// Deserialize CityModel
    static void Serialize(const CityModel &cityModel, nlohmann::json& json)
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
        jsonBuilding["Height"] = building.Height;
        jsonBuildings.push_back(jsonBuilding);
      }
      json["Type"] = "CityModel";
      json["Buildings"] = jsonBuildings;
    }

  };

} // namespace DTCC

#endif
