import dis
import opcode
import sys


def main() -> int:
    path = sys.argv[1] if len(sys.argv) > 1 else "tests_py/hello.py"
    with open(path, "r", encoding="utf-8") as f:
        src = f.read()
    co = compile(src, path, "exec")
    print("--- dis (show_caches=True) ---")
    dis.dis(co, show_caches=True)
    print("--- opcode numbers ---")
    need = [
        "CACHE",
        "RESUME",
        "PUSH_NULL",
        "LOAD_NAME",
        "LOAD_GLOBAL",
        "LOAD_CONST",
        "PRECALL",
        "CALL",
        "POP_TOP",
        "RETURN_VALUE",
    ]
    for n in need:
        if n in opcode.opmap:
            print(f"{n} {opcode.opmap[n]}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

