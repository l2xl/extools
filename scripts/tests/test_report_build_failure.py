# Open Trader
# Copyright (c) 2026 l2xl (l2xl/at/proton.me)
# Distributed under the Intellectual Property Reserve License, v2 (IPRL)

"""The CI build report is anchored on the first error, not on a fixed tail.

A tail window is unusable on this project's logs: one boost/beast template
backtrace runs for hundreds of lines, so the compiler error scrolls out and the
published report explains nothing.
"""

import sys
from pathlib import Path

import pytest

ROOT = Path(__file__).resolve().parent.parent.parent
sys.path.insert(0, str(ROOT / "ci"))

import report_build_failure as rbf  # noqa: E402


def _log_with_buried_error(backtrace_lines=800):
    """The real shape: a compile error, then a backtrace longer than any window."""
    head = [f"[{i}/62] Building CXX object foo/bar_{i}.cpp.o" for i in range(120)]
    error = "/src/test/datahub/test_data_feed.cpp:42:17: error: no member named 'subscribe' in 'feed'"
    backtrace = [f"    note: in instantiation of template class 'boost::beast::detail::thing_{i}'" for i in range(backtrace_lines)]
    tail = ["gmake[1]: *** [CMakeFiles/unit_tests.dir/rule] Error 2", "gmake: *** [Makefile:241: unit_tests] Error 2"]
    return head + [error] + backtrace + tail, error


def test_excerpt_keeps_the_error_its_context_and_everything_after():
    lines, error = _log_with_buried_error()
    text, header, error_offset = rbf.excerpt(lines)
    body = text.splitlines()
    assert body[error_offset] == error

    assert error in body
    assert body[-1] == lines[-1]
    assert len(body) == rbf.CONTEXT_LINES + 1 + 800 + 2
    assert body[0] == lines[120 - rbf.CONTEXT_LINES]
    assert "first error at line 121" in header


def test_a_tail_window_would_have_hidden_this_error():
    """Guards the regression this replaced: the old report tailed 120 lines."""
    lines, error = _log_with_buried_error()
    assert error not in lines[-120:]
    assert error in rbf.excerpt(lines)[0]


def test_log_without_any_error_line_is_reported_whole():
    lines = [f"line {i}" for i in range(300)]
    text, header, error_offset = rbf.excerpt(lines)
    assert error_offset == 0
    assert text.splitlines() == lines
    assert "no error line matched" in header


@pytest.mark.parametrize("line", [
    "/src/a.cpp:1:1: error: bad",
    "/src/a.cpp:1:1: fatal error: no such file",
    "FAILED: CMakeFiles/x.o ",
    "ld.lld: error: undefined symbol",
    "main.cpp.o: undefined reference to `foo()'",
    "  3/22 Test #3: test/datahub/test_data_feed.cpp ***Failed    0.01 sec",
    "Errors while running CTest",
])
def test_error_shapes_recognised(line):
    assert rbf.first_error(["noise", line, "more"]) == 1


def test_over_budget_excerpt_keeps_the_error_and_the_end():
    lines, error = _log_with_buried_error(backtrace_lines=20000)
    text, _, error_offset = rbf.excerpt(lines)
    fitted = rbf.fit(text, 4000, error_offset)

    assert len(fitted.encode("utf-8")) <= 4000
    assert error in fitted
    assert lines[-1] in fitted
    assert rbf.ELISION in fitted


def test_summary_and_title_carry_the_first_error(tmp_path):
    lines, error = _log_with_buried_error()
    log = tmp_path / "build.log"
    log.write_text("\n".join(lines), encoding="utf-8")

    summary = rbf.render_summary([str(log)])
    assert error in summary
    assert len(summary.encode("utf-8")) <= rbf.MAX_SUMMARY_BYTES
    assert rbf.render_title([str(log)]) == error


def test_missing_log_is_reported_not_crashed(tmp_path):
    missing = str(tmp_path / "absent.log")
    assert rbf.render_summary([missing]) == "No build/test logs were captured."
    assert rbf.render_title([missing]) == "Build or offline tests failed"
