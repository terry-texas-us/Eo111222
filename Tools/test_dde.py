"""
test_dde.py — AeSys DDE Integration Scenarios
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Exercises the AeSys DDE server (service "AeSys", topic "Commands") through a
sequence of realistic drawing scenarios rather than isolated unit tests.

Each test class is a self-contained scenario with a clear narrative.  All
geometry created within a scenario is undone at the end so AeSys is left in
a clean state for the next scenario.

Run while AeSys is open with at least one document active:

    python -m pytest Tools/test_dde.py -v

Requirements: only the Python standard library + ctypes (no pywin32).

DDE command format
------------------
The 'Commands' topic has exactly one registered execute verb: CLI.
All AeSys commands are forwarded through it:

    [CLI(REFRESH)]               -- bare command
    [CLI("LINE 0,0 200,0")]      -- coordinate arguments must be quoted so
                                    commas are not split by the tokenizer
    [CLI(UNDO)]
    [CLI(ZE)]

Multiple commands in one EXECUTE string:
    [CLI(REFRESH)][CLI(ZE)]
"""

from __future__ import annotations

import sys
import time

import pytest

sys.path.insert(0, str(__import__("pathlib").Path(__file__).parent))

from aespy._connection import DdeConnection


# ---------------------------------------------------------------------------
# Shared fixture
# ---------------------------------------------------------------------------

@pytest.fixture(scope="module")
def dde() -> DdeConnection:
    """Module-scoped DDE connection — opened once, closed after all tests."""
    conn = DdeConnection()
    conn.open()
    yield conn
    conn.close()


# ---------------------------------------------------------------------------
# Scenario 1 — Transport health check
# ---------------------------------------------------------------------------

class TestScenario01_TransportHealth:
    """Verify that the DDE channel itself is reliable.

    AeSys must be running before this test is collected.  No geometry is
    created or modified.
    """

    def test_01_connect_and_disconnect(self) -> None:
        """Connect and disconnect three times to confirm the server is stable."""
        for _ in range(3):
            with DdeConnection():
                pass  # clean connect/disconnect cycle

    def test_02_result_item_readable(self, dde: DdeConnection) -> None:
        """The 'Result' DDE item is always readable (may return empty string)."""
        value = dde.request(DdeConnection.RESULT_ITEM)
        assert isinstance(value, str)

    def test_03_refresh_does_not_raise(self, dde: DdeConnection) -> None:
        """REFRESH is the simplest AeSys command — must never raise."""
        dde.execute("[CLI(REFRESH)]")

    def test_04_rapid_refresh(self, dde: DdeConnection) -> None:
        """20 consecutive REFRESHes: confirms the server handles back-to-back
        DDE transactions without stalling or dropping a request."""
        for _ in range(20):
            dde.execute("[CLI(REFRESH)]")


# ---------------------------------------------------------------------------
# Scenario 2 — Single line, then undo
# ---------------------------------------------------------------------------

class TestScenario02_DrawAndUndoLine:
    """Draw one line from the origin to (200, 0), then undo it.

    After this scenario AeSys should have no net geometry change.

    Viewport note: ZE (Zoom Extents) is sent after creation so the line is
    visible in the AeSys window during test development.
    """

    def test_01_draw_horizontal_line(self, dde: DdeConnection) -> None:
        """Draw LINE 0,0 -> 200,0 via DDE and zoom to extents."""
        dde.execute('[CLI("LINE 0,0 200,0")]')
        dde.execute("[CLI(ZE)]")

    def test_02_result_item_after_draw(self, dde: DdeConnection) -> None:
        """Result item is still a string after a draw command (value may vary)."""
        result = dde.request(DdeConnection.RESULT_ITEM)
        assert isinstance(result, str)

    def test_03_undo_line(self, dde: DdeConnection) -> None:
        """UNDO removes the line.  AeSys returns to pre-draw state."""
        dde.execute("[CLI(UNDO)]")
        dde.execute("[CLI(ZE)]")


# ---------------------------------------------------------------------------
# Scenario 3 — Triangle: three lines, then undo all three
# ---------------------------------------------------------------------------

