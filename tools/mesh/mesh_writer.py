"""Minimal mesh writers extracted from the historical mesh utility.

No geometry defaults, alternate mesh levels, external imports or command-line
entry point are retained. All geometry parameters are explicit. Writers use
exclusive file creation so existing files are never overwritten. Numeric
formatting, connectivity order and floating-point operations are unchanged.
Only Python 3.10+ standard-library modules are required.
"""
from __future__ import annotations

import math
from dataclasses import dataclass
from pathlib import Path


@dataclass(frozen=True)
class Geometry:
    inlet_straight_length: float
    converging_length: float
    diverging_length: float
    inlet_radius: float
    throat_radius: float
    exit_radius: float


def node_id(i: int, j: int, radial_nodes: int) -> int:
    return i * radial_nodes + j + 1


def quad_metrics(points: tuple[tuple[float, float], ...]) -> tuple[float, float]:
    (x0, y0), (x1, y1), (x2, y2), (x3, y3) = points
    area = 0.5 * abs(
        x0 * y1
        + x1 * y2
        + x2 * y3
        + x3 * y0
        - y0 * x1
        - y1 * x2
        - y2 * x3
        - y3 * x0
    )
    lengths = [
        math.hypot(x1 - x0, y1 - y0),
        math.hypot(x2 - x1, y2 - y1),
        math.hypot(x3 - x2, y3 - y2),
        math.hypot(x0 - x3, y0 - y3),
    ]
    positive = [value for value in lengths if value > 0.0]
    aspect = max(positive) / min(positive)
    return area, aspect


def write_gmsh(
    path: Path,
    points: list[tuple[float, float]],
    line_elements: list[tuple[int, int, int]],
    quads: list[tuple[int, int, int, int]],
    centerline_name: str = "axis",
) -> None:
    with path.open("x", encoding="ascii", newline="\n") as stream:
        stream.write("$MeshFormat\n2.2 0 8\n$EndMeshFormat\n")
        stream.write("$PhysicalNames\n5\n")
        stream.write('1 1 "inlet"\n')
        stream.write('1 2 "outlet"\n')
        stream.write(f'1 3 "{centerline_name}"\n')
        stream.write('1 4 "wall"\n')
        stream.write('2 5 "fluid"\n')
        stream.write("$EndPhysicalNames\n")
        stream.write(f"$Nodes\n{len(points)}\n")
        for index, (x, y) in enumerate(points, start=1):
            stream.write(f"{index} {x:.16e} {y:.16e} 0.0\n")
        stream.write("$EndNodes\n")
        total_elements = len(line_elements) + len(quads)
        stream.write(f"$Elements\n{total_elements}\n")
        element_id = 1
        for physical_id, first, second in line_elements:
            stream.write(
                f"{element_id} 1 2 {physical_id} {physical_id} {first} {second}\n"
            )
            element_id += 1
        for n0, n1, n2, n3 in quads:
            stream.write(f"{element_id} 3 2 5 5 {n0} {n1} {n2} {n3}\n")
            element_id += 1
        stream.write("$EndElements\n")


