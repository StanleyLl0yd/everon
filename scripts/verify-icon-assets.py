from __future__ import annotations

import hashlib
from pathlib import Path

CANONICAL_SOURCE = "assets/Everon.png"
CANONICAL_SOURCE_SHA256 = "0614908b53f8d4c7b2990857d82535580f7b8edabfd473c50e26e78eb8018fdd"
REQUIRED = {
    CANONICAL_SOURCE,
    "src/Everon.ico",
    "macos/resources/Everon.icns",
}


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


manifest = Path("assets/icon-manifest.sha256")
entries: dict[str, str] = {}
for raw_line in manifest.read_text(encoding="utf-8").splitlines():
    line = raw_line.strip()
    if not line:
        continue
    expected, relative = line.split(maxsplit=1)
    entries[relative] = expected.lower()

if set(entries) != REQUIRED:
    raise SystemExit(f"unexpected icon manifest entries: {sorted(entries)}")

if entries[CANONICAL_SOURCE] != CANONICAL_SOURCE_SHA256:
    raise SystemExit("canonical Everon PNG hash changed")

for relative, expected in entries.items():
    actual = sha256(Path(relative))
    if actual != expected:
        raise SystemExit(f"icon asset hash mismatch: {relative}")

print("Everon icon assets verified")
