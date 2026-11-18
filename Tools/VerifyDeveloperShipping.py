"""Verify the compiled Shipping boundary and its Development positive control."""
from pathlib import Path
import json
root=Path(__file__).resolve().parents[1]
ship=root/'Binaries/Win64/ConflictOfInterest-Win64-Shipping.exe'
dev=root/'Binaries/Win64/UnrealEditor-ConflictOfInterest.dll'
markers=['COI_DEVELOPER_CONSOLE_AP07A','money add COMPANY SIGNED_CENTS','goods add COMPANY STORAGE GOOD KG']
results=[]
for marker in markers:
    encodings=[marker.encode('utf-8'),marker.encode('utf-16-le')]
    in_dev=any(x in dev.read_bytes() for x in encodings)
    in_ship=any(x in ship.read_bytes() for x in encodings)
    results.append(dict(marker=marker,development_present=in_dev,shipping_present=in_ship))
    assert in_dev and not in_ship, f'Shipping exclusion failed: {marker}'
# The old mutating reflected Exec entry was removed entirely; no alternative player route.
assert b'SimRestart' not in ship.read_bytes() and 'SimRestart'.encode('utf-16-le') not in ship.read_bytes()
print(json.dumps(dict(shipping=str(ship),checks=results,legacy_mutating_exec_absent=True,compile_time_boundary='static_assert in DeveloperConsoleTests.cpp',result='PASS'),indent=2))