def write_fluent_native(
    path: Path,
    points: list[tuple[float, float]],
    xs: list[float],
    n_radial: int,
    geometry: Geometry,
    *,
    throat_x: float | None = None,
    centerline_zone_type: str = "axis",
    centerline_bc_type: int = 37,
    centerline_zone_name: str = "axis",
    title: str = "Codex native Fluent ASCII structured nozzle mesh",
) -> int:
    """Write Fluent's documented legacy ASCII mesh sections 10/12/13/39."""
    radial_nodes = n_radial + 1
    axial_cells = len(xs) - 1

    def cell_id(i: int, j: int) -> int:
        return i * n_radial + j + 1

    interior_faces: list[tuple[int, int, int, int]] = []
    throat_faces: list[tuple[int, int, int, int]] = []
    target_throat_x = (
        geometry.converging_length if throat_x is None else throat_x
    )
    throat_station = min(
        range(len(xs)), key=lambda index: abs(xs[index] - target_throat_x)
    )

    # Vertical internal faces: bottom->top makes c0 the upstream/left cell
    # and c1 the downstream/right cell in Fluent's 2-D orientation.
    for i in range(1, axial_cells):
        target = throat_faces if i == throat_station else interior_faces
        for j in range(n_radial):
            target.append(
                (
                    node_id(i, j, radial_nodes),
                    node_id(i, j + 1, radial_nodes),
                    cell_id(i - 1, j),
                    cell_id(i, j),
                )
            )
    # Horizontal internal faces: left->right makes c0 the upper cell and c1
    # the lower cell.
    for i in range(axial_cells):
        for j in range(1, n_radial):
            interior_faces.append(
                (
                    node_id(i, j, radial_nodes),
                    node_id(i + 1, j, radial_nodes),
                    cell_id(i, j),
                    cell_id(i, j - 1),
                )
            )

    inlet_faces = [
        (
            node_id(0, j + 1, radial_nodes),
            node_id(0, j, radial_nodes),
            cell_id(0, j),
            0,
        )
        for j in range(n_radial)
    ]
    outlet_faces = [
        (
            node_id(axial_cells, j, radial_nodes),
            node_id(axial_cells, j + 1, radial_nodes),
            cell_id(axial_cells - 1, j),
            0,
        )
        for j in range(n_radial)
    ]
    axis_faces = [
        (
            node_id(i, 0, radial_nodes),
            node_id(i + 1, 0, radial_nodes),
            cell_id(i, 0),
            0,
        )
        for i in range(axial_cells)
    ]
    wall_faces = [
        (
            node_id(i + 1, n_radial, radial_nodes),
            node_id(i, n_radial, radial_nodes),
            cell_id(i, n_radial - 1),
            0,
        )
        for i in range(axial_cells)
    ]
    face_zones = [
        (1, 2, "interior", "interior-fluid", interior_faces),
        (2, 2, "interior", "throat-partition", throat_faces),
        (6, 4, "pressure-inlet", "inlet", inlet_faces),
        (7, 5, "pressure-outlet", "outlet", outlet_faces),
        (
            8,
            centerline_bc_type,
            centerline_zone_type,
            centerline_zone_name,
            axis_faces,
        ),
        (9, 3, "wall", "wall", wall_faces),
    ]
    total_faces = sum(len(zone[-1]) for zone in face_zones)
    total_cells = axial_cells * n_radial

    def hx(value: int) -> str:
        return format(value, "x")

    with path.open("x", encoding="ascii", newline="\n") as stream:
        stream.write(f'(1 "{title}")\n')
        stream.write("(2 2)\n")
        stream.write(f"(10 (0 1 {hx(len(points))} 0))\n")
        stream.write(f"(12 (0 1 {hx(total_cells)} 0))\n")
        stream.write(f"(13 (0 1 {hx(total_faces)} 0))\n")
        stream.write(f"(10 (4 1 {hx(len(points))} 1 2)(\n")
        for x, y in points:
            stream.write(f"{x:.16e} {y:.16e}\n")
        stream.write("))\n")
        stream.write(f"(12 (5 1 {hx(total_cells)} 1 3))\n")

        face_index = 1
        for zone_id, bc_type, _, _, faces in face_zones:
            first = face_index
            last = face_index + len(faces) - 1
            stream.write(
                f"(13 ({hx(zone_id)} {hx(first)} {hx(last)} {hx(bc_type)} 2)(\n"
            )
            for n0, n1, c0, c1 in faces:
                stream.write(f"{hx(n0)} {hx(n1)} {hx(c0)} {hx(c1)}\n")
            stream.write("))\n")
            face_index = last + 1

        stream.write("(39 (5 fluid fluid-domain 1)())\n")
        for zone_id, _, zone_type, zone_name, _ in face_zones:
            stream.write(f"(39 ({zone_id} {zone_type} {zone_name} 1)())\n")
    return total_faces
