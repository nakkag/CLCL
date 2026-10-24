#!/usr/bin/env python3
"""Convert a CLCL binary data dump (format written by File.c) to JSON."""

import argparse
import base64
import json
import os
import sys
from datetime import datetime, timedelta

MARK_FOLDER = 0x04
MARK_FOLDER_END = 0x05
MARK_ITEM = 0x01
MARK_ITEM_HEADER_END = 0x02
MARK_DATA_HEADER_END = 0x03

FILETIME_EPOCH = datetime(1601, 1, 1)


class DumpError(Exception):
    pass


class Reader:
    def __init__(self, buf, encoding):
        self.buf = buf
        self.pos = 0
        self.encoding = encoding

    def at_end(self):
        return self.pos >= len(self.buf)

    def peek(self):
        return None if self.at_end() else self.buf[self.pos]

    def expect(self, marker):
        if self.peek() != marker:
            got = "EOF" if self.at_end() else f"0x{self.peek():02x}"
            raise DumpError(f"expected 0x{marker:02x} at offset {self.pos}, got {got}")
        self.pos += 1

    def cstring(self):
        end = self.buf.find(b"\0", self.pos)
        if end < 0:
            raise DumpError(f"unterminated string at offset {self.pos}")
        raw = self.buf[self.pos:end]
        self.pos = end + 1
        return raw

    def text(self):
        return self.cstring().decode(self.encoding, errors="replace")

    def raw(self, size):
        if self.pos + size > len(self.buf):
            raise DumpError(f"data of {size} bytes exceeds file at offset {self.pos}")
        data = self.buf[self.pos:self.pos + size]
        self.pos += size
        return data


def parse_int(value):
    value = value.strip()
    return int(value) if value else 0


def parse_options(value):
    parts = (value.split(",") + ["", "", ""])[:3]
    return [parse_int(part) for part in parts]


def filetime_to_iso(raw):
    # Stored as 8 hex digits high DWORD followed by 8 hex digits low DWORD.
    # The value comes from SystemTimeToFileTime on local time, so it is naive local time.
    try:
        ticks = (int(raw[:8], 16) << 32) | int(raw[8:16], 16)
        if ticks == 0:
            return None
        return (FILETIME_EPOCH + timedelta(microseconds=ticks // 10)).isoformat(timespec="seconds")
    except (ValueError, OverflowError):
        return None


def decode_text(format_name, payload, encoding):
    if format_name == "UNICODE TEXT":
        return payload.decode("utf-16-le", errors="replace").rstrip("\0")
    if format_name in ("TEXT", "OEM TEXT"):
        return payload.split(b"\0", 1)[0].decode(encoding, errors="replace")
    return None


def parse_data(r):
    entries = []
    # Data headers start with the ASCII size digits; any other byte ends the list.
    while (marker := r.peek()) is not None and 0x30 <= marker <= 0x39:
        size = parse_int(r.text())
        format_name = r.text()
        plugin_string = r.text()
        plugin_param = parse_int(r.text())
        op_modifiers, op_virtkey, op_paste = parse_options(r.text())
        r.expect(MARK_DATA_HEADER_END)
        payload = r.raw(size)

        entry = {
            "format_name": format_name,
            "size": size,
            "plugin_string": plugin_string,
            "plugin_param": plugin_param,
            "op_modifiers": op_modifiers,
            "op_virtkey": op_virtkey,
            "op_paste": op_paste,
            "data_base64": base64.b64encode(payload).decode("ascii"),
        }
        text = decode_text(format_name, payload, r.encoding)
        if text is not None:
            entry["text"] = text
        entries.append(entry)
    return entries


def parse_item(r):
    title = r.text()
    modified_raw = r.text()
    window_name = r.text()
    plugin_string = r.text()
    plugin_param = parse_int(r.text())
    op_modifiers, op_virtkey, op_paste = parse_options(r.text())
    r.expect(MARK_ITEM_HEADER_END)
    return {
        "type": "item",
        "title": title,
        "modified": filetime_to_iso(modified_raw),
        "window_name": window_name,
        "plugin_string": plugin_string,
        "plugin_param": plugin_param,
        "op_modifiers": op_modifiers,
        "op_virtkey": op_virtkey,
        "op_paste": op_paste,
        "data": parse_data(r),
    }


def parse_items(r, level):
    items = []
    while not r.at_end():
        marker = r.peek()
        if marker == MARK_FOLDER_END:
            if level == 0:
                raise DumpError(f"unexpected folder end at offset {r.pos}")
            r.pos += 1
            return items
        if marker == MARK_FOLDER:
            r.pos += 1
            title = r.text()
            items.append({"type": "folder", "title": title, "children": parse_items(r, level + 1)})
        elif marker == MARK_ITEM:
            r.pos += 1
            items.append(parse_item(r))
        else:
            raise DumpError(f"unexpected byte 0x{marker:02x} at offset {r.pos}")
    if level > 0:
        raise DumpError("unterminated folder at end of file")
    return items


def convert(path, encoding):
    with open(path, "rb") as f:
        buf = f.read()
    items = parse_items(Reader(buf, encoding), 0)
    return {"source": os.path.basename(path), "encoding": encoding, "items": items}


def main():
    parser = argparse.ArgumentParser(description="Convert a CLCL binary data dump to JSON.")
    parser.add_argument("input", help="path to the exported CLCL binary dump")
    parser.add_argument("-o", "--output", help="output JSON file (default: stdout)")
    parser.add_argument("--encoding", default="cp932",
                        help="encoding of ANSI strings in the dump (default: cp932)")
    parser.add_argument("--indent", type=int, default=2, help="JSON indent width (default: 2)")
    args = parser.parse_args()

    try:
        result = convert(args.input, args.encoding)
    except (OSError, DumpError) as e:
        print(f"error: {e}", file=sys.stderr)
        return 1

    text = json.dumps(result, ensure_ascii=False, indent=args.indent)
    if args.output:
        with open(args.output, "w", encoding="utf-8") as f:
            f.write(text + "\n")
    else:
        print(text)
    return 0


if __name__ == "__main__":
    sys.exit(main())
