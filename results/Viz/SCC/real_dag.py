"""Component DAG of the Gnutella and SNAP graphs with Graphviz (dot), in the style of run2.py.

Real graphs have thousands of SCCs (mostly single vertices), so they cannot be drawn one by one. The picture is a
bow-tie around the largest SCC:
  - the largest SCCs (--top) are drawn on their own, labelled with their size,
  - every other SCC is merged into one of three groups by its position relative to the largest SCC:
    IN (can reach it), OUT (reachable from it) and OTHER (neither).
Edge labels are the number of graph edges between two groups; the width grows with the logarithm of that number.
Above each graph is a table with its properties and the algorithm times from the results CSV.

Usage:
    python3 results/Viz/SCC/real_dag.py                # gnutella_dag.pdf and snap_dag.pdf in this directory
    python3 results/Viz/SCC/real_dag.py --top 20 --only snap

Requires Graphviz (dot), networkx, numpy, matplotlib and PyMuPDF (fitz), like run2.py.
"""
import argparse, math, os, re, subprocess, sys
from collections import Counter, defaultdict

import fitz
import networkx as nx

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, "..", "..", ".."))
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.dirname(HERE))
from run2 import draw_metadata                                     # noqa: E402
import plot_results                                                # noqa: E402

GROUP_STYLE = {
    "giant": "#D64545", "top-in": "#4C8BF5", "top-out": "#2E9E44", "top-other": "#8C8C8C",
    "IN": "#A9C6FA", "OUT": "#A8D8B2", "OTHER": "#D9D9D9",
}


# Reading (same rules as GraphTools/snap.c and GenerateGraphGnutella) -------------------------------------------------
def read_edges(path):
    """Directed edges (u, v), self loops included. Supports the formats that GraphTools/snap.c reads."""
    edges, names = [], {}
    candidate, source = None, None
    name_id = lambda s: names.setdefault(s, len(names))
    with open(path, errors="replace") as f:
        for line in f:
            line = line.rstrip("\r\n")
            if not line or line[0] == "#":
                continue
            if line.startswith("SRC:"):
                source = name_id(line[4:]) if line[4:] else None
                continue
            if line.startswith("TGT:"):
                if source is not None and line[4:]:
                    edges.append((source, name_id(line[4:])))
                source = None
                continue
            if ":" in line and line[:2].isalpha() and line[:2].isupper():
                continue  # other wiki-RfA fields
            parts = line.replace(",", " ").split()
            if parts[0] == "U" and len(parts) > 1:
                candidate = int(parts[1])
                continue
            if parts[0] == "V" and len(parts) > 2:
                if candidate is not None:
                    edges.append((int(parts[2]), candidate))  # voter -> candidate
                continue
            try:
                edges.append((int(parts[0]), int(parts[1])))
            except (ValueError, IndexError):
                pass
    return edges


# Bow-tie component DAG -----------------------------------------------------------------------------------------------
def bowtie(edges, top):
    G = nx.DiGraph()
    G.add_nodes_from(x for e in edges for x in e)  # a vertex with only a self loop still counts, as in snap.c
    G.add_edges_from((u, v) for u, v in edges if u != v)
    sccs = sorted(nx.strongly_connected_components(G), key=len, reverse=True)
    comp = {v: i for i, c in enumerate(sccs) for v in c}
    C = nx.condensation(G, scc=sccs)  # node i = sccs[i]
    giant = 0
    down = nx.descendants(C, giant)
    up = nx.ancestors(C, giant)
    where = lambda i: "in" if i in up else "out" if i in down else "other"

    shown = [i for i in range(min(top, len(sccs))) if len(sccs[i]) > 1 or i == giant]
    group = {}
    for i in range(len(sccs)):
        group[i] = f"S{i}" if i in shown else where(i).upper()

    nodes = {}
    for i in shown:
        style = "giant" if i == giant else "top-" + where(i)
        nodes[f"S{i}"] = {"label": "{:,}".format(len(sccs[i])), "size": len(sccs[i]), "style": style}
    agg = defaultdict(lambda: [0, 0])  # group -> [#SCCs, #vertices]
    for i, c in enumerate(sccs):
        if i not in shown:
            agg[group[i]][0] += 1
            agg[group[i]][1] += len(c)
    for g, (k, nv) in agg.items():
        nodes[g] = {"label": "{}\n{:,} SCCs\n{:,} vertices".format(g, k, nv), "size": nv, "style": g}

    links = Counter()
    for u, v in G.edges():
        a, b = group[comp[u]], group[comp[v]]
        if a != b:
            links[a, b] += 1

    size = lambda ids: sum(len(sccs[i]) for i in ids)
    stats = {
        "n": G.number_of_nodes(), "m": G.number_of_edges(), "#SCC": len(sccs),
        "largest SCC": len(sccs[giant]), "IN": size(up), "OUT": size(down),
        "OTHER": G.number_of_nodes() - len(sccs[giant]) - size(up) - size(down),
        "SCCs > 1 vertex": sum(1 for c in sccs if len(c) > 1),
    }
    return nodes, links, stats


