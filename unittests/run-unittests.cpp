// Copyright (C) 2020 Anders Logg, Anton J Olsson
// Licensed under the MIT License

#define CATCH_CONFIG_MAIN

#include "Color.h"
#include "ColorMap.h"
#include "ColorMapIO.h"
#include "Grid.h"
#include "GridField.h"
#include "GridVectorField.h"
#include "Hashing.h"
#include "JSON.h"
#include "LAS.h"
#include "Mesh.h"
#include "MeshField.h"
#include "MeshVectorField.h"
#include "PointCloud.h"
#include "PointCloudProcessor.h"
#include "SHP.h"
#include "catch.hpp"
#include <XMLParser.h>
#include <deque>
#include <nlohmann/json.hpp>

using namespace DTCC;

TEST_CASE("Grid2D")
{
  Point2D p(0, 0);
  Point2D q(1, 1);
  BoundingBox2D bbox(p, q);
  Grid2D grid(bbox, 4, 5);

  SECTION("StepSize")
  {
    REQUIRE(grid.XStep == Approx(1.0/3.0));
    REQUIRE(grid.YStep == Approx(1.0/4.0));
  }

  SECTION("NumVertices")
  {
    REQUIRE(grid.NumVertices() == 20);
  }

  SECTION("NumCells")
  {
    REQUIRE(grid.NumCells() == 12);
  }

  SECTION("Index2Point2Index")
  {
    size_t index = grid.NumVertices() / 3;
    REQUIRE(grid.Point2Index(grid.Index2Point(index)) == index);
  }
}

TEST_CASE("Grid3D")
{
  Point3D p(0, 0, 0);
  Point3D q(1, 1, 1);
  BoundingBox3D bbox(p, q);
  Grid3D grid(bbox, 4, 5, 6);

  SECTION("StepSize")
  {
    REQUIRE(grid.XStep == Approx(1.0/3.0));
    REQUIRE(grid.YStep == Approx(1.0/4.0));
    REQUIRE(grid.ZStep == Approx(1.0/5.0));
  }

  SECTION("NumVertices")
  {
    REQUIRE(grid.NumVertices() == 120);
  }

  SECTION("NumCells")
  {
    REQUIRE(grid.NumCells() == 60);
  }

  SECTION("Index2Point2Index")
  {
    size_t index = grid.NumVertices() / 3;
    REQUIRE(grid.Point2Index(grid.Index2Point(index)) == index);
  }
}

TEST_CASE("GridField2D")
{
  Point2D p(0, 0);
  Point2D q(1, 1);
  Point2D r(0.234, 0.485);
  BoundingBox2D bbox(p, q);
  Grid2D grid(bbox, 11, 17);
  GridField2D u(grid);

  class MyField : public Field2D
  {
  public:
    double operator()(const Point2D& p) const
    {
      return sin(p.x)*cos(p.y);
    }
  };

  SECTION("Evaluate")
  {
    REQUIRE(u(p) == Approx(0.0));
    REQUIRE(u(q) == Approx(0.0));
  }

  SECTION("Interpolate")
  {
    MyField f;
    u.Interpolate(f);
    REQUIRE(u(r) == Approx(f(r)).margin(0.01));
  }
}

TEST_CASE("GridField3D")
{
  Point3D p(0, 0, 0);
  Point3D q(1, 1, 1);
  Point3D r(0.234, 0.485, 0.763);
  BoundingBox3D bbox(p, q);
  Grid3D grid(bbox, 11, 17, 23);
  GridField3D u(grid);

  class MyField : public Field3D
  {
  public:
    double operator()(const Point3D& p) const
    {
      return sin(p.x)*cos(p.y)*exp(p.z);
    }
  };

  SECTION("Evaluate")
  {
    REQUIRE(u(p) == Approx(0.0));
    REQUIRE(u(q) == Approx(0.0));
  }

  SECTION("Interpolate")
  {
    MyField f;
    u.Interpolate(f);
    REQUIRE(u(r) == Approx(f(r)).margin(0.01));
  }
}

