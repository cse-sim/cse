#!/usr/bin/env python3
"""Generate the SHADE overhang/fin geometry diagrams as three separate SVGs
(front elevation, vertical section, plan section).

Geometry is illustrative (not to scale), but every position is computed
from a small set of named offsets so spacing rules are enforced once, in
the helper functions, rather than re-eyeballed per label.

Usage: gen_shade_geometry.py <output_dir>
"""
import sys
from pathlib import Path

BLUE = "#1a5fb4"
DARK = "#333333"
GRAY_FILL = "#cccccc"
WALL_FILL = "#999999"
WIN_FILL = "#dbe9f7"
LABEL_FONT = "sans-serif"
FIELD_FONT = "'Courier New', Courier, monospace"
CLEARANCE = 10  # minimum gap enforced between an offset dimension line and the shape it's offset from
LABEL_GAP = 10  # standard clear space between a dimension line/arrow and its label

# Text sizes are shared constants, not per-call numbers, precisely so every
# diagram in this file uses the same scale for the same *role* of text --
# this was violated once already (elevation drifted out of sync with
# section/plan) by passing size= ad hoc at each call site instead of always
# referencing these. The images are displayed at the content column's full
# width (see shade.md's style override, which lifts the site's default
# max-width: 50% cap on images) rather than via an inflated font-to-canvas
# ratio, so these can stay at a normal scale.
FEATURE_SIZE = 13  # plain-language labels inside/on shapes: Window, Wall, Overhang, Left Fin...
FIELD_SIZE = 13  # monospace CSE field-name dimension labels: ohExL, lfDepth...

# Physical quantities shared across views -- the SAME modeled object, just cut
# or viewed differently, so its size can't be chosen independently per view.
WINDOW_W = 200  # elevation's horizontal extent == plan's window band width
WINDOW_H = 260  # elevation's vertical extent == section's window band height
FIN_FACE_W = 18  # elevation's fin width == plan's fin width (both face-on); wide enough to hold a rotated label at FEATURE_SIZE
WALL_THICKNESS = 34  # section's wall_w == plan's wall band height (both are wall thickness); same rotated-label constraint
OVERHANG_THICKNESS = 18  # elevation's oh_thick == section's oh_b_thick (both the same slab)
LF_DIST = 34  # lfDistL: elevation's gap == plan's gap, between the left fin and the window
RF_DIST = 42  # rfDistR: elevation's gap == plan's gap, between the right fin and the window


