"""DAG komponent z Graphviz (dot): od leve proti desni, povezave se izogibajo vozliščem in se ne prekrivajo.
Potrebuje samo program `dot` (Graphviz), numpy in matplotlib (za mrežo slik)."""
import subprocess, sys, io, os, argparse, csv
import fitz
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

def load_metadata(csv_path):
    """Prebere glavo in vrstice z meritvami v istem vrstnem redu kot grafi."""
    with open(csv_path, newline="", encoding="utf-8-sig") as f:
        reader = csv.DictReader(f, skipinitialspace=True)
        headers = reader.fieldnames or []
        rows = list(reader)
    if not headers:
        raise ValueError(f"Prazna tabela z meritvami: {csv_path}")
    return headers, rows


def draw_metadata(page, row, headers, x, y, width):
    """Nariše vse stolpce CSV v pregledni tabeli 3 stolpcev nad grafom."""
    if row is None:
        page.insert_text((x, y + 13), "Ni pripadajoce vrstice v CSV!",
                         fontsize=9, color=(0.7, 0.2, 0.15))
        return 27

    ncols = 3
    nrows = int(np.ceil(len(headers) / ncols))
    cell_w = width / ncols
    row_h = 13
    # Vsaka celica je par (ime stolpca, vrednost). Stolpci so razvrščeni
    # po vrstah, zato vrstni red ustreza vrstnemu redu v CSV.
    for index, key in enumerate(headers):
        col, r = index % ncols, index // ncols
        xx = x + col * cell_w
        yy = y + r * row_h
        page.draw_rect(fitz.Rect(xx, yy, xx + cell_w, yy + row_h),
                       color=(0.84, 0.87, 0.91), width=0.4,
                       fill=(0.96, 0.97, 0.99) if r % 2 == 0 else (1, 1, 1))
        label = key
        value = str(row.get(key, "")).strip()
        page.insert_text((xx + 3, yy + 9.5), label, fontsize=7.2,
                         color=(.22, .26, .33), fontname="helv")
        vw = fitz.get_text_length(value, fontname="hebo", fontsize=8)
        page.insert_text((xx + cell_w - 4 - vw, yy + 9.5), value,
                         fontsize=8, color=(.07, .16, .35), fontname="hebo")
    return nrows * row_h + 8


def save_multipage_pdf(blocks, szs, path, graphs_per_page=2, landscape=False,
                       metadata_headers=None, metadata_rows=None):
    """Večstranski vektorski PDF s celotno vrstico CSV nad vsakim grafom."""
    if graphs_per_page < 1:
        raise ValueError("graphs_per_page mora biti vsaj 1")
    page_w, page_h = fitz.paper_size("a4")
    if landscape:
        page_w, page_h = page_h, page_w
    margin_x, margin_top, margin_bottom = 28, 35, 28
    slot_h = (page_h - margin_top - margin_bottom) / graphs_per_page
    pdf = fitz.open()
    headers = metadata_headers or []
    rows = metadata_rows or []

    for k, (A, sz) in enumerate(zip(blocks, szs)):
        if k % graphs_per_page == 0:
            page = pdf.new_page(width=page_w, height=page_h)
            page_num = k // graphs_per_page + 1
            page.insert_text((margin_x, 20), f"DAG grafi - stran {page_num}",
                             fontsize=9, color=(.35, .35, .35))
        slot = k % graphs_per_page
        y_top = margin_top + slot * slot_h
        page.insert_text((margin_x, y_top + 11), f"Graf {k}",
                         fontsize=11, color=(.15, .15, .15))

        info_h = 0
        if headers:
            entry = rows[k] if k < len(rows) else None
            info_h = draw_metadata(page, entry, headers,
                                   margin_x, y_top + 17, page_w - 2 * margin_x)
        plot_area = fitz.Rect(margin_x, y_top + 18 + info_h,
                              page_w - margin_x, y_top + slot_h - 8)
        if plot_area.height <= 0:
            raise ValueError("Premalo prostora: zmanjšaj --graphs-per-page")

        source = to_dot(A, sz, f"g{k}").encode("utf-8")
        result = subprocess.run(["dot", "-Tpdf"], input=source,
                                capture_output=True, check=True)
        graph_pdf = fitz.open(stream=result.stdout, filetype="pdf")
        src = graph_pdf[0].rect
        scale = min(plot_area.width / src.width, plot_area.height / src.height)
        w, h = src.width * scale, src.height * scale
        target = fitz.Rect(plot_area.x0 + (plot_area.width-w)/2,
                           plot_area.y0 + (plot_area.height-h)/2,
                           plot_area.x0 + (plot_area.width+w)/2,
                           plot_area.y0 + (plot_area.height+h)/2)
        page.show_pdf_page(target, graph_pdf, 0, keep_proportion=True)
        graph_pdf.close()
    pdf.save(path, garbage=4, deflate=True)
    pdf.close()


if __name__ == "__main__":
    HERE = os.path.dirname(os.path.abspath(__file__))
    parser = argparse.ArgumentParser(description="Izris DAG grafov v večstranski vektorski PDF")
    parser.add_argument("outs", nargs="?", default=os.path.join(HERE, "outs.csv"))
    parser.add_argument("sizes", nargs="?", default=os.path.join(HERE, "sizes.csv"))
    parser.add_argument("single_index", nargs="?", type=int, default=None,
                        help="Neobvezno: indeks dodatnega posameznega grafa PNG")
    parser.add_argument("--graphs-per-page", type=int, default=2,
                        help="Število grafov na stran (privzeto 2)")
    parser.add_argument("--landscape", action="store_true", help="Ležeče strani A4")
    parser.add_argument("--output", default=os.path.join(HERE, "dag_graphviz_all.pdf"))
    parser.add_argument("--data", default=os.path.normpath(os.path.join(HERE, "../../randomSizeSCC_data.csv")),
                        help="CSV z meritvami; privzeto ../../randomSizeSCC_data.csv glede na run.py")
    parser.add_argument("--no-data", action="store_true", help="Izriši samo grafe, brez meritev")
    args = parser.parse_args()

    blocks, szs = load(args.outs, args.sizes)
    headers, metadata = [], []
    if not args.no_data:
        if not os.path.isfile(args.data):
            parser.error(f"Ne najdem CSV z meritvami: {args.data} (podaj --data POT ali --no-data)")
        headers, metadata = load_metadata(args.data)
        if len(metadata) != len(blocks):
            print(f"OPOZORILO: {len(blocks)} grafov, {len(metadata)} vrstic meritev. "
                  "Manjkajoce vrstice bodo oznacene v PDF.", file=sys.stderr)
        for k, row in enumerate(metadata[:len(blocks)]):
            if "sccCount" in row and str(row["sccCount"]).strip():
                if int(float(row["sccCount"])) != len(szs[k]):
                    print(f"OPOZORILO: Graf {k} ima {len(szs[k])} komponent, "
                          f"CSV pa navaja {row['sccCount']}.", file=sys.stderr)
    save_multipage_pdf(blocks, szs, args.output, args.graphs_per_page,
                       args.landscape, headers, metadata)
    print(f"Ustvarjen PDF: {args.output} ({len(blocks)} grafov, "
          f"{int(np.ceil(len(blocks)/args.graphs_per_page))} strani)")
    if args.single_index is not None:
        k = args.single_index
        if not (0 <= k < len(blocks)):
            parser.error(f"single_index mora biti med 0 in {len(blocks)-1}")
        mpimg.imsave(os.path.join(HERE, "dag_graphviz_single.png"),
                     render(blocks[k], szs[k], f"g{k}", dpi=150))