TEST_CASE("GridVectorField2D")
{
  Point2D p(0, 0);
  Point2D q(1, 1);
  Point2D r(0.234, 0.485);
  BoundingBox2D bbox(p, q);
  Grid2D grid(bbox, 11, 17);
  GridVectorField2D u(grid);

  class MyField : public VectorField2D
  {
  public:
    Vector2D operator()(const Point2D& p) const
    {
      return Vector2D(sin(p.x)*cos(p.y), cos(p.x)*sin(p.y));
    }
  };

  SECTION("Evaluate")
  {
    REQUIRE(u(p).x == Approx(0.0));
    REQUIRE(u(p).y == Approx(0.0));
    REQUIRE(u(q).x == Approx(0.0));
    REQUIRE(u(q).y == Approx(0.0));
  }

  SECTION("Interpolate")
  {
    MyField f;
    u.Interpolate(f);
    REQUIRE(u(r).x == Approx(f(r).x).margin(0.01));
    REQUIRE(u(r).y == Approx(f(r).y).margin(0.01));
  }
}

TEST_CASE("GridVectorField3D")
{
  Point3D p(0, 0, 0);
  Point3D q(1, 1, 1);
  Point3D r(0.234, 0.485, 0.763);
  BoundingBox3D bbox(p, q);
  Grid3D grid(bbox, 11, 17, 23);
  GridVectorField3D u(grid);

  class MyField : public VectorField3D
  {
  public:
    Vector3D operator()(const Point3D& p) const
    {
      return Vector3D(sin(p.x)*cos(p.y)*exp(p.z),
                      cos(p.x)*exp(p.y)*sin(p.z),
                      exp(p.x)*sin(p.y)*cos(p.z));
    }
  };

  SECTION("Evaluate")
  {
    REQUIRE(u(p).x == Approx(0.0));
    REQUIRE(u(p).y == Approx(0.0));
    REQUIRE(u(p).z == Approx(0.0));
    REQUIRE(u(q).x == Approx(0.0));
    REQUIRE(u(q).y == Approx(0.0));
    REQUIRE(u(q).z == Approx(0.0));
  }

  SECTION("Interpolate")
  {
    MyField f;
    u.Interpolate(f);
    REQUIRE(u(r).x == Approx(f(r).x).margin(0.01));
    REQUIRE(u(r).y == Approx(f(r).y).margin(0.01));
    REQUIRE(u(r).z == Approx(f(r).z).margin(0.01));
  }
}

TEST_CASE("XMLParser")
{
  const std::string RootPath1 =
      "/home/dtcc/core/unittests/xml-parser-test-files/";
  const std::string RootPath2 =
      "/builds/dtcc3d/core/unittests/xml-parser-test-files/";
  std::string rootPath = RootPath1;

  pugi::xml_document doc;
  std::string filePath = rootPath + "XMLExampleFile1.xml";
  pugi::xml_parse_result result = doc.load_file(filePath.c_str());
  if (!result)
    rootPath = RootPath2;
  filePath = rootPath + "XMLExampleFile1.xml";

  nlohmann::json json = XMLParser::GetJsonFromXML(filePath.c_str(), true);
  REQUIRE(json["city"]["name"] == "Johanneberg");

  json = XMLParser::GetJsonFromXML(filePath.c_str());
  REQUIRE(json["geometry"].is_array());
  REQUIRE(json["geometry"][0]["file_name"] == "Mesh/Buildings.vtk");

  json = XMLParser::GetJsonFromXML((rootPath + "XMLExampleFile2.xml").c_str());
  REQUIRE(json["Test"][0]["TestId"] == "0001");
  REQUIRE(json["example1"]["#content"][0] == 42);
  REQUIRE(json["example1"]["tag"][0] == "Content1");
  REQUIRE(json["example1"].size() == 2);
  REQUIRE(json["example1"]["#content"].size() == 2);
  REQUIRE(json["example2"]["#content"] == 37);
  REQUIRE(json["example2"]["x"] == 5);
  REQUIRE(json["example3"] == -26.3);

  json = XMLParser::GetJsonFromXML((rootPath + "XMLExampleFile3.xml").c_str(),
                                   false);
  REQUIRE(json["item"]["batters"]["batter"][0]["id"] == 1001);
  REQUIRE(json["item"]["topping"][5]["#content"] == "Maple");
}