class Diagram:
    """Every draw call grows a tracked content extent. The canvas is derived
    from that extent at render time (padded by `margin`), rather than guessed
    up front -- so the drawing is centered and tightly fit by construction,
    not by hand-picked width/height numbers that drift out of sync as the
    geometry changes."""

    def __init__(self, margin=24):
        self.margin = margin
        self.parts = []
        self.shape_boxes = []  # (x0,y0,x1,y1) of every drawn shape, for collision checks
        self.extent = [float("inf"), float("inf"), float("-inf"), float("-inf")]

    def _grow(self, x0, y0, x1, y1):
        self.extent[0] = min(self.extent[0], x0)
        self.extent[1] = min(self.extent[1], y0)
        self.extent[2] = max(self.extent[2], x1)
        self.extent[3] = max(self.extent[3], y1)

    def rect(self, x, y, w, h, fill, stroke=DARK, sw=2, track=True):
        stroke_attrs = f' stroke="{stroke}" stroke-width="{sw}"' if stroke else ""
        self.parts.append(f'<rect x="{x}" y="{y}" width="{w}" height="{h}" fill="{fill}"{stroke_attrs}/>')
        self._grow(x, y, x + w, y + h)
        if track:
            self.shape_boxes.append((x, y, x + w, y + h))

    def wall_band(self, x, y, w, h, fill, open_ends="horizontal", sw=2):
        """A wall segment whose *ends* are left open (no border) because the
        wall really continues past the edge of the illustration there -- that
        boundary isn't physically real, just where the drawing stops. Its
        actual thickness (the perpendicular pair of sides) is a real edge and
        keeps a border. 'horizontal' opens the left/right ends (a plan-view
        wall running left-right); 'vertical' opens the top/bottom ends (a
        section-view wall running up-down)."""
        self.parts.append(f'<rect x="{x}" y="{y}" width="{w}" height="{h}" fill="{fill}"/>')
        self._grow(x, y, x + w, y + h)
        self.shape_boxes.append((x, y, x + w, y + h))
        if open_ends == "horizontal":
            self.line(x, y, x + w, y, stroke=DARK, sw=sw, arrows=False, marker="")
            self.line(x, y + h, x + w, y + h, stroke=DARK, sw=sw, arrows=False, marker="")
        else:
            self.line(x, y, x, y + h, stroke=DARK, sw=sw, arrows=False, marker="")
            self.line(x + w, y, x + w, y + h, stroke=DARK, sw=sw, arrows=False, marker="")

    def line(self, x1, y1, x2, y2, stroke=BLUE, sw=1.5, arrows=True, marker="dim-arrow"):
        if not marker:
            m = ""
        elif arrows:
            m = f' marker-start="url(#{marker})" marker-end="url(#{marker})"'
        else:
            m = f' marker-end="url(#{marker})"'
        self.parts.append(f'<line x1="{x1}" y1="{y1}" x2="{x2}" y2="{y2}" stroke="{stroke}" stroke-width="{sw}"{m}/>')
        self._grow(min(x1, x2), min(y1, y2), max(x1, x2), max(y1, y2))

    @staticmethod
    def _text_bbox(x, y, s, size, anchor, rotate=0):
        if rotate:
            est_h = len(s) * size * 0.60
            est_w = size * 1.15
            y0 = y - est_h / 2 if anchor == "middle" else (y - est_h if anchor == "end" else y)
            return (x - est_w / 2, y0, x + est_w / 2, y0 + est_h)
        est_w = len(s) * size * 0.60
        if anchor == "middle":
            x0 = x - est_w / 2
        elif anchor == "end":
            x0 = x - est_w
        else:
            x0 = x
        return (x0, y - size, x0 + est_w, y + size * 0.25)

    def _overlaps_shape(self, box):
        bx0, by0, bx1, by1 = box
        for (sx0, sy0, sx1, sy1) in self.shape_boxes:
            if bx0 < sx1 and bx1 > sx0 and by0 < sy1 and by1 > sy0:
                return True
        return False

    def text(self, x, y, s, font=LABEL_FONT, size=12, fill=DARK, anchor="start", weight=None,
              check=False, tag="", rotate=0, allow_shape_overlap=False):
        if check and not allow_shape_overlap:
            box = self._text_bbox(x, y, s, size, anchor, rotate)
            if self._overlaps_shape(box):
                print(f"WARNING [{tag}]: label '{s}' at ({x},{y}) overlaps a shape", file=sys.stderr)
        w = f' font-weight="{weight}"' if weight else ""
        # SVG rotates around the text's own (x,y) anchor, which sits ON THE BASELINE --
        # a glyph's ascent (~0.7em above baseline) and descent (~0.2em below) are not
        # symmetric, so a naive rotation leaves the visual glyph off-center from (x,y).
        # Nudge the draw point opposite the ascender direction to compensate.
        draw_x = x
        if rotate == -90:
            draw_x = x + size * 0.30
        elif rotate == 90:
            draw_x = x - size * 0.30
        t = f' transform="rotate({rotate} {draw_x} {y})"' if rotate else ""
        self.parts.append(
            f'<text x="{draw_x}" y="{y}" font-family="{font}" font-size="{size}" fill="{fill}" '
            f'text-anchor="{anchor}"{w}{t}>{s}</text>'
        )
        self._grow(*self._text_bbox(draw_x, y, s, size, anchor, rotate))

    def field_label(self, x, y, s, size=FIELD_SIZE, anchor="middle", tag="", rotate=0, allow_shape_overlap=False):
        self.text(x, y, s, font=FIELD_FONT, size=size, fill=BLUE, anchor=anchor, check=True,
                   tag=tag, rotate=rotate, allow_shape_overlap=allow_shape_overlap)

    def ext_line(self, x1, y1, x2, y2):
        """Thin, light 'extension'/'witness' line connecting an actual feature point
        to an offset dimension line -- so an offset dimension is never left floating
        with no visible connection to what it's measuring."""
        self.parts.append(f'<line x1="{x1}" y1="{y1}" x2="{x2}" y2="{y2}" stroke="#aaaaaa" stroke-width="0.75"/>')
        self._grow(min(x1, x2), min(y1, y2), max(x1, x2), max(y1, y2))

    def h_dim(self, x1, x2, y, label, side="above", rotate_label=False, label_size=FIELD_SIZE, skip_label=False,
              marker="dim-arrow", stroke=BLUE):
        """Horizontal dimension line with its label at the standard LABEL_GAP,
        either 'above' or 'below' the line."""
        self.line(x1, y, x2, y, stroke=stroke, marker=marker)
        if skip_label:
            return
        xm = (x1 + x2) / 2
        label_y = y - LABEL_GAP if side == "above" else y + LABEL_GAP
        self.field_label(xm, label_y, label, size=label_size, tag=label, rotate=(-90 if rotate_label else 0))

    def v_dim(self, x, y1, y2, label, side="right", size=FIELD_SIZE):
        """Vertical dimension line with its label at the standard LABEL_GAP,
        either to the 'left' or 'right' of the line."""
        self.line(x, y1, x, y2)
        ym = (y1 + y2) / 2
        dx = LABEL_GAP if side == "right" else -LABEL_GAP
        anchor = "start" if side == "right" else "end"
        self.field_label(x + dx, ym + 4, label, size=size, anchor=anchor, tag=label)

    def detached_rotated_label(self, x, ref_y, label, size=FIELD_SIZE, side="below"):
        """A rotated field-name label not directly centered on its own dimension
        line (e.g. offset above/below to clear other geometry). Positions it so
        the standard LABEL_GAP separates the line at ref_y from the label's own
        near edge, exactly as h_dim/v_dim do for in-line labels."""
        est_half_h = len(label) * size * 0.30
        y = ref_y + LABEL_GAP + est_half_h if side == "below" else ref_y - LABEL_GAP - est_half_h
        self.field_label(x, y, label, size=size, rotate=-90, tag=label)

    def render(self, aria_label):
        minx, miny, maxx, maxy = self.extent
        m = self.margin
        vb_x, vb_y = minx - m, miny - m
        vb_w, vb_h = (maxx - minx) + 2 * m, (maxy - miny) + 2 * m
        defs = (
            '<defs>'
            '<marker id="dim-arrow" viewBox="0 0 10 10" refX="9" refY="5" '
            'markerWidth="6" markerHeight="6" orient="auto-start-reverse">'
            f'<path d="M0,1 L9,5 L0,9 Z" fill="{BLUE}"/></marker>'
            '<marker id="dim-arrow-gray" viewBox="0 0 10 10" refX="9" refY="5" '
            'markerWidth="6" markerHeight="6" orient="auto-start-reverse">'
            '<path d="M0,1 L9,5 L0,9 Z" fill="#666666"/></marker>'
            '</defs>'
        )
        body = "\n  ".join(self.parts)
        return (
            # width/height matching the viewBox 1:1 give the SVG a real intrinsic
            # size (1 viewBox unit == 1 CSS px) -- without them, an <img>-embedded
            # SVG falls back to the browser's default ~300x150 box and everything
            # inside, text included, gets scaled down to fit that, regardless of
            # how large the actual page column is.
            f'<svg viewBox="{vb_x:.1f} {vb_y:.1f} {vb_w:.1f} {vb_h:.1f}" width="{vb_w:.0f}" height="{vb_h:.0f}" '
            f'xmlns="http://www.w3.org/2000/svg" role="img"\n'
            f'     aria-label="{aria_label}">\n'
            f'  {defs}\n'
            f'  <rect x="{vb_x:.1f}" y="{vb_y:.1f}" width="{vb_w:.1f}" height="{vb_h:.1f}" fill="#ffffff"/>\n'
            f'  {body}\n'
            "</svg>\n"
        )


