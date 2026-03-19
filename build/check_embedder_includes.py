# Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
# Use of this source code is governed by a GNU-style license that can be
# found in the LICENSE file.

import pathlib
import re
import sys

def main() -> int:
  if len(sys.argv) < 3:
    print("usage: check_embedder_includes.py <stamp> <files...>")
    return 1
  stamp = pathlib.Path(sys.argv[1])
  pattern = re.compile(r'^\s*#\s*include\s*[<"]src/')
  errors = []
  for arg in sys.argv[2:]:
    path = pathlib.Path(arg)
    if not path.exists():
      errors.append(f"missing file: {path}")
      continue
    for index, line in enumerate(path.read_text(encoding="utf-8").splitlines(), 1):
      if pattern.search(line):
        errors.append(f"{path}:{index}: forbidden include from src/: {line.strip()}")
  if errors:
    for error in errors:
      print(error)
    return 1
  stamp.parent.mkdir(parents=True, exist_ok=True)
  stamp.write_text("ok\n", encoding="utf-8")
  return 0


if __name__ == "__main__":
  raise SystemExit(main())