def to_dot(nodes, links, name):
    smax = max(n["size"] for n in nodes.values())
    emax = max(links.values()) if links else 1
    out = [f'digraph "{name}" {{', 'rankdir=LR; splines=spline; nodesep=0.5; ranksep=1.3; '
           'node [shape=circle, fixedsize=true, style=filled, fontsize=11]; '
           'edge [color="#555555", fontsize=10, arrowsize=0.8];']
    for key, n in nodes.items():
        d = 0.6 + 1.4 * math.sqrt(n["size"] / smax)
        group = n["style"] in ("IN", "OUT", "OTHER")
        shape = ', shape=box, style="rounded,filled", fixedsize=false, margin="0.15,0.08"' if group else ""
        color = "black" if group else "white"
        out.append(f'{key} [label="{n["label"]}", width={d:.2f}, fillcolor="{GROUP_STYLE[n["style"]]}", '
                   f'fontcolor="{color}"{shape}];')
    for (a, b), c in links.items():
        w = 0.8 + 3.2 * math.log1p(c) / math.log1p(emax)
        out.append(f'{a} -> {b} [label="{c:,}", penwidth={w:.2f}];')
    # keep IN left of the largest SCC and OUT right of it
    if "IN" in nodes: out.append("{rank=min; IN;}")
    if "OUT" in nodes: out.append("{rank=max; OUT;}")
    out.append("}")
    return "\n".join(out)


# PDF -------------------------------------------------------------------------------------------------------------
def legend(page, x, y):
    items = [("largest SCC", "giant"), ("SCC in IN", "top-in"), ("SCC in OUT", "top-out"),
             ("other SCC", "top-other"), ("merged groups", "IN")]
    for label, style in items:
        r, g, b = (int(GROUP_STYLE[style][i:i + 2], 16) / 255 for i in (1, 3, 5))
        page.draw_circle((x + 4, y - 3), 4, color=None, fill=(r, g, b))
        page.insert_text((x + 11, y), label, fontsize=8, color=(.3, .3, .3))
        x += fitz.get_text_length(label, fontsize=8) + 26


def save_pdf(graphs, path, landscape):
    page_w, page_h = fitz.paper_size("a4")
    if landscape:
        page_w, page_h = page_h, page_w
    mx, top, bottom = 28, 35, 28
    pdf = fitz.open()
    for k, (title, nodes, links, row) in enumerate(graphs):
        page = pdf.new_page(width=page_w, height=page_h)
        page.insert_text((mx, 20), f"Component DAG (bow-tie around the largest SCC) - page {k + 1}",
                         fontsize=9, color=(.35, .35, .35))
        page.insert_text((mx, top + 11), title, fontsize=12, color=(.15, .15, .15))
        legend(page, mx + 150, top + 10)
        info_h = draw_metadata(page, row, list(row), mx, top + 17, page_w - 2 * mx)
        area = fitz.Rect(mx, top + 22 + info_h, page_w - mx, page_h - bottom)
        dot = subprocess.run(["dot", "-Tpdf"], input=to_dot(nodes, links, title).encode(),
                             capture_output=True, check=True).stdout
        g = fitz.open(stream=dot, filetype="pdf")
        src = g[0].rect
        s = min(area.width / src.width, area.height / src.height)
        w, h = src.width * s, src.height * s
        target = fitz.Rect(area.x0 + (area.width - w) / 2, area.y0, area.x0 + (area.width + w) / 2, area.y0 + h)
        page.show_pdf_page(target, g, 0, keep_proportion=True)
        g.close()
    pdf.save(path, garbage=4, deflate=True)
    pdf.close()


def times_by_label(load, csv_path, limit):
    """Algorithm times per graph from the results CSV (mean over runs, as in the dashboard)."""
    if not os.path.exists(csv_path):
        return {}
    ds = load(csv_path, limit)
    fmt = lambda i, a: ("> {:g} s".format(limit) if i["status"][a] == "TIMEOUT" else
                        i["status"][a] if i["status"][a] != "ok" else "{:.3f} s".format(i["times"][a]))
    return {i["label"]: {a: fmt(i, a) for a in ds["algs"]} for i in ds["instances"]}


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--gnutella", default=os.path.join(ROOT, "p2p-Gnutella"))
    ap.add_argument("--snap", default=os.path.join(ROOT, "SNAP-graphs"))
    ap.add_argument("--only", choices=["gnutella", "snap"])
    ap.add_argument("--top", type=int, default=12, help="number of largest SCCs drawn on their own (default 12)")
    ap.add_argument("--landscape", action="store_true", help="landscape A4 pages")
    ap.add_argument("--out", default=HERE, help="directory for the PDFs")
    args = ap.parse_args()

    limits = plot_results.time_limits()
    sets = {
        "gnutella": (args.gnutella, r"^\d+\.txt$", lambda f: "Gnutella{:02d}".format(int(f.split(".")[0])),
                     plot_results.load_gnutella, os.path.join(plot_results.RESULTS, "Gnutella.csv")),
        "snap": (args.snap, r"^[^.]", lambda f: re.sub(r"\.(txt|csv)$", "", f),
                 plot_results.load_snap, os.path.join(plot_results.RESULTS, "SNAP_data.csv")),
    }
    for kind, (folder, pattern, label_of, load, csv_path) in sets.items():
        if args.only and kind != args.only:
            continue
        files = sorted(f for f in os.listdir(folder)
                       if re.match(pattern, f) and os.path.isfile(os.path.join(folder, f)))
        times = times_by_label(load, csv_path, limits[kind])
        graphs = []
        for f in files:
            label = label_of(f)
            nodes, links, stats = bowtie(read_edges(os.path.join(folder, f)), args.top)
            row = {k: "{:,}".format(v) for k, v in stats.items()}
            row.update(times.get(label, {}))
            graphs.append((label, nodes, links, row))
            print("{:<28} n={:>7,}  #SCC={:>7,}  largest={:>6,}".format(label, stats["n"], stats["#SCC"],
                                                                     stats["largest SCC"]))
        path = os.path.join(args.out, kind + "_dag.pdf")
        save_pdf(graphs, path, args.landscape)
        print("written", path)


if __name__ == "__main__":
    main()
