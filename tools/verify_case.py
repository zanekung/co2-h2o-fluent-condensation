"""Read-only public-case verification. Uses no external packages or solver."""
from pathlib import Path
import ast
import gzip
import hashlib
import json
import re
import sys
from urllib.parse import unquote

ROOT = Path(__file__).resolve().parents[1]
EXCLUDED = {'provenance/release_manifest.json','verification/public_package_check.json'}
IGNORED_DIRS = {'.git','__pycache__','.venv','venv','runtime','runs','scratch','local_artifacts'}

def digest(data):
    return hashlib.sha256(data).hexdigest()

def public_files(root=ROOT):
    return [p for p in sorted(root.rglob('*')) if p.is_file()
            and not any(part in IGNORED_DIRS for part in p.relative_to(root).parts)]

def verify(root=ROOT):
    errors = []
    release = json.loads((root/'provenance/release_manifest.json').read_text(encoding='utf-8'))
    listed = set()
    for item in release['files']:
        name = item['path']
        target = (root/name).resolve()
        if not target.is_relative_to(root) or ':' in name or '..' in Path(name).parts:
            errors.append('Unsafe manifest entry')
            continue
        listed.add(name)
        if not target.is_file() or digest(target.read_bytes()) != item['sha256']:
            errors.append('Hash mismatch: '+name)
    actual = {p.relative_to(root).as_posix() for p in public_files(root)} - EXCLUDED
    if actual != listed:
        errors.append('Inventory mismatch: '+repr(sorted(actual ^ listed)))
    sources = json.loads((root/'provenance/public_source_manifest.json').read_text(encoding='utf-8'))
    for item in sources['files']:
        data = (root/item['path']).read_bytes()
        if digest(data) != item['public_sha256']:
            errors.append('Source copy mismatch: '+item['path'])
        if item['transformation'] == 'lossless_gzip' and digest(gzip.decompress(data)) != item['source_sha256']:
            errors.append('Decompression differs from source: '+item['path'])
        if 'decompressed_public_sha256' in item and digest(gzip.decompress(data)) != item['decompressed_public_sha256']:
            errors.append('Redacted transcript hash mismatch')
    links = 0
    parsed = 0
    for path in public_files(root):
        name = path.relative_to(root).as_posix()
        if path.suffix == '.md':
            for link in re.findall(r'!?\[[^\]]*\]\(([^)]+)\)',path.read_text(encoding='utf-8')):
                link = link.strip('<>').split('#')[0]
                if not link or re.match(r'\w+://',link): continue
                links += 1
                if not (path.parent/unquote(link)).exists(): errors.append('Broken link: '+name+' -> '+link)
        if path.suffix == '.py':
            ast.parse(path.read_text(encoding='utf-8-sig'),filename=name)
            parsed += 1
        native_allowed = {'native/final-100000.cas.h5', 'native/final-100000.dat.h5'}
        forbidden_suffixes = {'.docx','.dll','.exe','.pyd','.whl','.lic','.serverbkup','.clientbkup','.ctxz','.ar'}
        if path.suffix.lower() == '.h5' and name not in native_allowed:
            errors.append('Unexpected native binary: '+name)
        if path.suffix.lower() in forbidden_suffixes or path.name == '.local_connection.json':
            errors.append('Excluded private/proprietary artifact: '+name)
        data = path.read_bytes()
        if path.suffix == '.gz': data = gzip.decompress(data)
        # Text and embedded PNG metadata are scanned; this is not an exhaustive secret detector.
        if re.search(rb'(?i)[a-z]:[\\/]+users[\\/]+[a-z0-9_.-]+',data):
            errors.append('Personal Windows home path: '+name)
        if re.search(rb'(?i)"(?:password|api_key|access_token|secret_key)"\s*:\s*"[^"\s]+"', data):
            errors.append('Nonempty credential-like JSON field: '+name)
    if (root/'workflow').exists() or (root/'requirements-fluent.txt').exists():
        errors.append('Removed Fluent preparation workflow is still present')
    native = json.loads((root/'verification/native_files_check.json').read_text(encoding='utf-8'))
    for item in native['files']:
        if digest((root/item['path']).read_bytes()) != item['public_sha256']:
            errors.append('Native audit hash mismatch: '+item['path'])
    for name in ['co2_h2o_condensation.c','condensation_core.c','condensation_core.h','quasi1d_initialization.c']:
        if (root/'native'/name).read_bytes() != (root/'udf'/name).read_bytes():
            errors.append('Native UDF copy differs: '+name)
    from analyze_results import analyze
    analysis = analyze(root)
    if analysis['status'] != 'MATCHES_RECORDED_EXPORT_STRUCTURE':
        errors.append('Numerical export structure differs from the recorded case')
    return {'status':'PASS' if not errors else 'FAIL',
            'scope':'Local integrity, source correspondence, links, Python syntax, basic public-scope scan and CSV/history checks; no Fluent or physical validation.',
            'files_checked':len(listed),'source_artifacts_checked':len(sources['files']),
            'links_checked':links,'python_modules_parsed':parsed,
            'native_files_hash_checked':len(native['files']),
            'native_semantic_audit':'Recorded separately; run tools/verify_native.py with h5py to repeat current-file HDF checks.',
            'native_fluent_read_status':native['fluent_read_status'],
            'data_summary':{k:analysis[k] for k in ['cells','columns','history_rows','nonfinite_values','radius_mask']},
            'privacy_scan_limit':'Pattern checks, not a guarantee about every possible identifier or legal permission.',
            'errors':errors}

def main():
    report = verify()
    print(json.dumps(report,ensure_ascii=False,indent=2))
    return 0 if report['status'] == 'PASS' else 1

if __name__ == '__main__':
    sys.exit(main())
