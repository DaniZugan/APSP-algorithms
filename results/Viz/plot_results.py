"""Figures and tables from the test results (randomSizeSCC, Gnutella, SNAP) and data for the dashboard.

Usage (from any directory):
    python3 results/Viz/plot_results.py                 # all figures + tables + dashboard/data.js
    python3 results/Viz/plot_results.py --a Adaptive-SCC --b Vertex-SCC --ref Tree   # one pair instead of the default ones
    python3 results/Viz/plot_results.py --no-figures    # only refresh dashboard/data.js

Output:
    results/Viz/figures/*.pdf   figures for the paper (random_scatter_A_vs_B.pdf for every pair in RANDOM_PAIRS)
    results/Viz/figures/*.tex   tables (booktabs) for Gnutella and SNAP
    results/Viz/dashboard/data.js  data for results/Viz/dashboard/index.html

Requires numpy and matplotlib.
"""
import argparse, csv, datetime, json, math, os, re, sys
from collections import OrderedDict, defaultdict

import numpy as np
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
from matplotlib.backends.backend_pdf import PdfPages
from matplotlib.patches import Patch
from matplotlib.ticker import FixedLocator, FuncFormatter, NullLocator

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, "..", ".."))
RESULTS = os.path.join(ROOT, "results")

PALETTE = ["#2E9E44", "#1F6FB4", "#8E5BC9", "#E07B1A", "#D64545", "#17A2B8", "#8C6D31", "#E377C2"]
GROUP_COLORS = ["#0B3C5D", "#2BB3C0", "#C9A227", "#B23A48", "#6B8F71"]  # groups (k), distinct from the algorithm colours
RATIO_TICKS = [0.1, 0.2, 0.25, 0.33, 0.5, 0.67, 0.8, 1, 1.25, 1.5, 2, 3, 4, 5, 10]

ALG_COLORS = {"Tree": "#2E9E44", "Edge-SCC": "#1F6FB4", "Vertex-SCC": "#8E5BC9", "VertexNaive-SCC": "#17A2B8",
              "Adaptive-SCC": "#E07B1A", "Vertex-SCC-reverse": "#D64545",
              "Vertex-SCC-hourglass": "#C2185B",
              "Vertex-SCC-bridgeHG": "#6D4C41",
              "Dijkstra": "#5D6D7E", "Uniform": "#B7950B", "Hidden": "#A04000", "FW": "#212F3D"}
# Pairs (A, B) compared on randomSizeSCC: A / B < 1 means A is faster
RANDOM_PAIRS = [("Adaptive-SCC", "Vertex-SCC"),     # does relaxing over edges help
                ("Vertex-SCC", "VertexNaive-SCC"),  # does the out-tree help
                ("Vertex-SCC", "Edge-SCC")]         # vertices vs. edges

# Same property names for all three tests
PROP_LABELS = {
    "N": "n", "E": "m", "density(%)": "density (%)", "outgoingEdges": "|Out|", "tails": "tails",
    "heads": "heads", "sccCount": "#SCC", "largestSCCVertices": "largest SCC (n)",
    "largestSCCEdges": "largest SCC (m)", "wccCount": "#WCC", "largestWCCVertices": "largest WCC (n)",
    "largestWCCEdges": "largest WCC (m)", "tails/heads": "tails / heads", "largestSCC/N": "largest SCC share",
    "outProb": "outProb", "inProb": "inProb", "componentProb": "componentProb", "comps": "k",
}
GNUTELLA_PROPS = {"V": "N", "E": "E", "wccV": "largestWCCVertices", "wccE": "largestWCCEdges",
                  "sccV": "largestSCCVertices", "sccE": "largestSCCEdges"}


def alg_color(name, i):
    if name in ALG_COLORS: return ALG_COLORS[name]
    n = name.lower()
    if "adapt" in n: return "#E07B1A"
    if "vertex" in n: return "#8E5BC9"
    if "edge" in n: return "#1F6FB4"
    if "tree" in n or "hourglass" in n: return "#2E9E44"
    return PALETTE[i % len(PALETTE)]


def read_define(name, default):
    try:
        text = open(os.path.join(ROOT, "run_algorithm.h")).read()
        return float(re.search(r"#define\s+" + name + r"\s+([\d.]+)", text).group(1))
    except (OSError, AttributeError):
        return default


