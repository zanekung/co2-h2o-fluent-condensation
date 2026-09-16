"""Rebuild the exact 30000-cell input mesh used by the recorded Fluent run.

Offline geometry generation only: this does not start Fluent or calculate a
condensation field. The numerical expressions and evaluation order come from
the historical build_paper_mesh.py. No values are fitted to manuscript images.
Requires Python 3.10+; no third-party packages are needed.

Usage:
    python build_paper_mesh.py --output-dir PATH_TO_NEW_OR_EMPTY_OUTPUT_FOLDER

The output directory is mandatory. Existing output files are never replaced.
"""
from __future__ import annotations

import argparse
import csv
import json
import math
from pathlib import Path

from mesh_writer import Geometry, write_fluent_native, write_gmsh, node_id, quad_metrics


OUTPUT_FILES = (
    'paper_nozzle_30000.msh',
    'paper_nozzle_30000.gmsh',
    'mesh_metadata.json',
    'wall_coordinates_m.csv',
)


def build(output_dir: Path) -> dict:
    """Create four mesh/geometry files; refuse any pre-existing output name."""
    out = Path(output_dir).expanduser().resolve()
    if out.exists() and not out.is_dir():
        raise NotADirectoryError(f'Output directory is a file: {out}')
    conflicts = [out / name for name in OUTPUT_FILES
                 if (out / name).exists() or (out / name).is_symlink()]
    if conflicts:
        raise FileExistsError('Refusing to overwrite existing output: '
                              + ', '.join(str(path) for path in conflicts))
    out.mkdir(parents=True, exist_ok=True)
    g=Geometry(0.0,0.100,0.150,0.01995,0.00645,0.00775)
    def radius(x):
        if x <= .1:
            q=x/.1
            f=(1-q*q)**2/(1+q*q/3)**3
            return g.throat_radius/math.sqrt(1-(1-(g.throat_radius/g.inlet_radius)**2)*f)
        return g.throat_radius+(g.exit_radius-g.throat_radius)*(x-.1)/.15

    # 400 axial x 75 radial cells. Moderate axial spacing, clustered near throat.
    segments=[(0,.07,70),(.07,.13,180),(.13,.25,150)]
    xs=[0.0]
    for a,b,n in segments: xs.extend(a+(b-a)*i/n for i in range(1,n+1))
    nr=75; nn=nr+1
    # Hyperbolic wall clustering; actual y+ must be measured after the run.
    eta=[math.tanh(3.0*j/nr)/math.tanh(3.0) for j in range(nr+1)]
    points=[(x,radius(x)*e) for x in xs for e in eta]
    quads=[(node_id(i,j,nn),node_id(i+1,j,nn),node_id(i+1,j+1,nn),node_id(i,j+1,nn)) for i in range(400) for j in range(nr)]
    lines=[]
    for j in range(nr):
        lines.extend([(1,node_id(0,j,nn),node_id(0,j+1,nn)),(2,node_id(400,j,nn),node_id(400,j+1,nn))])
    for i in range(400):
        lines.extend([(3,node_id(i,0,nn),node_id(i+1,0,nn)),(4,node_id(i,nr,nn),node_id(i+1,nr,nn))])

    write_fluent_native(out/'paper_nozzle_30000.msh',points,xs,nr,g,title='Manuscript geometry 250mm 30000 cells; no solution data')
    write_gmsh(out/'paper_nozzle_30000.gmsh',points,lines,quads)
    areas=[quad_metrics(tuple(points[k-1] for k in q))[0] for q in quads]
    stats={'geometry_m':g.__dict__,'cells':len(quads),'nodes':len(points),'min_plane_cell_area_m2':min(areas),'wall_first_cell_height_throat_m':g.throat_radius*(eta[-1]-eta[-2]),'throat_x_m':.1,'wall_curve':'Rt/sqrt(1-(1-(Rt/Rin)^2)*(1-(x/L)^2)^2/(1+(x/L)^2/3)^3)','curve_reference':'https://doi.org/10.1016/j.csite.2023.103089','mesh_origin':'programmatic structured mesh, not ANSYS Mechanical','status':'geometry only, Fluent quality check required; no mesh independence claim'}
    with (out/'mesh_metadata.json').open('x',encoding='utf-8') as stream:
        stream.write(json.dumps(stats,indent=2))
    with (out/'wall_coordinates_m.csv').open('x',newline='') as f:
        w=csv.writer(f);w.writerow(['x_m','radius_m']);w.writerows((x,radius(x)) for x in xs)
    return stats



def main(argv=None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output-dir', required=True, type=Path,
                        help='Explicit output directory; existing output files cause an error.')
    args = parser.parse_args(argv)
    try:
        stats = build(args.output_dir)
    except (FileExistsError, NotADirectoryError) as exc:
        parser.error(str(exc))
    print(json.dumps(stats, indent=2))
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