def build_elevation():
    d = Diagram()

    win_x, win_y, win_w, win_h = 240, 190, WINDOW_W, WINDOW_H
    oh_gap, oh_thick, oh_ex_l, oh_ex_r = 45, OVERHANG_THICKNESS, 45, 70
    fin_w = FIN_FACE_W
    # *Up values kept generous so a double-headed arrow always has visible shaft
    # between its two ~9-unit-wide arrowheads (at stroke-width 1.5, markerWidth 6).
    lf_dist, lf_top, lf_bot = LF_DIST, 32, 70
    rf_dist, rf_top, rf_bot = RF_DIST, 28, 95

    win_right, win_bottom = win_x + win_w, win_y + win_h
    oh_bottom = win_y - oh_gap
    oh_top = oh_bottom - oh_thick
    oh_left, oh_right = win_x - oh_ex_l, win_right + oh_ex_r

    lf_right = win_x - lf_dist
    lf_left = lf_right - fin_w
    lf_cx = lf_left + fin_w / 2
    lf_top_y = win_y - lf_top
    lf_bottom_y = win_bottom - lf_bot

    rf_left = win_right + rf_dist
    rf_right = rf_left + fin_w
    rf_cx = rf_left + fin_w / 2
    rf_top_y = win_y - rf_top
    rf_bottom_y = win_bottom - rf_bot

    # ---- shapes ----
    d.rect(oh_left, oh_top, oh_right - oh_left, oh_thick, GRAY_FILL)
    d.rect(win_x, win_y, win_w, win_h, WIN_FILL)
    d.rect(lf_left, lf_top_y, fin_w, lf_bottom_y - lf_top_y, GRAY_FILL)
    d.rect(rf_left, rf_top_y, fin_w, rf_bottom_y - rf_top_y, GRAY_FILL)

    # ---- feature labels, Title Case, inside their own shapes ----
    d.text(win_x + win_w / 2, win_y + win_h / 2, "Window", size=FEATURE_SIZE, anchor="middle")
    d.text(oh_left + (oh_right - oh_left) / 2, oh_top + oh_thick / 2 + 4, "Overhang", size=FEATURE_SIZE, anchor="middle")
    d.text(lf_cx, (lf_top_y + lf_bottom_y) / 2, "Left Fin", size=FEATURE_SIZE, anchor="middle",
           rotate=-90, allow_shape_overlap=True)
    d.text(rf_cx, (rf_top_y + rf_bottom_y) / 2, "Right Fin", size=FEATURE_SIZE, anchor="middle",
           rotate=-90, allow_shape_overlap=True)

    # ---- ohExL / ohExR: dimension line offset above the overhang;
    #      extension lines drop down to the actual corners being measured ----
    dim_y = oh_top - 30
    d.ext_line(oh_left, oh_top, oh_left, dim_y)
    d.ext_line(win_x, win_y, win_x, dim_y)
    d.h_dim(oh_left, win_x, dim_y, "ohExL")

    d.ext_line(win_right, win_y, win_right, dim_y)
    d.ext_line(oh_right, oh_top, oh_right, dim_y)
    d.h_dim(win_right, oh_right, dim_y, "ohExR")

    # ---- ohDistUp: arrow's own endpoints are the real edges (overhang's
    #      bottom, window's top) -- no extension lines needed ----
    d.v_dim(win_x + win_w / 2, oh_bottom, win_y, "ohDistUp")

    # ---- lfDistL / rfDistR: arrow spans the real gap directly; label is
    #      rotated (so its width, not its length, has to fit the gap) and
    #      offset -- at the standard LABEL_GAP -- clear of the arrow itself,
    #      not sitting on top of it ----
    dist_y = win_y + 90
    d.h_dim(lf_right, win_x, dist_y, "lfDistL", rotate_label=True, skip_label=True)
    d.detached_rotated_label((lf_right + win_x) / 2, dist_y, "lfDistL", side="below")
    d.h_dim(win_right, rf_left, dist_y, "rfDistR", rotate_label=True, skip_label=True)
    d.detached_rotated_label((win_right + rf_left) / 2, dist_y, "rfDistR", side="below")

    # ---- lfTopUp / rfTopUp: offset to the outside of each fin;
    #      extension lines connect back to the fin-top and window-top corners ----
    dim_x = lf_left - CLEARANCE
    d.ext_line(lf_left, lf_top_y, dim_x, lf_top_y)
    d.ext_line(win_x, win_y, dim_x, win_y)
    d.v_dim(dim_x, lf_top_y, win_y, "lfTopUp", side="left")

    dim_x = rf_right + CLEARANCE
    d.ext_line(rf_right, rf_top_y, dim_x, rf_top_y)
    d.ext_line(win_right, win_y, dim_x, win_y)
    d.v_dim(dim_x, rf_top_y, win_y, "rfTopUp", side="right")

    # ---- lfBotUp / rfBotUp: centered on each fin; a horizontal extension
    #      line carries the window-bottom reference over to the fin's centerline ----
    d.ext_line(win_x, win_bottom, lf_cx, win_bottom)
    d.v_dim(lf_cx, lf_bottom_y, win_bottom, "lfBotUp", side="left")

    d.ext_line(win_right, win_bottom, rf_cx, win_bottom)
    d.v_dim(rf_cx, rf_bottom_y, win_bottom, "rfBotUp", side="right")

    return d.render(
        "Front elevation of a window with a SHADE overhang and fins, showing ohExL, ohExR, "
        "ohDistUp, lfDistL, lfTopUp, lfBotUp, rfDistR, rfTopUp, and rfBotUp."
    )


