"""
aespy._queries
~~~~~~~~~~~~~~
Structured query helpers for the AeSys named-pipe QUERY protocol.

Each function opens a *fresh* pipe connection for the query, keeping
query calls independent of any open CLI connection the caller may hold.

Protocol reference (EoPipeProtocol.h):
    QUERY TRAP              →  OK {"groups":N,"primitives":M,"bbox":{...},"items":[...]}
    QUERY BLOCK AT x,y      →  OK {"name":"...","layer":"...","x":F,"y":F,"z":F,
                                    "rotation":F,"scale_x":F,"scale_y":F,"scale_z":F,
                                    "attributes":[{"tag":"...","value":"..."},...]}
    QUERY BLOCKS [layer]    →  OK {"count":N,"blocks":[{...},...]}
    QUERY ROOM x,y          →  OK {"found":true,"area":F,"perimeter":F,
                                    "vertex_count":N,"layer":"..."}
    QUERY ROOMS [layer]     →  OK {"count":N,"rooms":[{"area":F,"perimeter":F,
                                    "centroid_x":F,"centroid_y":F,
                                    "vertex_count":N,"layer":"..."},...]}
    QUERY LAYER name        →  OK {"name":"...","visible":bool,"is_work":bool,
                                    "groups":N,"primitives":M,
                                    "extents":{"min_x":F,"min_y":F,"max_x":F,"max_y":F}}
    QUERY LAYERS            →  OK {"count":N,"layers":[{"name":"...","visible":bool,
                                    "is_work":bool,"groups":N,"primitives":M},...]}
"""

from __future__ import annotations

import json
from typing import Any

from ._connection import PipeConnection, is_ok, ok_value


def query_trap(conn: PipeConnection) -> dict[str, Any]:
    """Return a dict describing the current trap contents.

    Returned keys: ``groups``, ``primitives``, ``bbox``, ``bbox_width``,
    ``bbox_height``, ``bbox_area``, ``items``.
    """
    response = conn.send("QUERY TRAP")
    if not is_ok(response):
        raise RuntimeError(f"QUERY TRAP failed: {response!r}")
    return json.loads(ok_value(response))


def query_block(conn: PipeConnection, x: float, y: float, z: float = 0.0) -> dict[str, Any]:
    """Return a dict describing the block reference at world point (*x*, *y*, *z*).

    Returned keys: ``name``, ``layer``, ``x``, ``y``, ``z``, ``rotation``,
    ``scale_x``, ``scale_y``, ``scale_z``, ``attributes``.

    Raises ``RuntimeError`` on ERROR response (e.g. no block at point).
    """
    response = conn.send(f"QUERY BLOCK AT {x},{y},{z}")
    if not is_ok(response):
        raise RuntimeError(f"QUERY BLOCK AT {x},{y},{z} failed: {response!r}")
    return json.loads(ok_value(response))


def query_blocks(conn: PipeConnection, layer: str = "") -> dict[str, Any]:
    """Return a dict with all block references in model space.

    Pass *layer* to restrict results to a single layer (case-insensitive).

    Returned keys
    -------------
    count : int
        Total number of block references found.
    blocks : list[dict]
        Each entry has the same keys as :func:`query_block`:
        ``name``, ``layer``, ``x``, ``y``, ``z``, ``rotation``,
        ``scale_x``, ``scale_y``, ``scale_z``, ``attributes``.

    Raises ``RuntimeError`` on ERROR response.
    """
    cmd = f"QUERY BLOCKS {layer.strip()}" if layer.strip() else "QUERY BLOCKS"
    response = conn.send(cmd)
    if not is_ok(response):
        raise RuntimeError(f"{cmd!r} failed: {response!r}")
    return json.loads(ok_value(response))


def query_room(conn: PipeConnection, x: float, y: float, z: float = 0.0) -> dict[str, Any]:
    """Return a dict describing the room boundary enclosing (*x*, *y*).

    Returned keys: ``found``, ``area``, ``perimeter``, ``vertex_count``, ``layer``.

    Raises ``RuntimeError`` on ERROR response (e.g. ``NOT_FOUND``).
    """
    response = conn.send(f"QUERY ROOM {x},{y},{z}")
    if not is_ok(response):
        raise RuntimeError(f"QUERY ROOM {x},{y},{z} failed: {response!r}")
    return json.loads(ok_value(response))


def query_rooms(conn: PipeConnection, layer: str = "") -> dict[str, Any]:
    """Return a dict with all closed room boundaries in model space.

    Pass *layer* to restrict results to a single layer (case-insensitive).

    Returned keys
    -------------
    count : int
        Total number of closed boundaries found.
    rooms : list[dict]
        Each entry has: ``area``, ``perimeter``, ``centroid_x``, ``centroid_y``,
        ``vertex_count``, ``layer``.

    Raises ``RuntimeError`` on ERROR response.
    """
    cmd = f"QUERY ROOMS {layer.strip()}" if layer.strip() else "QUERY ROOMS"
    response = conn.send(cmd)
    if not is_ok(response):
        raise RuntimeError(f"{cmd!r} failed: {response!r}")
    return json.loads(ok_value(response))


def query_layer(conn: PipeConnection, name: str) -> dict[str, Any]:
    """Return a dict describing the model-space layer named *name*.

    Returned keys: ``name``, ``visible``, ``is_work``, ``groups``,
    ``primitives``, ``extents``.

    Raises ``RuntimeError`` on ERROR response (e.g. ``NOT_FOUND``).
    """
    response = conn.send(f"QUERY LAYER {name}")
    if not is_ok(response):
        raise RuntimeError(f"QUERY LAYER {name!r} failed: {response!r}")
    return json.loads(ok_value(response))


def query_layers(conn: PipeConnection) -> dict[str, Any]:
    """Return a dict enumerating every model-space layer.

    Returned keys
    -------------
    count : int
        Total number of model-space layers.
    layers : list[dict]
        Each entry has: ``name``, ``visible``, ``is_work``,
        ``groups``, ``primitives``.
        (No ``extents`` — use :func:`query_layer` for per-layer detail.)

    Raises ``RuntimeError`` on ERROR response.
    """
    response = conn.send("QUERY LAYERS")
    if not is_ok(response):
        raise RuntimeError(f"QUERY LAYERS failed: {response!r}")
    return json.loads(ok_value(response))

