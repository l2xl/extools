#!/usr/bin/env python3
# Open Trader
# Copyright (c) 2026 l2xl (l2xl/at/proton.me)
# Distributed under the Intellectual Property Reserve License, v2 (IPRL)

"""Publish a failed build/test log as a GitHub Check Run, anchored on the first error.

CI job logs are only downloadable with repo-admin auth, so a failing build is
opaque to anyone driving CI through the public REST API. The whole log travels
as a workflow artifact; this posts the part that explains the failure into a
Check Run's Markdown summary, which is readable unauthenticated.

The excerpt runs from CONTEXT_LINES before the first error line to the end of
the log. A plain tail cannot work here: one template backtrace is longer than
any fixed window, so the error scrolls out and the report says nothing. The
Checks API caps output.summary at 65535 characters, which is why an excerpt is
posted at all; when even that is over budget the middle is dropped, never the
first error or the end.
"""

import os
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "scripts"))

from publish_check_run import publish  # noqa: E402

MAX_SUMMARY_BYTES = 60000
CONTEXT_LINES = 60
BLOCK_OVERHEAD_BYTES = 300
TITLE_LIMIT = 200

# The first line matching any of these is what a reader opens the report for:
# a compiler diagnostic, a linker failure, or ctest's per-test verdict.
ERROR_RE = re.compile(
    r": (?:fatal )?error:"
    r"|^FAILED: "
    r"|undefined reference to"
    r"|^[\w./+-]*ld(?:\.lld)?: error:"
    r"|\*\*\*(?:Failed|Exception|Timeout)"
    r"|^Errors while running CTest"
)

ELISION = "...(middle dropped to fit the Checks API limit -- whole log in this run's CI log artifact)"


def read_lines(path):
    return Path(path).read_text(encoding="utf-8", errors="replace").splitlines()


def first_error(lines):
    for index, line in enumerate(lines):
        if ERROR_RE.search(line):
            return index
    return None


def excerpt(lines):
    """(text, header, error_offset): from CONTEXT_LINES before the first error to
    the end; error_offset locates that error line inside the returned text."""
    total = len(lines)
    anchor = first_error(lines)
    if anchor is None:
        return "\n".join(lines), f"{total} lines, no error line matched", 0
    start = max(0, anchor - CONTEXT_LINES)
    header = f"lines {start + 1}-{total} of {total}, first error at line {anchor + 1}"
    return "\n".join(lines[start:]), header, anchor - start


def _take(lines, room):
    """As many whole lines as fit in `room` bytes, and the bytes they cost."""
    taken, used = [], 0
    for line in lines:
        cost = len(line.encode("utf-8")) + 1
        if used + cost > room:
            break
        taken.append(line)
        used += cost
    return taken, used


def fit(text, budget, error_offset=0):
    """Shed what is furthest from the failure: the context before the first error
    goes first, then the middle. The error line and the log's last words -- what
    the build finally gave up on -- always survive."""
    if len(text.encode("utf-8")) <= budget:
        return text
    room = budget - len(ELISION.encode("utf-8")) - 2
    if room <= 0:
        return ELISION
    lines = text.splitlines()
    context, rest = lines[:error_offset], lines[error_offset:]

    head, head_used = _take(rest, room * 2 // 3)
    if not head and rest:
        head = [rest[0][:max(room // 2, 80)]]
        head_used = len(head[0].encode("utf-8")) + 1
    tail, tail_used = _take(reversed(rest[len(head):]), room - head_used)
    tail.reverse()
    lead, _ = _take(reversed(context), room - head_used - tail_used)
    lead.reverse()

    return "\n".join(lead + head + [ELISION] + tail)


def render_summary(log_paths):
    paths = [path for path in log_paths if Path(path).is_file()]
    if not paths:
        return "No build/test logs were captured."
    budget = max(MAX_SUMMARY_BYTES // len(paths) - BLOCK_OVERHEAD_BYTES, 1000)
    blocks = []
    for path in paths:
        text, header, error_offset = excerpt(read_lines(path))
        fenced = fit(text, budget, error_offset).replace("```", "` ` `")
        blocks.append(f"### `{path}` ({header})\n\n```\n{fenced}\n```")
    return "\n\n".join(blocks)


def render_title(log_paths):
    """The first error on the check run's own line, so the failure is legible
    from the checks list without opening the report."""
    for path in log_paths:
        if not Path(path).is_file():
            continue
        lines = read_lines(path)
        anchor = first_error(lines)
        if anchor is not None:
            return lines[anchor].strip()[:TITLE_LIMIT]
    return "Build or offline tests failed"


def main():
    repo = os.environ.get("GITHUB_REPOSITORY")
    sha = os.environ.get("GITHUB_SHA")
    token = os.environ.get("GITHUB_TOKEN")
    if not (repo and sha and token):
        print("missing GITHUB_REPOSITORY/GITHUB_SHA/GITHUB_TOKEN", file=sys.stderr)
        return 1

    body = {
        "status": "completed",
        "conclusion": "failure",
        "output": {"title": render_title(sys.argv[1:]), "summary": render_summary(sys.argv[1:])},
    }
    try:
        result = publish(repo, sha, token, body, name="CI Build Failure")
    except RuntimeError as exc:
        print(f"FAILED to create check run: {exc}", file=sys.stderr)
        return 1
    print(f"created check run: {result['html_url']}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
