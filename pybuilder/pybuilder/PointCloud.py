import _pybuilder
import os
import numpy
from typing import List, Tuple


def readLasFiles(las_path, extra_data=True):
    las_path = str(las_path)

    # print(f"loading las from {las_path}")
    if os.path.isdir(las_path):
        pc = _pybuilder.LASReadDirectory(las_path, extra_data)
    elif os.path.isfile(las_path):
        pc = _pybuilder.LASReadFile(las_path, extra_data)
    return pc


def setOrigin(
    point_cloud: _pybuilder.PointCloud, origin: Tuple[float, float]
) -> _pybuilder.PointCloud:
    return _pybuilder.SetPointCloudOrigin(point_cloud, origin)


def getLasBounds(las_path):
    las_path = str(las_path)
    if not os.path.isdir(las_path):
        print(f"{las_path} is not a directory")
        return None
    bbox = _pybuilder.LASBounds(las_path)
    return bbox


def globalOutlierRemover(point_cloud, outlierMargin):
    point_cloud = _pybuilder.GlobalOutlierRemover(point_cloud, outlierMargin)
    return point_cloud


def VegetationFilter(point_cloud):
    point_cloud = _pybuilder.VegetationFilter(point_cloud)
    return point_cloud


def pointCloud2numpy(point_cloud):
    pts = point_cloud.points
    pts = [[p.x, p.y, p.z] for p in pts]
    return numpy.array(pts)

def setOrigin(point_cloud, origin):
    return _pybuilder.SetPointCloudOrigin(point_cloud,origin)

def getBounds(point_cloud):
    pts = pointCloud2numpy(point_cloud)
    x_min, y_min, z_min = pts.min(axis=0)
    x_max, y_max, z_max = pts.max(axis=0)
    return (x_min, y_min, x_max, y_max)