TEST_CASE("COLORMAPS")
{
  ColorMap cm;
  cm.InsertColor(1, Color(1.0, 1.0, 1.0));
  cm.InsertColor(0, Color(0.0, 0.0, 0.0));

  ColorMap cm2;
  cm2.InsertColor(0.8, Color(0.0, 0.0, 0.0));
  cm2.InsertColor(0.9, Color(1.0, 0.0, 0.0));

  SECTION("Insert")
  {
    REQUIRE(cm.size() == 2);
    REQUIRE(cm.Colors.front().first == 0);
    REQUIRE(cm.Colors.back().first == 1);
  }
  SECTION("Interpolate")
  {

    REQUIRE(cm(0).R == 0.0);
    REQUIRE(cm(0).G == 0.0);
    REQUIRE(cm(0).B == 0.0);

    REQUIRE(cm(1.0).R == 1.0);
    REQUIRE(cm(1.0).G == 1.0);
    REQUIRE(cm(1.0).B == 1.0);

    REQUIRE(cm(1.1).R == 1.0);
    REQUIRE(cm(1.1).G == 1.0);
    REQUIRE(cm(1.1).B == 1.0);

    REQUIRE(cm(0.3).R == 0.3);
    REQUIRE(cm(0.3).G == 0.3);
    REQUIRE(cm(0.3).B == 0.3);

    REQUIRE(cm2(0.85).R == Approx(0.5).margin(0.0001));
    REQUIRE(cm2(0.85).G == 0.0);
    REQUIRE(cm2(0.85).B == 0.0);
  }

  SECTION("Load PNG")
  {
    ColorMap cm3;
    ColorMapIO::ReadPNG(cm3, "../unittests/data/colormap_jet.png");
    REQUIRE(cm3.size() == 256);

    REQUIRE(cm3(0).R == Approx(127 / 255.0).margin(0.0001));
    REQUIRE(cm3(0).G == Approx(0).margin(0.0001));
    REQUIRE(cm3(0).B == Approx(0).margin(0.0001));

    REQUIRE(cm3(0.5).R == Approx(121 / 255.0).margin(0.0001));
    REQUIRE(cm3(0.5).G == Approx(255 / 255.0).margin(0.0001));
    REQUIRE(cm3(0.5).B == Approx(124.5 / 255.0).margin(0.0001));

    REQUIRE(cm3(1).R == Approx(0).margin(0.0001));
    REQUIRE(cm3(1).G == Approx(0).margin(0.0001));
    REQUIRE(cm3(1).B == Approx(127 / 255.0).margin(0.0001));
  }

  SECTION("Write PNG")
  {
    ColorMapIO::WritePNG(cm, "testmap.png");
    ColorMap cm4;
    ColorMapIO::ReadPNG(cm4, "testmap.png");
    REQUIRE(cm4(0.3).R == Approx(0.3).margin(0.0001));
    REQUIRE(cm4(0.3).G == Approx(0.3).margin(0.0001));
    REQUIRE(cm4(0.3).B == Approx(0.3).margin(0.0001));
    remove("testmap.png");
  }

  SECTION("Read cpt")
  {
    ColorMap cm6;
    ColorMapIO::ReadCPT(cm6, "../unittests/data/inferno.cpt");
    REQUIRE(cm6.size() == 255 * 2);
    REQUIRE(cm6(125 / 255.0).R == Approx(183 / 255.0).margin(0.0001));
    REQUIRE(cm6(125 / 255.0).G == Approx(53 / 255.0).margin(0.0001));
    REQUIRE(cm6(125 / 255.0).B == Approx(87 / 255.0).margin(0.0001));

    ColorMap cm7;
    ColorMapIO::ReadCPT(cm7, "../unittests/data/BrBG_11.cpt");
    REQUIRE(cm7(0.5).R == Approx(245 / 255.0).margin(0.0001));
    REQUIRE(cm7(0.5).G == Approx(245 / 255.0).margin(0.0001));
    REQUIRE(cm7(0.5).B == Approx(245 / 255.0).margin(0.0001));
  }

  SECTION("Serialize JSON")
  {
    ColorMap cm3;
    ColorMapIO::ReadPNG(cm3, "../unittests/data/colormap_jet.png");
    JSON::Write(cm3, "testmap.json");
    ColorMap cm5;
    JSON::Read(cm5, "testmap.json");

    REQUIRE(cm5.size() == 256);

    REQUIRE(cm5(0).R == Approx(127 / 255.0).margin(0.0001));
    REQUIRE(cm5(0).G == Approx(0).margin(0.0001));
    REQUIRE(cm5(0).B == Approx(0).margin(0.0001));

    REQUIRE(cm5(0.5).R == Approx(121 / 255.0).margin(0.0001));
    REQUIRE(cm5(0.5).G == Approx(255 / 255.0).margin(0.0001));
    REQUIRE(cm5(0.5).B == Approx(124.5 / 255.0).margin(0.0001));

    REQUIRE(cm5(1).R == Approx(0).margin(0.0001));
    REQUIRE(cm5(1).G == Approx(0).margin(0.0001));
    REQUIRE(cm5(1).B == Approx(127 / 255.0).margin(0.0001));
    // remove("testmap.json");
  }
}