def build_section():
    d = Diagram()

    wall_x, wall_y, wall_w = 20, 60, WALL_THICKNESS
    win_b_y, win_b_h = 140, WINDOW_H
    wall_h = (win_b_y - wall_y) + win_b_h + 30
    oh_depth, oh_flap, oh_b_thick = 100, 28, OVERHANG_THICKNESS
    fin_w = 12

    wall_right = wall_x + wall_w
    win_bottom = win_b_y + win_b_h

    # The wall is ONE continuous outline; the window is a borderless color
    # change within it (same convention as the plan view below) -- there's no
    # real seam at a window's edges, so it gets no border of its own.
    d.wall_band(wall_x, wall_y, wall_w, wall_h, WALL_FILL, open_ends="vertical")
    d.rect(wall_x, win_b_y, wall_w, win_b_h, WIN_FILL)

    # label each of the wall's own unbroken segments plus the window, centered,
    # rotated to fit the wall's narrow (thickness-wise) width
    d.text(wall_x + wall_w / 2, (wall_y + win_b_y) / 2, "Wall", size=FEATURE_SIZE, anchor="middle",
           rotate=-90, allow_shape_overlap=True)
    d.text(wall_x + wall_w / 2, (win_b_y + win_bottom) / 2, "Window", size=FEATURE_SIZE, anchor="middle",
           rotate=-90, allow_shape_overlap=True)

    oh_b_y = 80
    oh_b_bottom = oh_b_y + oh_b_thick
    d.rect(wall_right, oh_b_y, oh_depth, oh_b_thick, GRAY_FILL)
    flap_x = wall_right + oh_depth - fin_w
    flap_right = flap_x + fin_w
    flap_bottom = oh_b_bottom + oh_flap
    d.rect(flap_x, oh_b_bottom, fin_w, oh_flap, GRAY_FILL)

    # ohDepth: offset above the overhang. Only the right end needs a guideline
    # down to the overhang's real top edge -- the left end sits at wall_right,
    # where the wall's own border (kept continuous by wall_band) already runs
    # the full height, so a second line there would just retrace it.
    dim_y = oh_b_y - CLEARANCE
    d.ext_line(wall_right + oh_depth, oh_b_y, wall_right + oh_depth, dim_y)
    d.h_dim(wall_right, wall_right + oh_depth, dim_y, "ohDepth")

    # ohFlap: offset right of the flap; extension lines connect to its real edge
    dim_x = flap_right + CLEARANCE
    d.ext_line(flap_right, oh_b_bottom, dim_x, oh_b_bottom)
    d.ext_line(flap_right, flap_bottom, dim_x, flap_bottom)
    d.v_dim(dim_x, oh_b_bottom, flap_bottom, "ohFlap")

    # ohDistUp: same quantity already shown in the elevation, but also
    # directly visible here (the real gap between overhang and window).
    # Offset just past the wall's outer face into genuinely empty space --
    # the arrow's own path, not just its label, needs to clear the wall's
    # border; sitting exactly at wall_right would run it right along that
    # existing line instead of through open space. Only the bottom end needs
    # a guideline back to window's real edge -- the top end (dim_x, oh_b_bottom)
    # already sits on the overhang's own existing bottom border (which spans
    # the whole wall_right..wall_right+oh_depth width), so a second line
    # there would just retrace it, same as ohDepth above.
    dim_x = wall_right + CLEARANCE
    d.ext_line(wall_right, win_b_y, dim_x, win_b_y)
    d.v_dim(dim_x, oh_b_bottom, win_b_y, "ohDistUp")

    outward_y = win_b_y + win_b_h / 2
    d.line(wall_right, outward_y, wall_right + oh_depth, outward_y, stroke="#666", marker="dim-arrow-gray", arrows=False)
    d.text(wall_right + oh_depth / 2, outward_y + LABEL_GAP + 6, "Outward", size=FEATURE_SIZE, fill="#666", anchor="middle")

    return d.render(
        "Vertical section through a SHADE overhang, showing ohDepth (projection outward "
        "from the window) and ohFlap (a downward lip at the outer edge)."
    )


