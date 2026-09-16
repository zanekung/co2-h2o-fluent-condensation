"""Compile and run the independent C tests; never starts Fluent.

Provide a configured GCC/Clang or MSVC/clang-cl compiler explicitly. Compiler
headers, SDKs and libraries are external prerequisites, not downloaded here.
"""
from pathlib import Path
import argparse
from datetime import datetime, timezone
import hashlib
import json
import os
import re
import shutil
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]

def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--cc', required=True, help='Compiler executable or command name')
    ap.add_argument('--style', choices=['auto', 'gnu', 'msvc'], default='auto')
    ap.add_argument('--cflag', action='append', default=[], help='Repeat as --cflag=VALUE')
    ap.add_argument('--ldflag', action='append', default=[], help='Repeat as --ldflag=VALUE')
    args = ap.parse_args(argv)
    compiler = shutil.which(args.cc)
    if compiler is None:
        print(json.dumps({'status':'COMPILER_UNAVAILABLE','solver_started':False}))
        return 2
    style = args.style
    if style == 'auto':
        style = 'msvc' if Path(compiler).stem.lower() in {'cl','clang-cl'} else 'gnu'
    tests = [('core', [ROOT/'tests/core/test_core.c', ROOT/'udf/condensation_core.c']),
             ('quasi1d', [ROOT/'tests/core/test_quasi1d.c'])]
    report = {'checked_at_utc':datetime.now(timezone.utc).isoformat(), 'status':'PASS',
              'scope':'Independent C core and frozen-composition quasi-1D consistency only. No Fluent hooks, CFD solve or physical validation.',
              'compiler_style':style, 'solver_started':False, 'tests':[]}
    inc = (ROOT/'tests/core/quasi1d_initialization.inc').read_bytes()
    actual = (ROOT/'udf/quasi1d_initialization.c').read_bytes()
    report['quasi1d_test_source_matches_udf_source_bytes'] = inc == actual
    if inc != actual:
        raise ValueError('Quasi-1D test copy differs from the packaged UDF implementation')
    env = os.environ.copy()
    env['PATH'] = str(Path(compiler).parent) + os.pathsep + env.get('PATH','')
    with tempfile.TemporaryDirectory(prefix='condensation_core_') as tmp:
        build = Path(tmp)
        def redact(value):
            for original, replacement in [(str(build),'<BUILD>'), (str(ROOT),'<CASE>'),
                                           (compiler,'<COMPILER>'), (str(Path.home()),'<HOME>')]:
                value = value.replace(original, replacement).replace(original.replace('\\','/'), replacement)
            return value
        for name, sources in tests:
            exe = build/(name + ('.exe' if os.name == 'nt' else ''))
            includes = [ROOT/'udf', ROOT/'tests/core']
            if style == 'msvc':
                command = [compiler,'/nologo','/O2','/W4'] + ['/I'+str(p) for p in includes] + args.cflag
                command += [str(p) for p in sources] + ['/Fe'+str(exe), '/link'] + args.ldflag
            else:
                command = [compiler,'-std=c99','-O2','-Wall','-Wextra'] + ['-I'+str(p) for p in includes] + args.cflag
                command += [str(p) for p in sources] + ['-o',str(exe)] + args.ldflag
                if os.name != 'nt': command.append('-lm')
            result = {'name':name, 'sources':[p.relative_to(ROOT).as_posix() for p in sources]}
            try:
                compiled = subprocess.run(command, cwd=build, env=env, capture_output=True,
                                          text=True, encoding='utf-8', errors='replace', timeout=120)
                result.update(compile_exit=compiled.returncode, compile_output=redact(compiled.stdout+compiled.stderr))
                if compiled.returncode == 0:
                    ran = subprocess.run([str(exe)], cwd=build, capture_output=True, text=True,
                                         encoding='utf-8', errors='replace', timeout=120)
                    result.update(test_exit=ran.returncode, test_output=redact(ran.stdout+ran.stderr))
                else:
                    result['test_exit'] = None
            except (OSError, subprocess.TimeoutExpired) as exc:
                result.update(test_exit=None, error=redact(str(exc)))
            if result.get('compile_exit') != 0 or result.get('test_exit') != 0:
                report['status'] = 'FAIL_OR_ENVIRONMENT_BLOCKED'
            report['tests'].append(result)
    print(json.dumps(report, ensure_ascii=False, indent=2))
    return 0 if report['status'] == 'PASS' else 1

if __name__ == '__main__':
    sys.exit(main())