TEST_CASE("citymodel")
{
  const char *fileName1 = "../unittests/data/CityModelExample.json";
  const char *fileName2 = "../unittests/data/CityModelExample2.json";

  for (bool firstRun : {true, false})
  {
    District district;
    BaseArea baseArea;
    nlohmann::json json;

    JSON::Read(json, firstRun ? fileName1 : fileName2);

    if (firstRun)
    {
      JSON::Deserialize(district, json, "606");

      REQUIRE(district.AreaID == "606");
      REQUIRE(district.Name == "Hammarkullen");
      REQUIRE(district.Footprint.Vertices[0].x == Approx(-446.4952344278572));
      REQUIRE(district.Footprint.Vertices[0].y == Approx(150.96198354940861));
      REQUIRE(district.PrimaryAreas.size() == 1);

      PrimaryArea primaryArea = district.PrimaryAreas[0];
      REQUIRE(primaryArea.AreaID == "606");
      REQUIRE(primaryArea.Name == "Hammarkullen");
      REQUIRE(primaryArea.DistrictAreaID == "606");
      REQUIRE(primaryArea.Footprint.Vertices[0].x ==
              Approx(-446.4952344278572));
      REQUIRE(primaryArea.Footprint.Vertices[0].y ==
              Approx(150.96198354940861));

      baseArea = primaryArea.BaseAreas[0];
    }
    else
      JSON::Deserialize(baseArea, json, "60605");

    REQUIRE(baseArea.AreaID == "60605");
    REQUIRE(baseArea.PrimaryAreaID == "606");
    REQUIRE(baseArea.Footprint.Vertices[0].x == Approx(214.59035302355187));
    REQUIRE(baseArea.Footprint.Vertices[0].y == Approx(189.76460300106555));
    REQUIRE(baseArea.Buildings.size() == 1);
    REQUIRE(baseArea.Properties.size() == 2);

    Property property = baseArea.Properties[1];
    REQUIRE(property.FNR == 140127538);
    REQUIRE(property.UUID == "b8574e12-4618-4f1e-91fd-71e2f89b2375");
    REQUIRE(property.Buildings.empty());
    REQUIRE(property.Footprint.Vertices[0].x == Approx(539.78));
    REQUIRE(property.Footprint.Vertices[0].y == Approx(60.7778956));

    Building building = baseArea.Buildings[0];
    REQUIRE(building.UUID == "c8374e11-2767-4f0a-91fd-71d7f89b6681");
    REQUIRE(building.PropertyUUID == building.UUID);
    REQUIRE(building.PropertyFNR == 140029233);
    REQUIRE(building.BaseAreaID == "60605");
    REQUIRE(building.GroundHeight == Approx(34.865));
    REQUIRE(building.Height == Approx(107.574735807257));
    REQUIRE(building.Footprint.Vertices[0].x == Approx(551.020999965025));
    REQUIRE(building.Footprint.Vertices[0].y == Approx(57.5619951048866));

    if (firstRun)
    {
      json.clear();
      JSON::Serialize(district, json);
      JSON::Write(json, fileName2, 4);
    }
  }
}

