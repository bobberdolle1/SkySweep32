"""PlatformIO pre-build gate for the canonical Rev C hardware contract."""

from pathlib import Path
import subprocess
import sys

Import("env")  # type: ignore[name-defined]  # Provided by PlatformIO/SCons.

project_dir = Path(env.subst("$PROJECT_DIR"))  # type: ignore[name-defined]
checks = [
    project_dir / "tools" / "development" / "generate_rev_c_pinmap.py",
    project_dir / "tools" / "development" / "generate_dashboard.py",
]
for checker in checks:
    subprocess.run(
        [sys.executable, str(checker), "--check"],
        cwd=project_dir,
        check=True,
    )
