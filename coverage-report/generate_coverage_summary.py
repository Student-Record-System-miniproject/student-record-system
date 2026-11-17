import gzip
import json
from pathlib import Path

REPORT_DIR = Path(__file__).resolve().parent
PROJECT_DIR = REPORT_DIR.parent
SOURCE_DIR = PROJECT_DIR / "src"

coverage = {}

for report_path in sorted(REPORT_DIR.glob("*.gcov.json.gz")):
    with gzip.open(report_path, "rt", encoding="utf-8") as report:
        data = json.load(report)

    for source in data["files"]:
        source_path = Path(source["file"]).resolve()

        # Exclude GCC standard-library headers and other external files.
        try:
            module = source_path.relative_to(SOURCE_DIR).as_posix()
        except ValueError:
            continue

        lines = source.get("lines", [])
        total = len(lines)
        covered = sum(1 for line in lines if line.get("count", 0) > 0)

        if module in coverage:
            raise RuntimeError(f"Duplicate coverage report for {module}")

        coverage[module] = (covered, total)

print("\nBUSINESS-LOGIC LINE COVERAGE")
print("=" * 65)

total_covered = 0
total_lines = 0

for module, (covered, total) in sorted(coverage.items()):
    percentage = (covered / total * 100) if total else 0
    total_covered += covered
    total_lines += total

    print(
        f"{module:<28} "
        f"{covered:>4}/{total:<4} "
        f"{percentage:>7.2f}%"
    )

overall = total_covered / total_lines * 100 if total_lines else 0

print("=" * 65)
print(f"Total modules: {len(coverage)}")
print(f"Covered lines: {total_covered}/{total_lines}")
print(f"Overall coverage: {overall:.2f}%")
print(f"Required coverage: 70.00%")

if len(coverage) != 15:
    print("WARNING: Expected 15 production modules.")

print("RESULT:", "PASS" if overall >= 70 else "BELOW 70%")