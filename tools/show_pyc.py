import argparse
import binascii
import dis
import marshal
import struct
import sys
import time
import types
import warnings


def _u32le(data: bytes) -> int:
    return struct.unpack("<I", data)[0]


def _hex(data: bytes) -> str:
    return binascii.hexlify(data).decode("ascii")


def _show_hex(label: str, data: bytes, *, indent: str = "") -> None:
    h = _hex(data)
    if len(h) <= 60:
        print(f"{indent}{label} {h}")
        return
    print(f"{indent}{label}")
    for i in range(0, len(h), 60):
        print(f"{indent}   {h[i:i + 60]}")


def _format_iter(it, *, limit: int = 200) -> str:
    items = []
    count = 0
    for item in it:
        items.append(item)
        count += 1
        if count >= limit:
            items.append("... truncated ...")
            break
    return repr(items)


def _parse_exception_table(code: types.CodeType):
    table = getattr(code, "co_exceptiontable", None)
    if not isinstance(table, (bytes, bytearray, memoryview)):
        return []

    def parse_varint(iterator):
        b = next(iterator)
        val = b & 63
        while b & 64:
            val <<= 6
            b = next(iterator)
            val |= b & 63
        return val

    entries = []
    iterator = iter(table)
    try:
        while True:
            start = parse_varint(iterator) * 2
            length = parse_varint(iterator) * 2
            end = start + length
            target = parse_varint(iterator) * 2
            dl = parse_varint(iterator)
            depth = dl >> 1
            lasti = bool(dl & 1)
            entries.append((start, end, target, depth, lasti))
    except StopIteration:
        return entries


def _show_code(code: types.CodeType, *, indent: str = "") -> None:
    print(f"{indent}code")
    indent2 = indent + "   "

    def p(name: str) -> None:
        if hasattr(code, name):
            print(f"{indent2}{name[3:]} {getattr(code, name)!r}")

    def pi(name: str, *, fmt: str | None = None) -> None:
        if not hasattr(code, name):
            return
        value = getattr(code, name)
        if not isinstance(value, int):
            print(f"{indent2}{name[3:]} {value!r}")
            return
        if fmt is None:
            print(f"{indent2}{name[3:]} {value}")
        else:
            print(f"{indent2}{name[3:]} {format(value, fmt)}")

    pi("co_argcount")
    pi("co_posonlyargcount")
    pi("co_kwonlyargcount")
    pi("co_nlocals")
    pi("co_stacksize")
    pi("co_flags", fmt="04x")

    _show_hex("code", code.co_code, indent=indent2)
    try:
        dis.disassemble(code)
    except Exception:
        dis.dis(code)

    print(f"{indent2}consts")
    for const in code.co_consts:
        if isinstance(const, types.CodeType):
            _show_code(const, indent=indent2 + "   ")
        else:
            print(f"{indent2}   {const!r}")

    p("co_names")
    p("co_varnames")
    if hasattr(code, "co_localsplusnames"):
        print(f"{indent2}localsplusnames {code.co_localsplusnames!r}")
    if hasattr(code, "co_localspluskinds"):
        _show_hex("localspluskinds", code.co_localspluskinds, indent=indent2)

    p("co_freevars")
    p("co_cellvars")
    p("co_filename")
    p("co_name")
    p("co_qualname")
    pi("co_firstlineno")

    if hasattr(code, "co_linetable"):
        _show_hex("linetable", code.co_linetable, indent=indent2)
    if hasattr(code, "co_lines"):
        print(f"{indent2}lines {_format_iter(code.co_lines())}")
    if hasattr(code, "co_positions"):
        print(f"{indent2}positions {_format_iter(code.co_positions())}")

    if hasattr(code, "co_exceptiontable"):
        _show_hex("exceptiontable", code.co_exceptiontable, indent=indent2)
        entries = _parse_exception_table(code)
        if entries:
            print(f"{indent2}exception_entries {entries!r}")

    if hasattr(code, "co_lnotab"):
        with warnings.catch_warnings():
            warnings.simplefilter("ignore", DeprecationWarning)
            lnotab = code.co_lnotab
        if isinstance(lnotab, (bytes, bytearray, memoryview)):
            _show_hex("lnotab(deprecated)", bytes(lnotab), indent=indent2)
        else:
            print(f"{indent2}lnotab(deprecated) {lnotab!r}")


def _read_pyc_header(f):
    header = f.read(16)
    if len(header) < 16:
        raise EOFError("truncated pyc header")

    magic = header[:4]
    flags = _u32le(header[4:8])

    if flags & ~0b11:
        raise ValueError(f"invalid flags: {flags!r}")

    hash_based = bool(flags & 0b1)
    checked = bool(flags & 0b10)

    info = {
        "magic": magic,
        "flags": flags,
        "hash_based": hash_based,
        "checked": checked,
    }

    if hash_based:
        info["source_hash"] = header[8:16]
    else:
        info["mtime"] = _u32le(header[8:12])
        info["source_size"] = _u32le(header[12:16])

    return info


def show_file(path: str) -> None:
    with open(path, "rb") as f:
        header = _read_pyc_header(f)
        body = f.read()

    print(f"python {sys.version}")
    print(f"magic 0x{_hex(header['magic'])}")
    print(f"flags {header['flags']} (0b{header['flags']:02b})")

    if header["hash_based"]:
        print("pyc_type hash-based")
        print(f"checked {header['checked']}")
        print(f"source_hash 0x{_hex(header['source_hash'])}")
    else:
        mtime = header["mtime"]
        modtime = time.asctime(time.localtime(mtime))
        print("pyc_type timestamp-based")
        print(f"mtime {mtime} ({modtime})")
        print(f"source_size {header['source_size']}")

    obj = marshal.loads(body)
    if not isinstance(obj, types.CodeType):
        raise TypeError(f"expected code object, got {type(obj)!r}")
    _show_code(obj)


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("pyc", help="path to .pyc file")
    ns = parser.parse_args(argv)
    show_file(ns.pyc)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
