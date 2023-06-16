import json
from pathlib import Path

_parameters = {}

_parameters["model_name"] = "DTCC"
_parameters["auto_domain"] = True
_parameters["generate_surface_mesh"] = True
_parameters["generate_volume_mesh"] = True
_parameters["write_json"] = True
_parameters["write_vtk"] = True
_parameters["write_stl"] = True
_parameters["write_obj"] = False
_parameters["write_protobuf"] = True

_parameters["debug"] = False

_parameters["ground_smoothing"] = 5

_parameters["domain_margin"] = 10.0
_parameters["x0"] = 0.0
_parameters["y0"] = 0.0
_parameters["x_min"] = 0.0
_parameters["y_min"] = 0.0
_parameters["x_max"] = 0.0
_parameters["y_max"] = 0.0
_parameters["elevation_model_resolution"] = 1.0
_parameters["min_building_distance"] = 1.0
_parameters["min_building_height"] = 2.5
_parameters["min_vertex_distance"] = 1.0
_parameters["ground_margin"] = 1.0
_parameters["mesh_resolution"] = 10.0
_parameters["domain_height"] = 100.0
_parameters["groun_percentile"] = 0.5
_parameters["roof_percentile"] = 0.9
_parameters["outlier_margin"] = 2.0
_parameters["min_building_size"] = 15.0
_parameters["uuid_field"] = "uuid"
_parameters["height_field"] = ""

_parameters["statistical_outlier_remover"] = True
_parameters["outlier_neighbors"] = 5
_parameters["roof_outlier_margin"] = 1.5

_parameters["ransac_outlier_remover"] = True
_parameters["ransac_outlier_margin"] = 3.0
_parameters["ransac_iterations"] = 250

_parameters["naive_vegitation_filter"] = True
_parameters["data_directory"] = ""
_parameters["buildings_filename"] = "PropertyMap.shp"
_parameters["pointcloud_directory"] = ""
_parameters["output_directory"] = ""


def load_parameters(file_path=None, project_path="."):
    global _parameters

    if file_path is None:
        p = _parameters.copy()

    else:
        file_path = Path(file_path)
        print(file_path)
        if file_path.is_dir():
            file_path = file_path / "Parameters.json"
        if not file_path.exists():
            print(
                f"Parameters file {file_path} does not exist, using default parameters"
            )
            p = _parameters.copy()
        else:
            with open(file_path) as src:
                loaded_parameters = json.load(src)
            p = _parameters.copy()
            p.update(loaded_parameters)
    p = set_directories(p, project_path)
    return p


def set_directories(p, project_path):
    if p["data_directory"] == "":
        p["data_directory"] = project_path
    if p["output_directory"] == "":
        p["output_directory"] = p["data_directory"]

    p["data_directory"] = Path(p["data_directory"])
    p["output_directory"] = Path(p["output_directory"])
    p["output_directory"].mkdir(parents=True, exist_ok=True)
    if p["pointcloud_directory"] == "":
        p["pointcloud_directory"] = p["data_directory"]
    else:
        p["pointcloud_directory"] = Path(p["pointcloud_directory"])
        if not p["pointcloud_directory"].is_absolute():
            p["pointcloud_directory"] = p["data_directory"] / p["pointcloud_directory"]

    return p