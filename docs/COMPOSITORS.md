# Compositor support

hyprwat runs on Hyprland and [fenriz](https://github.com/zackb/fenriz). It picks one at startup in
`compositor::detect()` (`src/compositor/detect.cpp`) by environment variable:
`FENRIZ_SOCKET` wins, then `HYPRLAND_INSTANCE_SIGNATURE`. Neither present is a clean error.

fenriz is checked first because a fenriz session nested inside Hyprland inherits
`HYPRLAND_INSTANCE_SIGNATURE` from its parent, so it is the less ambiguous signal.

Everything a compositor must provide is the `compositor::Compositor` interface
(`src/compositor/compositor.hpp`). Implementations:

| | Hyprland | fenriz |
|---|---|---|
| Source | `src/hyprland/ipc.cpp` | `src/compositor/fenriz.cpp` |
| Transport | request/reply over `.socket.sock` | one NDJSON snapshot read on connect |

## Mode support

Most modes need the compositor for exactly one thing: **where the cursor is, and which monitor
that is on.** Everything else is plain Wayland layer-shell.

| Mode | Hyprland | fenriz |
|---|---|---|
| menu, `--input`, `--password`, `--wifi`, `--audio`, `--volume-*`, `--custom` | yes | yes |
| `--wallpaper` | yes (drives hyprpaper) | yes, but does not set the wallpaper — see below |
| `--overview` | yes | **no** — see gaps |

### `--wallpaper` on fenriz

`WallpaperFlow` prints the selected path to stdout regardless of what the compositor does, and
that is what a wallpaper script consumes. `Fenriz::setWallpaper()` is therefore a deliberate
no-op: the picker works, the path is printed, and the caller decides what to do with it.

```sh
hyprwat --wallpaper ~/Pictures/walls | xargs -r swaybg -i
```

On Hyprland the same flow additionally tells hyprpaper to load the image, which is
Hyprland-specific and has no fenriz equivalent (fenriz wallpapers are layer-shell clients).

## Gaps in fenriz

### Implemented — `cursor` in the IPC snapshot

**Required by every mode; without it nothing works.** fenriz had no way to report pointer
position. Added to `snapshot()` in `fenriz/src/ipc.cpp` as
`"cursor":{"x":N,"y":N}` in layout coordinates, matching `outputs[].x/y`.

Pointer motion is deliberately **not** a publish trigger — it would flood the feed. The
snapshot is built fresh on connect, which is all a one-shot client needs. See
`fenriz/docs/IPC.md`. `Fenriz::cursorPos()` throws a clear error against a fenriz too old to
have this field rather than silently placing the menu at 0,0.

### Missing — window list (blocks `--overview`)

The snapshot carries only `activeWindow{appId,title}`. Overview needs every window: a stable
id, app id, title, workspace, `x/y/width/height`, and mapped state. fenriz already walks
`server.views` to compute `workspaces.occupied`, so the data is in hand — this is mostly a
matter of serializing it.

### Missing — per-window pixel capture (blocks `--overview`)

The real blocker. Overview renders a live thumbnail per window. On Hyprland this uses
`hyprland-toplevel-export-v1`, which captures a **single toplevel** to a dmabuf keyed by window
address (`src/frames/overview.cpp`).

fenriz has `wlr-screencopy-v1` (whole output only) and `wlr-foreign-toplevel-management-v1`
(metadata, no pixels). Neither can capture one window.

The right fix is **`ext-image-copy-capture-v1` + `ext-foreign-toplevel-list-v1`** — the
standardized successors, both provided by wlroots 0.20 and both already listed under "Should
implement" in `fenriz/docs/PROTOCOLS.md`. That also needs a second capture backend in hyprwat,
since the protocol shape differs from Hyprland's.

Do **not** hand-write Hyprland's private protocol into fenriz to avoid that work.
