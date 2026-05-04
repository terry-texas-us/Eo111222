"""
test_pipe.py
~~~~~~~~~~~~
Pytest tests for the AeSys named-pipe CLI server.

Run while AeSys is open with at least one document active:

    python -m pytest Tools/test_pipe.py -v

Requirements: only the Python standard library (no pywin32, no extra packages).
pytest is used only for discovery/reporting; the tests themselves use plain
assert statements.

Pipe path: \\\\.\\pipe\\AeSys   (matches EoPipeProtocol.h kPipeName)
Protocol:  one UTF-8 line in, one UTF-8 line out, both terminated by \\n.
"""

from __future__ import annotations

import sys
import time

import pytest

# Allow running from the repo root without installing the package.
sys.path.insert(0, str(__import__("pathlib").Path(__file__).parent))

from aespy._connection import (
    PIPE_PATH,
    PipeConnection,
    error_code,
    is_error,
    is_ok,
    ok_value,
)


# ---------------------------------------------------------------------------
# Fixtures
# ---------------------------------------------------------------------------

@pytest.fixture(scope="module")
def pipe() -> PipeConnection:
    """Module-scoped pipe connection — shared across all tests in this file."""
    conn = PipeConnection(timeout=8.0)
    conn.open()
    yield conn
    conn.close()


# ---------------------------------------------------------------------------
# Helper
# ---------------------------------------------------------------------------

def cmd(pipe: PipeConnection, command: str) -> str:
    """Send *command* and return the stripped response."""
    return pipe.send(command)


# ---------------------------------------------------------------------------
# Connectivity
# ---------------------------------------------------------------------------

class TestConnectivity:
    def test_pipe_opens(self) -> None:
        """The pipe path must be available (AeSys must be running)."""
        with PipeConnection(timeout=3.0) as p:
            r = p.send("REFRESH")
        assert is_ok(r), f"Expected OK, got: {r!r}"

    def test_empty_line_ignored(self, pipe: PipeConnection) -> None:
        """An empty line should return OK without side effects."""
        r = pipe.send("")
        assert is_ok(r), f"Expected OK for empty line, got: {r!r}"


# ---------------------------------------------------------------------------
# Response format
# ---------------------------------------------------------------------------

class TestResponseFormat:
    def test_ok_has_no_extra_whitespace(self, pipe: PipeConnection) -> None:
        r = cmd(pipe, "REFRESH")
        assert r in ("OK", ) or r.startswith("OK "), f"Malformed OK: {r!r}"

    def test_error_format_unknown_command(self, pipe: PipeConnection) -> None:
        r = cmd(pipe, "XYZZY_NOSUCHCMD")
        # Currently ExecuteCommand logs to history but the wire response is OK
        # because dispatch is fire-and-forget.  This test documents the *current*
        # behaviour; update it when structured error propagation lands.
        # For now just check the response is a valid protocol line.
        assert is_ok(r) or is_error(r), f"Not a valid protocol line: {r!r}"

    def test_error_code_helper(self) -> None:
        assert error_code("ERROR:UNKNOWN_COMMAND FOOBAR") == "UNKNOWN_COMMAND"
        assert error_code("ERROR:NO_VIEW") == "NO_VIEW"
        assert error_code("ERROR:INTERNAL pipe write failed") == "INTERNAL"
        assert error_code("OK") == ""
        assert error_code("WARN something") == ""

    def test_ok_value_helper(self) -> None:
        assert ok_value("OK") == ""
        assert ok_value("OK 3.14") == "3.14"
        assert ok_value("OK hello world") == "hello world"
        assert ok_value("ERROR:NO_VIEW") == ""


# ---------------------------------------------------------------------------
# Draw commands (fire-and-forget — verify OK, not geometry)
# ---------------------------------------------------------------------------

class TestDrawCommands:
    """Verify that draw commands are accepted without errors.

    These tests do not assert that specific geometry was created; that would
    require a query interface not yet implemented.  They verify the wire
    protocol response only.
    """

    def test_refresh(self, pipe: PipeConnection) -> None:
        assert is_ok(cmd(pipe, "REFRESH"))

    def test_line_two_points(self, pipe: PipeConnection) -> None:
        assert is_ok(cmd(pipe, "LINE 0,0 100,100"))

    def test_line_relative(self, pipe: PipeConnection) -> None:
        assert is_ok(cmd(pipe, "LINE 50,50 @100,0"))

    def test_circle(self, pipe: PipeConnection) -> None:
        assert is_ok(cmd(pipe, "CIRCLE 200,200 50,200"))

    def test_polyline(self, pipe: PipeConnection) -> None:
        assert is_ok(cmd(pipe, "POLYLINE 0,0 100,0 100,100 0,100"))

    def test_undo(self, pipe: PipeConnection) -> None:
        assert is_ok(cmd(pipe, "UNDO"))

    def test_redo(self, pipe: PipeConnection) -> None:
        assert is_ok(cmd(pipe, "REDO"))


# ---------------------------------------------------------------------------
# Navigation commands
# ---------------------------------------------------------------------------

class TestNavigationCommands:
    def test_zoom_extents(self, pipe: PipeConnection) -> None:
        assert is_ok(cmd(pipe, "ZE"))

    def test_zoom_in(self, pipe: PipeConnection) -> None:
        assert is_ok(cmd(pipe, "ZOOMIN"))

    def test_zoom_out(self, pipe: PipeConnection) -> None:
        assert is_ok(cmd(pipe, "ZOOMOUT"))

    def test_refresh_alias(self, pipe: PipeConnection) -> None:
        assert is_ok(cmd(pipe, "REDRAW"))


# ---------------------------------------------------------------------------
# Sequential commands — verify ordering is preserved
# ---------------------------------------------------------------------------

class TestSequentialOrdering:
    """Send N commands and verify each one gets its own OK, in order."""

    def test_sequence_of_five(self, pipe: PipeConnection) -> None:
        commands = ["REFRESH", "LINE 0,0 10,0", "LINE 10,0 10,10",
                    "UNDO", "UNDO"]
        for c in commands:
            r = pipe.send(c)
            assert is_ok(r), f"Command {c!r} got {r!r}"

    def test_rapid_fire(self, pipe: PipeConnection) -> None:
        """50 REFRESH commands — smoke-test throughput and server stability."""
        for _ in range(50):
            r = pipe.send("REFRESH")
            assert is_ok(r)


# ---------------------------------------------------------------------------
# Reconnect
# ---------------------------------------------------------------------------

class TestReconnect:
    def test_reconnect_after_close(self, pipe: PipeConnection) -> None:
        """Close and re-open the pipe — server must accept a new connection.

        The module-scoped *pipe* fixture holds the only allowed connection
        (server nMaxInstances=1).  Close it first so the server is free to
        accept new clients, then restore it for the fixture's own teardown.
        """
        pipe.close()
        try:
            for _ in range(3):
                with PipeConnection(timeout=5.0) as p:
                    r = p.send("REFRESH")
                assert is_ok(r), f"Got {r!r} on reconnect"
                time.sleep(0.1)
        finally:
            pipe.open()  # restore so fixture teardown doesn't error


# ---------------------------------------------------------------------------
# Entry point for direct execution
# ---------------------------------------------------------------------------

if __name__ == "__main__":
    raise SystemExit(pytest.main([__file__, "-v"]))
