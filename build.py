import argparse
from pathlib import Path
import subprocess
import sys

# builds and runs
targets = ["Main", "Tests"]
targetCount = len(targets)
parser = argparse.ArgumentParser()
parser.add_argument("target", nargs="?", choices=targets)

for i, t in enumerate(targets):
    print(f"{i + 1}. {t}")
target = parser.parse_args().target or input(f"Choose {' / '.join(targets)} (1-{targetCount}): ")
if target.isdigit():
    targetIndex = int(target) - 1
    if 0 <= targetIndex < targetCount:
        target = targets[targetIndex]

if target not in targets:
    parser.error(f"Choose one of: {', '.join(targets)}")

root = Path(__file__).parent
try:
    subprocess.run(["cmake", "-S", str(root), "-B", str(root / "build"),
                    "-G", "Ninja", "-DCMAKE_BUILD_TYPE=Debug"], check=True)
    subprocess.run(["cmake", "--build", str(root / "build"), "--target", target], check=True)
    executable = root / "build" / (target + (".exe" if sys.platform == "win32" else ""))
    # open a terminal window (vscode terminal space is too small)
    if sys.platform == "win32":
        subprocess.run(["start", "cmd", "/k", str(executable)], cwd=root, shell=True)
    elif sys.platform == "darwin":
        subprocess.run(["open", "-a", "Terminal", str(executable)], cwd=root)
    else:
        subprocess.run(["x-terminal-emulator", "-e", str(executable)], cwd=root)
        sys.exit(subprocess.run([str(executable)], cwd=root).returncode)
except:
    sys.exit("build error")