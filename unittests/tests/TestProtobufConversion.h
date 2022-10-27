#include "protobuf/dtcc.pb.h"
#include "protobuf/include/PointCloudMethods.h"

#include "PointCloud.h"
#include "converter/ProtobufConverter.h"

using namespace DTCC_BUILDER;

TEST_CASE("Protobuf to PointCloud")
{
  SECTION("Load protobuf pointcloud")
  {
    std::string pbFilePath = RootPath + "data/MinimalCase/pointcloud.las.pb";
    DTCC::PointCloud pb_pointCloud;
    std::fstream input(pbFilePath, std::ios::in | std::ios::binary);
    REQUIRE(pb_pointCloud.ParseFromIstream(&input));
    REQUIRE(pb_pointCloud.points().size() == 8148);
  }

  SECTION("Convert to PointCloud")
  {
    std::string pbFilePath = RootPath + "data/MinimalCase/pointcloud.las.pb";
    DTCC::PointCloud pb_pointCloud;
    std::fstream input(pbFilePath, std::ios::in | std::ios::binary);
    pb_pointCloud.ParseFromIstream(&input);
    PointCloud pc = ProtobufConverter::ConvertPointCloud(pb_pointCloud);

    REQUIRE(pc.Points.size() == 8148);
    REQUIRE(pc.Classifications.size() == 8148);
  }
}