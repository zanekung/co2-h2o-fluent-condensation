"""Inspect numerical exports using only the standard library; no CFD solve."""
from pathlib import Path
import csv
import gzip
import json
import math
import sys

ROOT = Path(__file__).resolve().parents[1]

def analyze(root=ROOT):
    final = root / 'data/final/all-cells-100000.csv.gz'
    minima, maxima = {}, {}
    ids = set()
    rows = valid = nonfinite = 0
    radius_valid_min, radius_valid_max = float('inf'), -float('inf')
    with gzip.open(final, 'rt', encoding='utf-8-sig', newline='') as f:
        reader = csv.reader(f)
        names = [name.strip() for name in next(reader)]
        for raw in reader:
            rows += 1
            if len(raw) != len(names):
                raise ValueError('Inconsistent CSV columns at data row ' + str(rows))
            values = [float(v) for v in raw]
            nonfinite += sum(not math.isfinite(v) for v in values)
            row = dict(zip(names, values))
            ids.add(row['cellnumber'])
            for name, value in row.items():
                minima[name] = min(minima.get(name, value), value)
                maxima[name] = max(maxima.get(name, value), value)
            if row['liquid-loading-kg-liquid-per-kg-gas'] > 1e-12 and row['droplet-number-per-kg-gas-div-1e15'] > 0:
                valid += 1
                radius = row['droplet-radius-m'] * 1e9
                radius_valid_min = min(radius_valid_min, radius)
                radius_valid_max = max(radius_valid_max, radius)
    history_rows = history_nonfinite = 0
    with gzip.open(root/'data/history/physics-history.out.gz', 'rt', encoding='utf-8') as f:
        for line in f:
            line = line.strip()
            if not line or not line[0].isdigit():
                continue
            values = [float(v) for v in line.split()]
            if len(values) != 28 or values[0] != history_rows:
                raise ValueError('History is not the expected consecutive 28-column series')
            history_nonfinite += sum(not math.isfinite(v) for v in values)
            history_rows += 1
    match = (rows == 30000 and len(names) == 39 and len(ids) == 30000
             and nonfinite == 0 and valid == 15035 and history_rows == 100001 and history_nonfinite == 0)
    return {
        'status': 'MATCHES_RECORDED_EXPORT_STRUCTURE' if match else 'DIFFERS_FROM_RECORDED_EXPORT',
        'scope': 'CSV and history structure, finite values and display-mask diagnostics. No volume/mass integrals, HDF audit, solver execution or physical validation.',
        'cells': rows, 'columns': len(names), 'unique_cell_ids': len(ids), 'nonfinite_values': nonfinite,
        'history_rows': history_rows, 'history_steps': [0, history_rows-1], 'history_nonfinite': history_nonfinite,
        'radius_mask': {'liquid_loading_min_exclusive': 1e-12, 'number_min_exclusive': 0,
                        'valid_cells':valid, 'radius_nm_min':radius_valid_min, 'radius_nm_max':radius_valid_max,
                        'raw_global_radius_nm_max':maxima['droplet-radius-m']*1e9},
        'field_ranges':{name:{'min':minima[name], 'max':maxima[name]} for name in names[1:]},
        'cell_counts_are_not_mass_fractions':True}

def main():
    report = analyze()
    print(json.dumps(report, ensure_ascii=False, indent=2, allow_nan=False))
    return 0 if report['status'] == 'MATCHES_RECORDED_EXPORT_STRUCTURE' else 1

if __name__ == '__main__':
    sys.exit(main())