def build_plan():
    d = Diagram()

    pwall_x, pwall_y, pwall_w, pwall_h = 60, 60, 340, WALL_THICKNESS
    pwin_w = WINDOW_W
    pwin_x = pwall_x + (pwall_w - pwin_w) / 2
    pfin_w, pfin_depth = FIN_FACE_W, 80

    pwin_right = pwin_x + pwin_w

    # The wall is ONE continuous outline; the window is a borderless color
    # change within it -- there's no real seam at a window's edges.
    d.wall_band(pwall_x, pwall_y, pwall_w, pwall_h, WALL_FILL, open_ends="horizontal")
    d.rect(pwin_x, pwall_y, pwin_w, pwall_h, WIN_FILL)

    d.text((pwall_x + pwin_x) / 2, pwall_y + pwall_h / 2 + 4, "Wall", size=FEATURE_SIZE, anchor="middle")
    d.text(pwin_x + pwin_w / 2, pwall_y + pwall_h / 2 + 4, "Window", size=FEATURE_SIZE, anchor="middle")

    lfin_x = pwin_x - LF_DIST - pfin_w
    rfin_x = pwin_right + RF_DIST
    fin_bottom = pwall_y + pwall_h

    d.rect(lfin_x, fin_bottom, pfin_w, pfin_depth, GRAY_FILL)
    d.rect(rfin_x, fin_bottom, pfin_w, pfin_depth, GRAY_FILL)
    d.text(lfin_x + pfin_w / 2, fin_bottom + pfin_depth / 2, "Left Fin", size=FEATURE_SIZE, anchor="middle",
           rotate=-90, allow_shape_overlap=True)
    d.text(rfin_x + pfin_w / 2, fin_bottom + pfin_depth / 2, "Right Fin", size=FEATURE_SIZE, anchor="middle",
           rotate=-90, allow_shape_overlap=True)

    # lfDepth / rfDepth: offset outside each fin; extension lines connect
    # back to the fin's own real top/bottom edges
    dim_x = lfin_x - CLEARANCE
    d.ext_line(lfin_x, fin_bottom, dim_x, fin_bottom)
    d.ext_line(lfin_x, fin_bottom + pfin_depth, dim_x, fin_bottom + pfin_depth)
    d.v_dim(dim_x, fin_bottom, fin_bottom + pfin_depth, "lfDepth", side="left")

    dim_x = rfin_x + pfin_w + CLEARANCE
    d.ext_line(rfin_x + pfin_w, fin_bottom, dim_x, fin_bottom)
    d.ext_line(rfin_x + pfin_w, fin_bottom + pfin_depth, dim_x, fin_bottom + pfin_depth)
    d.v_dim(dim_x, fin_bottom, fin_bottom + pfin_depth, "rfDepth", side="right")

    # lfDistL / rfDistR: same quantity already shown in elevation, but also
    # directly visible here. The fin's own border reaches dist_y directly
    # (the fin runs the full depth), but the window here is a short band --
    # its border stops at its own bottom edge, well above dist_y -- so unlike
    # elevation's tall window, this end needs a guideline back to the
    # window's real corner.
    dist_y = fin_bottom + 25
    d.ext_line(pwin_x, fin_bottom, pwin_x, dist_y)
    d.h_dim(lfin_x + pfin_w, pwin_x, dist_y, "lfDistL", rotate_label=True, skip_label=True)
    d.detached_rotated_label((lfin_x + pfin_w + pwin_x) / 2, dist_y, "lfDistL", side="below")

    d.ext_line(pwin_right, fin_bottom, pwin_right, dist_y)
    d.h_dim(pwin_right, rfin_x, dist_y, "rfDistR", rotate_label=True, skip_label=True)
    d.detached_rotated_label((pwin_right + rfin_x) / 2, dist_y, "rfDistR", side="below")

    outward_x = pwin_x + pwin_w / 2
    d.line(outward_x, fin_bottom, outward_x, fin_bottom + pfin_depth, stroke="#666", marker="dim-arrow-gray", arrows=False)
    d.text(outward_x + LABEL_GAP, fin_bottom + pfin_depth / 2 + 4, "Outward", size=FEATURE_SIZE, fill="#666")

    return d.render(
        "Plan section through both SHADE fins, showing lfDepth and rfDepth (each fin's "
        "projection outward from the window)."
    )


def main():
    if len(sys.argv) != 2:
        print("usage: gen_shade_geometry.py <output_dir>", file=sys.stderr)
        sys.exit(1)
    out_dir = Path(sys.argv[1])
    out_dir.mkdir(parents=True, exist_ok=True)
    (out_dir / "shade_geometry_elevation.svg").write_text(build_elevation())
    (out_dir / "shade_geometry_section.svg").write_text(build_section())
    (out_dir / "shade_geometry_plan.svg").write_text(build_plan())


if __name__ == "__main__":
    main()
