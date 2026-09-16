"""Optional read-only HDF content verification; requires h5py and NumPy.

Checks the distributed files against the recorded source-comparison audit.
Does not launch Fluent, compile UDFs, initialize a field or run a calculation.
"""
from pathlib import Path
import hashlib
import json
import re
import sys

ROOT=Path(__file__).resolve().parents[1]

def main():
    try:
        import h5py
        import numpy as np
    except ImportError:
        print('Optional HDF check requires h5py and NumPy. Native Fluent viewing does not require Python.')
        return 2
    audit=json.loads((ROOT/'verification/native_files_check.json').read_text(encoding='utf-8'))
    checked=0
    for entry in audit['files']:
        path=ROOT/entry['path']
        assert hashlib.sha256(path.read_bytes()).hexdigest()==entry['public_sha256'],path.name
        with h5py.File(path,'r') as handle:
            datasets=[]
            handle.visititems(lambda key,obj: datasets.append(key) if isinstance(obj,h5py.Dataset) else None)
            assert sorted(datasets)==sorted(d['path'] for d in entry['datasets'])
            for record in entry['datasets']:
                obj=handle[record['path']]
                values=np.asarray(obj[()])
                assert list(obj.shape)==record['shape'] and str(obj.dtype)==record['dtype']
                assert hashlib.sha256(values.tobytes()).hexdigest()==record['sha256_values'],record['path']
                if obj.dtype.kind in 'iufb': assert np.isfinite(values).all(),record['path']
                if obj.dtype.kind in 'SO':
                    raw=b'\n'.join(v if isinstance(v,bytes) else str(v).encode() for v in values.reshape(-1))
                    assert not re.search(rb'(?i)[a-z]:[\\/]+users[\\/]+',raw),record['path']
                checked+=1
    print(json.dumps({'status':'PASS_RECORDED_HDF_CONTENT','datasets_checked':checked,
                      'fluent_import_test_performed':False,
                      'scope':'Current files match the recorded numerical and metadata audit. Does not prove Fluent compatibility or model validity.'},indent=2))
    return 0

if __name__=='__main__':
    sys.exit(main())
