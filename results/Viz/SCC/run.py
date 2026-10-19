"""DAG komponent z Graphviz (dot): od leve proti desni, povezave se izogibajo vozliščem in se ne prekrivajo.
Potrebuje samo program `dot` (Graphviz), numpy in matplotlib (za mrežo slik)."""
import subprocess, sys, io, os
import numpy as np
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import matplotlib.image as mpimg

def load(outs_path, sizes_path):
    blocks, cur = [], []
    for line in open(outs_path):
        line = line.strip()
        if not line:
            if cur: blocks.append(np.array(cur, float)); cur = []
        else:
            cur.append([float(x) for x in line.split(",")])
    if cur: blocks.append(np.array(cur, float))
    sizes = [[float(x) for x in l.split(",")] for l in open(sizes_path) if l.strip()]
    assert len(blocks) == len(sizes), (len(blocks), len(sizes))
    return blocks, sizes

def to_dot(A, sz, name):
    smax = max(sz)
    out = [f'digraph "{name}" {{', 'rankdir=LR; splines=spline; nodesep=0.5; ranksep=1.1; '
           'node [shape=circle, fixedsize=true, style=filled, fillcolor="#4C8BF5", fontcolor=white, fontsize=11]; '
           'edge [color="#555555", fontsize=10, arrowsize=0.8];']
    for v, s in enumerate(sz):
        d = 0.5 + 0.9 * np.sqrt(s / smax)
        out.append(f'n{v} [label="{int(s)}", width={d:.2f}];')
    emax = A.max() if A.max() > 0 else 1
    for i in range(len(A)):
        for j in range(len(A)):
            if A[i, j] > 0:
                out.append(f'n{i} -> n{j} [label="{int(A[i,j])}", penwidth={0.8 + 3.2*A[i,j]/emax:.2f}];')
    out.append("}")
    return "\n".join(out)

def render(A, sz, name, dpi=110):
    png = subprocess.run(["dot", "-Tpng", f"-Gdpi={dpi}"], input=to_dot(A, sz, name).encode(),
                         capture_output=True, check=True).stdout
    return mpimg.imread(io.BytesIO(png), format="png")

if __name__ == "__main__":
    HERE = os.path.dirname(os.path.abspath(__file__))   # mapa, kjer leži skripta
    outs, sizes = (sys.argv[1], sys.argv[2]) if len(sys.argv) > 2 else (os.path.join(HERE, "outs.csv"), os.path.join(HERE, "sizes.csv"))
    blocks, szs = load(outs, sizes)
    cols = 3
    rows = int(np.ceil(len(blocks) / cols))
    fig, axes = plt.subplots(rows, cols, figsize=(6 * cols, 3.6 * rows))
    for ax in np.ravel(axes): ax.axis("off")
    for k, (A, s) in enumerate(zip(blocks, szs)):
        ax = np.ravel(axes)[k]
        ax.imshow(render(A, s, f"g{k}")); ax.set_title(f"graf {k}", fontsize=9)
    fig.tight_layout()
    fig.savefig(os.path.join(HERE, "dag_graphviz_all.png"), dpi=120)
    # en graf posebej
    k = int(sys.argv[3]) if len(sys.argv) > 3 else 3
    mpimg.imsave(os.path.join(HERE, "dag_graphviz_single.png"), render(blocks[k], szs[k], f"g{k}", dpi=150))
