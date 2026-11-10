"""Izriše en generiran graf (RandomSizeSCC ali StarSCC) na nivoju vozlišč.

Graf najprej izvozi C koda z Export_SCCGraph (main.cpp), npr.
    Export_SCCGraph(32, 4, 0.1, 0.5, 0.5, 1, "../results/Viz/SCC/graph.csv");
Komponente (SCC) poišče skripta sama, ker so vozlišča premešana. Vsaka komponenta je krog vozlišč, komponente so
od leve proti desni v topološkem vrstnem redu (C_0, ponor, je skrajno desno). Povezave znotraj komponente so sive,
povezave med komponentami so obarvane z barvo izvorne komponente. Repi (tails) imajo debel črn rob, glave (heads)
so kvadrati.

Uporaba:
    python3 results/Viz/SCC/draw_graph.py                      # graph.csv -> graph.pdf v tej mapi
    python3 results/Viz/SCC/draw_graph.py pot/do/graph.csv -o slika.pdf --labels

Potrebuje networkx in matplotlib.
"""
import argparse, csv, math, os

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import networkx as nx

HERE = os.path.dirname(os.path.abspath(__file__))


def load(path):
    G = nx.DiGraph()
    with open(path) as f:
        for row in csv.DictReader(f):
            G.add_edge(int(row["from"]), int(row["to"]), weight=float(row["weight"]))
    return G


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("csv", nargs="?", default=os.path.join(HERE, "graph.csv"))
    ap.add_argument("-o", "--out", default=None)
    ap.add_argument("--labels", action="store_true", help="izpiši številke vozlišč")
    args = ap.parse_args()
    out = args.out or os.path.splitext(args.csv)[0] + ".pdf"

    G = load(args.csv)
    C = nx.condensation(G)                       # DAG komponent, C.graph["mapping"]: vozlišče -> komponenta
    comp_of = C.graph["mapping"]
    order = list(reversed(list(nx.topological_sort(C))))   # ponor (C_0) prvi, kot v SCC.cpp
    index = {c: i for i, c in enumerate(order)}
    k = len(order)

    # Postavitev: komponenta i je krog s središčem na x = -i (C_0 desno), polmer raste s številom vozlišč
    sizes = [len(C.nodes[c]["members"]) for c in order]
    radius = [0.4 + 0.12 * math.sqrt(s) for s in sizes]
    pos, x = {}, 0.0
    centers = []
    for i in range(k):
        if i > 0:
            x -= radius[i - 1] + radius[i] + 1.2
        centers.append(x)
        members = sorted(C.nodes[order[i]]["members"])
        for j, v in enumerate(members):
            a = 2 * math.pi * j / len(members)
            pos[v] = (x + radius[i] * math.cos(a), (0.6 if i % 2 else -0.6) * (k > 2) + radius[i] * math.sin(a))

    cmap = plt.get_cmap("tab10" if k <= 10 else "tab20")
    color = {v: cmap(index[comp_of[v]] % cmap.N) for v in G}
    inner = [(u, v) for u, v in G.edges if comp_of[u] == comp_of[v]]
    cross = [(u, v) for u, v in G.edges if comp_of[u] != comp_of[v]]
    tails = {u for u, _ in cross}
    heads = {v for _, v in cross}

    fig, ax = plt.subplots(figsize=(max(6, 2.2 * k + 2), 5))
    nx.draw_networkx_edges(G, pos, edgelist=inner, ax=ax, edge_color="#bbbbbb", width=0.5, arrowsize=6,
                           connectionstyle="arc3,rad=0.1")
    nx.draw_networkx_edges(G, pos, edgelist=cross, ax=ax, edge_color=[color[u] for u, _ in cross], width=0.9,
                           arrowsize=8, alpha=0.8, connectionstyle="arc3,rad=0.15")
    for shape, nodes in (("o", [v for v in G if v not in heads]), ("s", [v for v in G if v in heads])):
        nx.draw_networkx_nodes(G, pos, nodelist=nodes, node_shape=shape, ax=ax, node_size=90,
                               node_color=[color[v] for v in nodes],
                               edgecolors=["black" if v in tails else "white" for v in nodes],
                               linewidths=[1.8 if v in tails else 0.5 for v in nodes])
    if args.labels:
        nx.draw_networkx_labels(G, pos, ax=ax, font_size=6)
    for i, cx in enumerate(centers):
        ax.text(cx, -max(radius) - 1.0, f"C$_{{{i}}}$  |C|={sizes[i]}", ha="center", fontsize=9)

    ax.set_title(f"n = {G.number_of_nodes()}, m = {G.number_of_edges()}, k = {k}, "
                 f"povezav med komponentami: {len(cross)}  (repi: rob, glave: kvadrat)", fontsize=10)
    ax.set_aspect("equal")
    ax.axis("off")
    fig.tight_layout()
    fig.savefig(out)
    print("zapisano:", out)


if __name__ == "__main__":
    main()
