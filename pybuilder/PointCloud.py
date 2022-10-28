import _pybuilder
import os

def readLasFiles(las_path, extra_data = True):
    las_path = str(las_path)
    
    #print(f"loading las from {las_path}")
    if os.path.isdir(las_path):
        pc = _pybuilder.LASReadDirectory(las_path, extra_data)
    elif os.path.isfile(las_path):
        pc = _pybuilder.LASReadFile(las_path,extra_data)
    return pc

def getLasBounds(las_path):
    las_path = str(las_path)
    if not os.path.isdir(las_path):
        print(f"{las_path} is not a directory")
        return None
    bbox = _pybuilder.LASBounds(las_path)
    return bbox
 