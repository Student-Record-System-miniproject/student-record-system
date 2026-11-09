import json
from pathlib import Path

report = Path("coverage-report/functions.json")

rows = [
    json.loads(line)
    for line in report.read_text(encoding="utf-8").splitlines()
    if line.strip()
]

functions = []

for item in rows:
    if item.get("_type") != "tag" or item.get("kind") != "function":
        continue

    path = item.get("path", "").replace("\\", "/")

    # Include production source files and project headers only.
    if not (path.startswith("src/") or path.startswith("include/srm/")):
        continue

    start = item.get("line")
    end = item.get("end")

    if start is None or end is None:
        continue

    length = end - start + 1

    functions.append({
        "name": item.get("name", ""),
        "path": path,
        "start": start,
        "end": end,
        "length": length
    })

long_functions = [f for f in functions if f["length"] > 50]

print("PRODUCTION FUNCTION LENGTH AUDIT")
print("=" * 65)
print(f"Functions checked: {len(functions)}")
print(f"Functions over 50 lines: {len(long_functions)}")

for function in sorted(long_functions, key=lambda f: f["length"], reverse=True):
    print(
        f'{function["length"]:>3} lines | '
        f'{function["path"]}:{function["start"]}-{function["end"]} | '
        f'{function["name"]}'
    )

print("=" * 65)
print("RESULT:", "PASS" if not long_functions else "REVIEW REQUIRED")