def time_limits():
    """Time limit of each test, as set in run_algorithm.h (Gnutella and SNAP have their own)."""
    limit = read_define("MAX_INSTANCE_TIME", 60.0)
    return {"random": limit, "equal": limit, "star": limit, "snap": read_define("SNAP_INSTANCE_TIME", limit),
            "gnutella": read_define("GNUTELLA_INSTANCE_TIME", limit)}


def status_from_time(t, limit):
    """RunTest writes only the time to the CSV: the limit means TIMEOUT, -1 means CRASH."""
    if not math.isfinite(t) or t < 0: return "CRASH"
    if abs(t - limit) < 0.0015: return "TIMEOUT"
    return "ok"


def read_rows(path):
    rows = []
    with open(path, newline="") as f:
        for r in csv.reader(f):
            r = [c.strip() for c in r]
            while r and r[-1] == "": r.pop()
            if r: rows.append(r)
    return rows


def num(x):
    try: return float(x)
    except ValueError: return float("nan")


def add_derived(props):
    t, h, n = props.get("tails"), props.get("heads"), props.get("N")
    props["tails/heads"] = t / h if t is not None and h else float("nan")
    s = props.get("largestSCCVertices")
    props["largestSCC/N"] = s / n if s is not None and n else float("nan")


def k_type(k, n):
    """'log n' or 'sqrt n' if the number of components k was given as COMPS_LOG_N or COMPS_SQRT_N, else k."""
    if k == math.floor(math.log2(n)): return "log n"
    if k == math.isqrt(int(n)): return "sqrt n"
    return "{:g}".format(k)


def k_label(k, n):
    t = k_type(k, n)
    return "k = {:g}".format(k) + ("" if t == "{:g}".format(k) else " ({})".format(t))


def k_type_order(t):
    """Constants first (ascending), then log n, then sqrt n."""
    return (0, float(t), "") if t not in ("log n", "sqrt n") else (1, 0.0, t)


def make_dataset(name, algs, instances):
    return {"name": name, "algs": algs, "instances": instances}


def load_random(path, limit):
    rows = read_rows(path)
    head = rows[0]
    k = head.index("outProb")
    algs = head[:k]
    inst = []
    for r in rows[1:]:
        if len(r) < len(head): continue  # interrupted run
        times = {a: num(r[i]) for i, a in enumerate(algs)}
        props = {head[i]: num(r[i]) for i in range(k, len(head))}
        add_derived(props)
        comps = props.get("comps", props["sccCount"])
        props["kType"] = k_type(comps, props["N"])
        # componentProb = 2/k depends on k, so all of them share code 0 (the CSV keeps 3 decimals)
        if props["componentProb"] < 1 and abs(props["componentProb"] - round(2 / comps, 3)) < 1e-6:
            props["componentProb"] = 0
        label = "n={:g} k={:g} out={:g} in={:g} comp={}".format(props["N"], comps, props["outProb"], props["inProb"],
                                                              comp_label(props["componentProb"]))
        inst.append({"label": label, "props": props, "times": times,
                     "status": {a: status_from_time(t, limit) for a, t in times.items()}})
    split_k2(inst)
    return make_dataset("randomSizeSCC", algs, inst)