TEST_CASE("Hashing")
{
  SECTION("Hash Point2D")
  {
    Point2D p(1, 2);
    Info(Hashing::Hex(Hashing::Hash(p)));
  }

  SECTION("Hash Point3D")
  {
    Point3D p(1, 2, 3);
    Info(Hashing::Hex(Hashing::Hash(p)));
  }
}

TEST_CASE("ISO 8559-1 to UTF-8")
{
  std::string testStr("G\345ngv\344g");
  REQUIRE(Utils::Iso88591ToUtf8(testStr) == "Gångväg");
}

TEST_CASE("SHP Extraction")
{
  SECTION("Road data extraction")
  {
    std::vector<Polygon> roads;
    nlohmann::json attributes;
    SHP::Read(roads, "../unittests/data/roadnetwork-torslanda/vl_riks.shp",
              nullptr, nullptr, &attributes);
    REQUIRE(roads.size() == 7);
    REQUIRE(roads[0].Vertices.size() == 13);
    Point2D v = roads[0].Vertices[0];
    REQUIRE(v.x == Approx(306234.751));
    REQUIRE(v.y == Approx(6401785.2819999997));

    json jsonAttrib = attributes["attributes"];
    REQUIRE(jsonAttrib.size() == 7);
    nlohmann::json firstAttr = jsonAttrib[0];
    REQUIRE(firstAttr.size() == 2);
    REQUIRE(firstAttr["KATEGORI"] == "Bilväg");
    REQUIRE(firstAttr["KKOD"] == "5071");

    /// Check correct number of attributes in case of multi-part roads
    SHP::Read(roads, "../unittests/data/roadnetwork-central-gbg/vl_riks.shp",
              nullptr, nullptr, &attributes);
    REQUIRE(attributes["polyToAttribute"].size() == roads.size());
  }
}

TEST_CASE("POINT_CLOUD")
{
  SECTION("READ LAS")
  {
    PointCloud pc;
    LAS::Read(pc,"../unittests/data/minimal_las.las");

    REQUIRE(pc.Points.size()==10);
    for (size_t i = 0; i < pc.Points.size(); i++)
    {
      REQUIRE(pc.Classification[i]==Approx(pc.Points[i].x).margin(1e-6));
    }
  }

  SECTION("ClassificationFilter")
  {
    PointCloud pc;
    pc.Points.push_back(Vector3D(0,0,0));
    pc.Classification.push_back(0);
    pc.Points.push_back(Vector3D(1,0,0));
    pc.Classification.push_back(1);
    pc.Points.push_back(Vector3D(2,0,0));
    pc.Classification.push_back(2);

    PointCloud out_pc = PointCloudProcessor::ClassificationFilter(pc,{1,2});
    REQUIRE(out_pc.Points.size()==2);
    REQUIRE(out_pc.Classification.size()==2);
    REQUIRE(out_pc.Points[0].x==1);
    REQUIRE(out_pc.Points[1].x==2);

  }
}