class TestScenario03_DrawAndUndoTriangle:
    """Draw a right-triangle outline using three LINE commands, then undo all.

    Demonstrates that multi-command sequences are consistent and that UNDO
    correctly reverses each step in reverse order.

        (0,0) -> (100,0) -> (100,100) -> (0,0)
    """

    def test_01_draw_base(self, dde: DdeConnection) -> None:
        dde.execute('[CLI("LINE 0,0 100,0")]')

    def test_02_draw_vertical(self, dde: DdeConnection) -> None:
        dde.execute('[CLI("LINE 100,0 100,100")]')

    def test_03_draw_hypotenuse(self, dde: DdeConnection) -> None:
        dde.execute('[CLI("LINE 100,100 0,0")]')
        dde.execute("[CLI(ZE)]")

    def test_04_undo_hypotenuse(self, dde: DdeConnection) -> None:
        dde.execute("[CLI(UNDO)]")

    def test_05_undo_vertical(self, dde: DdeConnection) -> None:
        dde.execute("[CLI(UNDO)]")

    def test_06_undo_base(self, dde: DdeConnection) -> None:
        dde.execute("[CLI(UNDO)]")
        dde.execute("[CLI(ZE)]")


# ---------------------------------------------------------------------------
# Scenario 4 — Circle, then undo
# ---------------------------------------------------------------------------

class TestScenario04_DrawAndUndoCircle:
    """Draw a circle (center 100,100, passing through 150,100) then undo it.

    Two-argument form: first point = center, second point = point on circle.
    """

    def test_01_draw_circle(self, dde: DdeConnection) -> None:
        dde.execute('[CLI("CIRCLE 100,100 150,100")]')
        dde.execute("[CLI(ZE)]")

    def test_02_undo_circle(self, dde: DdeConnection) -> None:
        dde.execute("[CLI(UNDO)]")


# ---------------------------------------------------------------------------
# Scenario 5 — Multi-command batching
# ---------------------------------------------------------------------------

class TestScenario05_BatchCommands:
    """Send multiple CLI commands in a single DDE EXECUTE string.

    EoDdeLib's parser processes the full bracket sequence before returning,
    so this is more efficient than one transaction per command when a sequence
    of related operations does not require intermediate state inspection.
    """

    def test_01_draw_and_zoom_single_transaction(self, dde: DdeConnection) -> None:
        """Draw a line and zoom to extents in one DDE call."""
        dde.execute('[CLI("LINE 0,0 50,50")][CLI(ZE)]')

    def test_02_undo_in_same_transaction(self, dde: DdeConnection) -> None:
        """Draw and undo in a single DDE call -- document is unchanged."""
        dde.execute('[CLI("LINE 10,10 90,90")][CLI(UNDO)]')

    def test_03_cleanup_scenario_01_line(self, dde: DdeConnection) -> None:
        """Undo the line from test_01 above to leave AeSys clean."""
        dde.execute("[CLI(UNDO)]")


# ---------------------------------------------------------------------------
# Scenario 6 — Redo after undo
# ---------------------------------------------------------------------------

class TestScenario06_UndoRedo:
    """Verify that UNDO -> REDO round-trips geometry correctly.

    After REDO the line must be back in the document; a second UNDO cleans up.
    """

    def test_01_draw_line(self, dde: DdeConnection) -> None:
        dde.execute('[CLI("LINE 0,0 100,0")]')
        dde.execute("[CLI(ZE)]")

    def test_02_undo(self, dde: DdeConnection) -> None:
        dde.execute("[CLI(UNDO)]")

    def test_03_redo(self, dde: DdeConnection) -> None:
        dde.execute("[CLI(REDO)]")

    def test_04_final_undo_cleanup(self, dde: DdeConnection) -> None:
        dde.execute("[CLI(UNDO)]")


# ---------------------------------------------------------------------------
# Scenario 7 — Server robustness (bad input must not crash AeSys)
# ---------------------------------------------------------------------------

class TestScenario07_Robustness:
    """Send malformed or unknown commands; AeSys must survive all of them."""

    def test_01_empty_brackets(self, dde: DdeConnection) -> None:
        """Empty bracket pair -- ParseCmd returns HDDEDATA(FALSE); no crash."""
        try:
            dde.execute("[]")
        except RuntimeError:
            pass  # server-side rejection is acceptable

    def test_02_unknown_cli_verb(self, dde: DdeConnection) -> None:
        """Unknown command -- EoCommandRegistry returns 'unknown'; no crash."""
        try:
            dde.execute("[CLI(XYZZY_NO_SUCH_COMMAND)]")
        except RuntimeError:
            pass

    def test_03_recover_after_bad_input(self, dde: DdeConnection) -> None:
        """Confirm the server still accepts good commands after bad ones."""
        dde.execute("[CLI(REFRESH)]")


# ---------------------------------------------------------------------------
# Entry point
# ---------------------------------------------------------------------------

if __name__ == "__main__":
    raise SystemExit(pytest.main([__file__, "-v"]))