def split_k2(inst):
    """For k = 2, 2/k = 1.0, so both componentProb runs are written as 1.000. The test runs 2/k first, so the first
    half of the rows with the same parameters is 2/k."""
    groups = {}
    for i in inst:
        p = i["props"]
        if p.get("comps", p["sccCount"]) == 2 and p["componentProb"] == 1:
            groups.setdefault((p["N"], p["outProb"], p["inProb"], p.get("equalSizes")), []).append(i)
    for g in groups.values():
        for i in g[:len(g) // 2]:
            i["props"]["componentProb"] = 0
            i["label"] = i["label"].replace("comp=1", "comp=2/k")


def merge_runs(groups, limit):
    """TESTRUNS > 1 runs the same real graph several times: time = mean over the runs. A run that did not
    finish decides the status (CRASH before TIMEOUT before WRONG)."""
    out = []
    for g in groups:
        g["times"], g["status"] = {}, {}
        runs_by_alg = g.pop("runs")
        for a, runs in runs_by_alg.items():
            ts = [t for t, _ in runs]
            sts = [st for _, st in runs]
            st = next((x for x in ("CRASH", "TIMEOUT", "WRONG") if x in sts), "ok")
            g["status"][a] = st
            g["times"][a] = -1.0 if st == "CRASH" else limit if st == "TIMEOUT" else sum(ts) / len(ts)
        g["props"]["runs"] = max(len(r) for r in runs_by_alg.values())
        out.append(g)
    return out


def load_snap(path, limit):
    rows = read_rows(path)
    head = rows[0]
    first_status = next(i for i, h in enumerate(head) if h.endswith(" status"))
    algs = head[1:first_status]
    prop_start = first_status + len(algs)
    groups = OrderedDict()
    for r in rows[1:]:
        if len(r) < len(head): continue
        label = re.sub(r"\.(txt|csv)$", "", r[0])
        if label not in groups:
            props = {head[i]: num(r[i]) for i in range(prop_start, len(head))}
            add_derived(props)
            groups[label] = {"label": label, "props": props, "runs": {a: [] for a in algs}}
        for i, a in enumerate(algs):
            groups[label]["runs"][a].append((num(r[1 + i]), r[first_status + i]))
    return make_dataset("SNAP", algs, merge_runs(groups.values(), limit))


def load_gnutella(path, limit):
    rows = read_rows(path)
    head = rows[0]
    col = {h: i for i, h in enumerate(head)}
    algs, groups = [], OrderedDict()
    for r in rows[1:]:
        if len(r) < len(head): continue
        a = r[col["algorithm"]]
        if a not in algs: algs.append(a)
        key = (r[col["id"]], r[col["V"]], r[col["E"]])
        if key not in groups:
            props = {GNUTELLA_PROPS[h]: num(r[col[h]]) for h in GNUTELLA_PROPS}
            add_derived(props)
            groups[key] = {"label": "Gnutella{:02d}".format(int(num(r[col["id"]]))), "props": props, "runs": {}}
        t = num(r[col["time"]])
        groups[key]["runs"].setdefault(a, []).append((t, status_from_time(t, limit)))
    return make_dataset("Gnutella", algs, merge_runs(groups.values(), limit))


def ok(inst, a):
    return inst["status"].get(a) == "ok" and math.isfinite(inst["times"].get(a, float("nan")))


def geomean(xs):
    xs = [x for x in xs if x > 0 and math.isfinite(x)]
    return math.exp(sum(map(math.log, xs)) / len(xs)) if xs else float("nan")


def find_alg(algs, *needles, exclude=()):
    for a in algs:
        l = a.lower()
        if all(n in l for n in needles) and not any(e in l for e in exclude):
            return a
    return None


# Figures ---------------------------------------------------------------------------------------------------------------

def plot_times(ds, path, limit):
    inst = sorted(ds["instances"], key=lambda i: i["props"].get("N", 0))
    algs = ds["algs"]
    w = 0.8 / len(algs)
    fig, ax = plt.subplots(figsize=(max(6.0, 1.0 * len(inst) + 2), 3.8))
    x = np.arange(len(inst))
    for j, a in enumerate(algs):
        c = alg_color(a, j)
        xs = x + (j - (len(algs) - 1) / 2) * w
        for xi, i in zip(xs, inst):
            s, t = i["status"].get(a), i["times"].get(a, float("nan"))
            if s == "TIMEOUT":
                ax.bar(xi, limit, w, color="white", edgecolor=c, hatch="///", linewidth=0.8)
            elif s == "CRASH" or not math.isfinite(t) or t <= 0:
                ax.text(xi, 0, "×", color=c, ha="center", va="bottom", transform=ax.get_xaxis_transform())
            else:
                ax.bar(xi, t, w, color=c, edgecolor="#D64545" if s == "WRONG" else c,
                       linewidth=1.5 if s == "WRONG" else 0)
    ax.set_yscale("log")
    ax.set_ylabel("time (s)")
    ax.set_xticks(x, ["{}\n(n={:,})".format(i["label"], int(i["props"].get("N", 0))).replace(",", ".") for i in inst],
                  rotation=0 if len(inst) <= 6 else 30, ha="center" if len(inst) <= 6 else "right", fontsize=8)
    handles = [Patch(color=alg_color(a, j), label=a) for j, a in enumerate(algs)]
    if any(i["status"].get(a) == "TIMEOUT" for i in inst for a in algs):
        handles.append(Patch(facecolor="white", edgecolor="#555", hatch="///", label="> {:g} s (TIMEOUT)".format(limit)))
    ax.legend(handles=handles, fontsize=8, ncol=min(len(handles), 5), loc="upper left", frameon=False)
    ax.grid(axis="y", which="both", alpha=0.25)
    ax.set_title("{}: running time".format(ds["name"]))
    fig.tight_layout()
    fig.savefig(path)
    plt.close(fig)


def plot_speedup(ds, path, ref):
    inst = sorted(ds["instances"], key=lambda i: i["props"].get("N", 0))
    algs = [a for a in ds["algs"] if a != ref]
    w = 0.8 / max(1, len(algs))
    fig, ax = plt.subplots(figsize=(max(6.0, 0.9 * len(inst) + 2), 3.6))
    x = np.arange(len(inst))
    for j, a in enumerate(algs):
        xs = x + (j - (len(algs) - 1) / 2) * w
        vals = [i["times"][ref] / i["times"][a] if ok(i, a) and ok(i, ref) else np.nan for i in inst]
        # bars grow from 1 upwards (faster) or downwards (slower)
        ax.bar(xs, [v - 1 for v in vals], w, bottom=1, color=alg_color(a, ds["algs"].index(a)), label=a)
    ax.axhline(1, color="#333", linewidth=0.8)
    ax.set_yscale("log")
    speedups = [i["times"][ref] / i["times"][a] for i in inst for a in algs if ok(i, a) and ok(i, ref)]
    if speedups:
        ratio_axis(ax, min(speedups), max(speedups))
    ax.set_ylabel("speedup over {}".format(ref))
    ax.set_xticks(x, [i["label"] for i in inst], rotation=0 if len(inst) <= 6 else 30,
                  ha="center" if len(inst) <= 6 else "right", fontsize=8)
    ax.legend(fontsize=8, frameon=False, ncol=len(algs))
    ax.grid(axis="y", which="both", alpha=0.25)
    ax.set_title("{}: > 1 means faster than {}".format(ds["name"], ref))
    fig.tight_layout()
    fig.savefig(path)
    plt.close(fig)


def comp_label(p):
    """componentProb 0 stands for 2/k (see load_random)."""
    return "2/k" if p == 0 else "{:g}".format(p)


def values(ds, key):
    return sorted({i["props"][key] for i in ds["instances"] if math.isfinite(i["props"].get(key, float("nan")))})


def ratio_axis(ax, lo, hi):
    """Log axis for ratios with readable ticks (0.5, 0.8, 1, 1.25, ...) instead of 10^0."""
    ticks = [t for t in RATIO_TICKS if lo / 1.05 <= t <= hi * 1.05] or [1]
    ax.yaxis.set_major_locator(FixedLocator(ticks))
    ax.yaxis.set_minor_locator(NullLocator())
    ax.yaxis.set_major_formatter(FuncFormatter(lambda v, _: "{:g}".format(v)))


def plot_random_lines(ds, path):
    ns, ins, comps, outs = (values(ds, k) for k in ("N", "inProb", "componentProb", "outProb"))
    with PdfPages(path) as pdf:
        for n, p_in in ((n, p_in) for n in ns for p_in in ins):
            ks = sorted({i["props"]["sccCount"] for i in ds["instances"] if i["props"]["N"] == n})
            fig, axes = plt.subplots(len(ks), len(comps), figsize=(3.0 * len(comps), 2.4 * len(ks)),
                                     sharex=True, sharey=True, squeeze=False)
            for r, k in enumerate(ks):
                for c, comp in enumerate(comps):
                    ax = axes[r][c]
                    sel = [i for i in ds["instances"] if i["props"]["N"] == n and i["props"]["sccCount"] == k
                           and i["props"]["inProb"] == p_in and i["props"]["componentProb"] == comp]
                    for j, a in enumerate(ds["algs"]):
                        ys = [geomean([i["times"][a] for i in sel if i["props"]["outProb"] == o and ok(i, a)]) for o in outs]
                        ax.plot(outs, ys, marker="o", markersize=3, linewidth=1.2, color=alg_color(a, j), label=a)
                    ax.set_xscale("log")
                    ax.set_yscale("log")
                    ax.grid(which="both", alpha=0.2)
                    if r == 0: ax.set_title("componentProb = {}".format(comp_label(comp)), fontsize=9)
                    if c == 0: ax.set_ylabel("{}\ntime (s)".format(k_label(k, n)), fontsize=9)
                    if r == len(ks) - 1: ax.set_xlabel("outProb", fontsize=9)
            handles, labels = axes[0][0].get_legend_handles_labels()
            fig.suptitle("randomSizeSCC, n = {:g}, inProb = {:g}".format(n, p_in), y=0.995, fontsize=10)
            fig.legend(handles, labels, loc="upper center", bbox_to_anchor=(0.5, 0.97), ncol=len(labels), frameon=False,
                       fontsize=9)
            fig.tight_layout(rect=(0, 0, 1, 0.93))
            pdf.savefig(fig)
            plt.close(fig)


def plot_random_scatter(ds, path, a, b):
    ks = sorted({i["props"]["kType"] for i in ds["instances"]}, key=k_type_order)
    xs_keys = [("tails/heads", True), ("outgoingEdges", True)]
    fig, axes = plt.subplots(1, len(xs_keys), figsize=(5.2 * len(xs_keys), 3.8), sharey=True)
    all_y = []
    for ax, (key, xlog) in zip(axes, xs_keys):
        for n_k, k in enumerate(ks):
            pts = [(i["props"][key], i["times"][a] / i["times"][b]) for i in ds["instances"]
                   if i["props"]["kType"] == k and ok(i, a) and ok(i, b) and i["props"][key] > 0]
            if pts:
                px, py = zip(*pts)
                all_y += py
                ax.scatter(px, py, s=14, alpha=0.75, color=GROUP_COLORS[n_k % len(GROUP_COLORS)],
                           label="k = {}".format(k))
        ax.axhline(1, color="#333", linewidth=0.8)
        ax.axvline(1, color="#999", linewidth=0.6, linestyle=":") if key == "tails/heads" else None
        ax.set_xscale("log" if xlog else "linear")
        ax.set_yscale("log")
        ax.set_xlabel(PROP_LABELS.get(key, key))
        ax.grid(which="both", alpha=0.2)
    if all_y:
        ratio_axis(axes[0], min(all_y), max(all_y))
    axes[0].set_ylabel("time {} / time {}".format(a, b))
    axes[0].legend(fontsize=8, frameon=False)
    fig.suptitle("{} vs. {} (< 1: {} faster)".format(a, b, a), fontsize=10)
    fig.tight_layout()
    fig.savefig(path)
    plt.close(fig)


def profile_ratios(datasets):
    """For each algorithm the ratios (time / best time) on all instances where it was run."""
    ratios = defaultdict(list)
    for ds in datasets:
        for i in ds["instances"]:
            best = min([i["times"][a] for a in ds["algs"] if ok(i, a)] or [float("nan")])
            for a in ds["algs"]:
                ratios[a].append(i["times"][a] / best if ok(i, a) and math.isfinite(best) else float("inf"))
    return ratios


def plot_profile(datasets, path):
    ratios = profile_ratios(datasets)
    finite = [r for rs in ratios.values() for r in rs if math.isfinite(r)]
    tmax = max(2.0, max(finite) * 1.1) if finite else 2.0
    taus = np.logspace(0, math.log10(tmax), 400)
    algs = list(ratios)
    fig, ax = plt.subplots(figsize=(6.2, 3.8))
    for j, a in enumerate(algs):
        rs = np.array(ratios[a])
        ax.step(taus, [(rs <= t).mean() for t in taus], where="post", color=alg_color(a, j), linewidth=1.6,
                label="{} (fastest: {:.0%})".format(a, (rs <= 1.0000001).mean()))
    ax.set_xscale("log")
    ax.set_ylim(0, 1.02)
    ax.set_xlabel("τ (time / best time)")
    ax.set_ylabel("fraction of instances")
    ax.grid(which="both", alpha=0.25)
    ax.legend(fontsize=8, frameon=False, loc="lower right")
    ax.set_title("Performance profile: " + ", ".join("{} ({})".format(d["name"], len(d["instances"])) for d in datasets),
                 fontsize=9)
    fig.tight_layout()
    fig.savefig(path)
    plt.close(fig)


def latex_table(ds, path, limit):
    algs = ds["algs"]
    lines = ["\\begin{tabular}{l" + "r" * 4 + "|" + "r" * len(algs) + "}", "\\toprule",
             "graph & $n$ & $m$ & sccV & sccE & " + " & ".join(algs) + " \\\\", "\\midrule"]
    for i in sorted(ds["instances"], key=lambda i: i["props"].get("N", 0)):
        p = i["props"]
        best = min([i["times"][a] for a in algs if ok(i, a)] or [float("nan")])
        cells = []
        for a in algs:
            s, t = i["status"].get(a), i["times"].get(a)
            if s == "TIMEOUT": cells.append("$>${:g}".format(limit))
            elif s == "CRASH": cells.append("--")
            else:
                c = "{:.3f}".format(t) + ("$^*$" if s == "WRONG" else "")
                cells.append("\\textbf{" + c + "}" if t == best else c)
        props = ["{:,}".format(int(p.get(k, 0))).replace(",", "\\,") for k in ("N", "E", "largestSCCVertices", "largestSCCEdges")]
        lines.append(" & ".join([i["label"].replace("_", "\\_")] + props + cells) + " \\\\")
    lines += ["\\bottomrule", "\\end{tabular}"]
    with open(path, "w") as f:
        f.write("\n".join(lines) + "\n")


def write_dashboard_data(files, limits, path):
    data = {"generated": datetime.datetime.now().strftime("%Y-%m-%d %H:%M"), "maxTime": limits["random"],
            "limits": limits, "files": {}}
    for kind, p in files.items():
        if p and os.path.exists(p):
            data["files"][kind] = {"name": os.path.relpath(p, ROOT), "text": open(p).read()}
    # Drawn graphs (run2.py, draw_graph.py) and figures, as paths relative to the dashboard
    data["pdfs"] = []
    for group, d in (("SCC graphs", os.path.join(HERE, "SCC")), ("Figures", os.path.join(HERE, "figures"))):
        if os.path.isdir(d):
            for name in sorted(os.listdir(d)):
                if name.lower().endswith(".pdf"):
                    data["pdfs"].append({"group": group, "name": name,
                                         "path": os.path.relpath(os.path.join(d, name), os.path.dirname(path))})
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "w") as f:
        f.write("// Written by results/Viz/plot_results.py, do not edit by hand.\nwindow.APSP_DATA = ")
        json.dump(data, f)
        f.write(";\n")


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--random", default=os.path.join(RESULTS, "randomSizeSCC_data.csv"))
    ap.add_argument("--equal", default=os.path.join(RESULTS, "equalSizeSCC_data.csv"))
    ap.add_argument("--star", default=os.path.join(RESULTS, "starSCC_data.csv"))
    ap.add_argument("--gnutella", default=os.path.join(RESULTS, "Gnutella.csv"))
    ap.add_argument("--snap", default=os.path.join(RESULTS, "SNAP_data.csv"))
    ap.add_argument("--out", default=os.path.join(HERE, "figures"), help="directory for figures and tables")
    ap.add_argument("--a", help="algorithm A for the scatter plot (default: all RANDOM_PAIRS)")
    ap.add_argument("--b", help="algorithm B for the comparison")
    ap.add_argument("--ref", help="reference for the speedup (default Tree)")
    ap.add_argument("--no-figures", action="store_true", help="only refresh dashboard/data.js")
    args = ap.parse_args()

    limits = time_limits()
    files = {"random": args.random, "equal": args.equal, "star": args.star, "gnutella": args.gnutella, "snap": args.snap}
    write_dashboard_data(files, limits, os.path.join(HERE, "dashboard", "data.js"))
    print("dashboard/data.js refreshed -> open results/Viz/dashboard/index.html")
    if args.no_figures:
        return

    loaders = {"random": load_random, "equal": load_random, "star": load_random, "gnutella": load_gnutella,
               "snap": load_snap}
    datasets = {}
    for kind, p in files.items():
        if not os.path.exists(p):
            print("no file {}, skipped".format(p))
            continue
        ds = loaders[kind](p, limits[kind])
        if ds["instances"]:
            datasets[kind] = ds
        else:
            print("{}: no result rows".format(p))

    os.makedirs(args.out, exist_ok=True)
    out = lambda name: os.path.join(args.out, name)
    for kind in ("gnutella", "snap"):
        if kind in datasets:
            ds = datasets[kind]
            ref = args.ref if args.ref in ds["algs"] else find_alg(ds["algs"], "tree") or ds["algs"][0]
            plot_times(ds, out(kind + "_times.pdf"), limits[kind])
            plot_speedup(ds, out(kind + "_speedup.pdf"), ref)
            latex_table(ds, out(kind + "_table.tex"), limits[kind])
    if "random" in datasets:
        ds = datasets["random"]
        if args.a in ds["algs"] and args.b in ds["algs"]:
            pairs = [(args.a, args.b)]
        else:
            pairs = [(a, b) for a, b in RANDOM_PAIRS if a in ds["algs"] and b in ds["algs"]]
        plot_random_lines(ds, out("random_lines.pdf"))
        for a, b in pairs:
            tag = "{}_vs_{}".format(a, b)
            plot_random_scatter(ds, out("random_scatter_{}.pdf".format(tag)), a, b)
    if datasets:
        plot_profile(list(datasets.values()), out("performance_profile.pdf"))
    print("figures and tables in {}".format(args.out))
    for f in sorted(os.listdir(args.out)):
        print("  " + f)


if __name__ == "__main__":
    main()
