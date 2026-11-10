"use strict";

// Viewer for the APSP test results. Reads data.js (written by results/Viz/plot_results.py).
// Charts are plain SVG, no external libraries.

// Colours and labels ----------------------------------------------------------------------------------------------
const PALETTE = ["#2E9E44", "#1F6FB4", "#8E5BC9", "#E07B1A", "#D64545", "#17A2B8", "#8C6D31", "#E377C2"];

const ALG_COLORS = { "Tree": "#2E9E44", "Edge-SCC": "#1F6FB4", "Vertex-SCC": "#8E5BC9", "VertexNaive-SCC": "#17A2B8",
  "Adaptive-SCC": "#E07B1A", "Vertex-SCC-reverse": "#D64545",
  "Vertex-SCC-hourglass": "#C2185B",
  "Vertex-SCC-bridgeHG": "#6D4C41",
  "Dijkstra": "#5D6D7E", "Uniform": "#B7950B", "Hidden": "#A04000", "FW": "#212F3D" };

function algColor(name, i) {
  if (ALG_COLORS[name]) return ALG_COLORS[name];
  const n = name.toLowerCase();
  if (n.includes("adapt")) return "#E07B1A";
  if (n.includes("vertex")) return "#8E5BC9";
  if (n.includes("edge")) return "#1F6FB4";
  if (n.includes("tree") || n.includes("hourglass")) return "#2E9E44";
  return PALETTE[i % PALETTE.length];
}

const GNUTELLA_PROPS = { V: "N", E: "E", wccV: "largestWCCVertices", wccE: "largestWCCEdges",
  sccV: "largestSCCVertices", sccE: "largestSCCEdges" };
const KIND_NAMES = { random: "Random SCC", equal: "Equal SCC", star: "Star SCC", gnutella: "Gnutella", snap: "SNAP" };
const RANDOM_KINDS = ["random", "equal", "star"]; // generated SCC graphs, all in the randomSizeSCC CSV format

const fmtTime = t => !Number.isFinite(t) ? "–" : t >= 100 ? t.toFixed(0) : t >= 10 ? t.toFixed(1) : t >= 1 ? t.toFixed(2) : t.toFixed(3);
const fmtNum = v => !Number.isFinite(v) ? "–" : Math.abs(v) >= 1000 ? Math.round(v).toLocaleString("en-US") : String(+v.toPrecision(3));
const fmtRatio = v => !Number.isFinite(v) ? "–" : v.toFixed(2);
const fmtTick = v => Math.abs(v) >= 1000 ? Math.round(v).toLocaleString("en-US") : String(+v.toPrecision(3));
const esc = s => String(s).replace(/[&<>"]/g, c => ({ "&": "&amp;", "<": "&lt;", ">": "&gt;", '"': "&quot;" }[c]));

// CSV parsing --------------------------------------------------------------------------------------------------------
function readRows(text) {
  return text.split(/\r?\n/).map(line => {
    const r = line.split(",").map(c => c.trim());
    while (r.length && r[r.length - 1] === "") r.pop();
    return r;
  }).filter(r => r.length);
}

function detectKind(rows) {
  if (!rows.length) return null;
  const h = rows[0].map(s => s.toLowerCase());
  if (h[0] === "algorithm") return "gnutella";
  if (h[0] === "graph") return "snap";
  if (h.includes("outprob")) return "random";
  return null;
}

const num = x => { const v = parseFloat(x); return Number.isFinite(v) ? v : NaN; };

// RunTest writes only the time to the CSV: the limit means TIMEOUT, -1 means CRASH
function statusFromTime(t, limit) {
  if (!Number.isFinite(t) || t < 0) return "CRASH";
  if (Math.abs(t - limit) < 0.0015) return "TIMEOUT";
  return "ok";
}

function addDerived(p) {
  p["tails/heads"] = p.heads ? p.tails / p.heads : NaN;
  p["largestSCC/N"] = p.N ? p.largestSCCVertices / p.N : NaN;
}

// "log n" or "√n" if the number of components was given as COMPS_LOG_N or COMPS_SQRT_N, else k
function kType(k, n) {
  if (k === Math.floor(Math.log2(n))) return "log n";
  if (k === Math.floor(Math.sqrt(n))) return "√n";
  return String(k);
}
const kLabel = (k, n) => { const t = kType(k, n); return `k = ${fmtTick(k)}` + (t === String(k) ? "" : ` (${t})`); };

const compLabel = p => p === 0 ? "2/k" : fmtTick(p);

function loadRandom(rows, limit) {
  const head = rows[0];
  const k = head.indexOf("outProb");
  const algs = head.slice(0, k);
  const instances = [];
  for (const r of rows.slice(1)) {
    if (r.length < head.length) continue; // interrupted run
    const times = {}, status = {}, props = {};
    algs.forEach((a, i) => { times[a] = num(r[i]); status[a] = statusFromTime(times[a], limit); });
    for (let i = k; i < head.length; i++) props[head[i]] = num(r[i]);
    addDerived(props);
    const comps = Number.isFinite(props.comps) ? props.comps : props.sccCount;
    props.kType = kType(comps, props.N);
    // componentProb = 2/k depends on k, so all of them share the code 0 (the CSV keeps 3 decimals)
    if (props.componentProb < 1 && Math.abs(props.componentProb - Math.round(2000 / comps) / 1000) < 1e-6) props.componentProb = 0;
    instances.push({ props, times, status });
  }
  // For k = 2, 2/k = 1, so both runs are written as 1.000. The test runs 2/k first: the first half of the rows
  // with the same parameters is 2/k.
  const k2 = new Map();
  for (const i of instances) {
    const p = i.props, comps = Number.isFinite(p.comps) ? p.comps : p.sccCount;
    if (comps !== 2 || p.componentProb !== 1) continue;
    const key = [p.N, p.outProb, p.inProb].join();
    if (!k2.has(key)) k2.set(key, []);
    k2.get(key).push(i);
  }
  for (const g of k2.values()) g.slice(0, g.length >> 1).forEach(i => { i.props.componentProb = 0; });
  for (const i of instances) {
    const p = i.props, comps = Number.isFinite(p.comps) ? p.comps : p.sccCount;
    i.label = `n=${p.N} k=${comps} out=${p.outProb} in=${p.inProb} comp=${compLabel(p.componentProb)}`;
  }
  return { kind: "random", algs, instances };
}

// TESTRUNS > 1 runs the same real graph several times: time = mean over the runs. A run that did not finish
// decides the status (CRASH before TIMEOUT before WRONG).
function mergeRuns(groups, limit) {
  return groups.map(g => {
    const times = {}, status = {};
    for (const [a, runs] of Object.entries(g.runs)) {
      const st = ["CRASH", "TIMEOUT", "WRONG"].find(x => runs.some(r => r.status === x)) || "ok";
      status[a] = st;
      times[a] = st === "CRASH" ? -1 : st === "TIMEOUT" ? limit : runs.reduce((s, r) => s + r.t, 0) / runs.length;
    }
    g.props.runs = Math.max(...Object.values(g.runs).map(r => r.length));
    return { label: g.label, props: g.props, times, status };
  });
}

function loadSnap(rows, limit) {
  const head = rows[0];
  const firstStatus = head.findIndex(h => h.endsWith(" status"));
  const algs = head.slice(1, firstStatus);
  const propStart = firstStatus + algs.length;
  const groups = new Map();
  for (const r of rows.slice(1)) {
    if (r.length < head.length) continue;
    const label = r[0].replace(/\.(txt|csv)$/, "");
    if (!groups.has(label)) {
      const props = {};
      for (let i = propStart; i < head.length; i++) props[head[i]] = num(r[i]);
      addDerived(props);
      groups.set(label, { label, props, runs: Object.fromEntries(algs.map(a => [a, []])) });
    }
    algs.forEach((a, i) => groups.get(label).runs[a].push({ t: num(r[1 + i]), status: r[firstStatus + i] }));
  }
  return { kind: "snap", algs, instances: mergeRuns([...groups.values()], limit) };
}

function loadGnutella(rows, limit) {
  const head = rows[0];
  const col = Object.fromEntries(head.map((h, i) => [h, i]));
  const algs = [], groups = new Map();
  for (const r of rows.slice(1)) {
    if (r.length < head.length) continue;
    const a = r[col.algorithm];
    if (!algs.includes(a)) algs.push(a);
    const key = [r[col.id], r[col.V], r[col.E]].join("|");
    if (!groups.has(key)) {
      const props = {};
      for (const [h, p] of Object.entries(GNUTELLA_PROPS)) props[p] = num(r[col[h]]);
      addDerived(props);
      const id = Math.round(num(r[col.id]));
      groups.set(key, { label: "Gnutella" + String(id).padStart(2, "0"), props, runs: {} });
    }
    const g = groups.get(key), t = num(r[col.time]);
    (g.runs[a] ||= []).push({ t, status: statusFromTime(t, limit) });
  }
  return { kind: "gnutella", algs, instances: mergeRuns([...groups.values()], limit) };
}

const LOADERS = { random: loadRandom, equal: loadRandom, star: loadRandom, gnutella: loadGnutella, snap: loadSnap };

// State ------------------------------------------------------------------------------------------------------------
const state = { data: {}, sources: {}, pdfs: [], limit: 60, limits: {}, generated: null, tab: "overview", hidden: new Set(), c: {} };
const TABS = ["overview", "random", "equal", "star", "gnutella", "snap", "pdfs"];
const STORE_KEY = "apsp-dashboard-v1";

function saveState() {
  try {
    localStorage.setItem(STORE_KEY, JSON.stringify({ tab: state.tab, hidden: [...state.hidden], c: state.c }));
  } catch (e) { /* private window or blocked storage */ }
}

function restoreState() {
  try {
    const s = JSON.parse(localStorage.getItem(STORE_KEY) || "null");
    if (s) { state.tab = s.tab || state.tab; state.hidden = new Set(s.hidden || []); state.c = s.c || {}; }
  } catch (e) { /* empty */ }
}

const ok = (inst, a) => inst.status[a] === "ok" && Number.isFinite(inst.times[a]) && inst.times[a] > 0;

function geomean(xs) {
  const v = xs.filter(x => x > 0 && Number.isFinite(x));
  return v.length ? Math.exp(v.reduce((s, x) => s + Math.log(x), 0) / v.length) : NaN;
}

function allAlgs() {
  const out = [];
  for (const kind of Object.keys(KIND_NAMES))
    for (const a of state.data[kind]?.algs || []) if (!out.includes(a)) out.push(a);
  return out;
}
const colorOf = a => algColor(a, allAlgs().indexOf(a));
const visible = ds => ds.algs.filter(a => !state.hidden.has(a));

function bestTime(inst, algs) {
  const ts = algs.filter(a => ok(inst, a)).map(a => inst.times[a]);
  return ts.length ? Math.min(...ts) : NaN;
}

// DOM helpers ------------------------------------------------------------------------------------------------------
function h(tag, attrs = {}, ...children) {
  const el = document.createElement(tag);
  for (const [k, v] of Object.entries(attrs)) {
    if (v === undefined || v === null || v === false) continue;
    if (k.startsWith("on")) el.addEventListener(k.slice(2), v);
    else if (k === "html") el.innerHTML = v;
    else el.setAttribute(k, v === true ? "" : v);
  }
  for (const c of children.flat()) if (c !== null && c !== undefined && c !== false) el.append(c.nodeType ? c : String(c));
  return el;
}

const SVGNS = "http://www.w3.org/2000/svg";
function s(tag, attrs = {}, parent = null, text = null) {
  const el = document.createElementNS(SVGNS, tag);
  for (const [k, v] of Object.entries(attrs)) if (v !== undefined && v !== null) el.setAttribute(k, v);
  if (text !== null) el.textContent = text;
  if (parent) parent.appendChild(el);
  return el;
}

const tip = document.getElementById("tip");
function moveTip(e) {
  const pad = 14, r = tip.getBoundingClientRect();
  let x = e.clientX + pad, y = e.clientY + pad;
  if (x + r.width > innerWidth - 8) x = e.clientX - r.width - pad;
  if (y + r.height > innerHeight - 8) y = e.clientY - r.height - pad;
  tip.style.left = Math.max(4, x) + "px";
  tip.style.top = Math.max(4, y) + "px";
}
function hover(node, html) {
  node.classList.add("hit");
  node.addEventListener("mouseenter", e => { tip.innerHTML = html; tip.hidden = false; moveTip(e); });
  node.addEventListener("mousemove", moveTip);
  node.addEventListener("mouseleave", () => { tip.hidden = true; });
}

// Controls: the value is stored in state.c[key]
function select(label, key, options, def) {
  const values = options.map(o => String(o.value));
  if (!values.includes(String(state.c[key]))) state.c[key] = String(def);
  const sel = h("select", { onchange: e => { state.c[key] = e.target.value; saveState(); render(); } },
    options.map(o => h("option", { value: o.value, selected: String(o.value) === String(state.c[key]) }, o.label)));
  return h("label", { class: "control" }, label, sel);
}

function checkbox(label, key, def) {
  if (typeof state.c[key] !== "boolean") state.c[key] = def;
  return h("label", { class: "check" },
    h("input", { type: "checkbox", checked: state.c[key], onchange: e => { state.c[key] = e.target.checked; saveState(); render(); } }),
    label);
}

function segmented(key, options, def) {
  if (!options.some(o => o.value === state.c[key])) state.c[key] = def;
  return h("div", { class: "seg", role: "group" }, options.map(o =>
    h("button", { type: "button", "aria-pressed": String(o.value === state.c[key]),
      onclick: () => { state.c[key] = o.value; saveState(); render(); } }, o.label)));
}

// Axes and scales -----------------------------------------------------------------------------------------------------
function scale(log, d0, d1, r0, r1) {
  const t = v => (log ? Math.log10(v) : v);
  const a = t(d0), b = t(d1);
  const f = v => r0 + ((t(v) - a) / ((b - a) || 1)) * (r1 - r0);
  f.log = log;
  f.domain = [d0, d1];
  f.ticks = () => (log ? logTicks(d0, d1) : linTicks(d0, d1, 6));
  return f;
}

function linTicks(a, b, n) {
  const raw = (b - a) / n || 1, mag = 10 ** Math.floor(Math.log10(raw));
  const step = [1, 2, 2.5, 5, 10].map(m => m * mag).find(x => x >= raw);
  const out = [];
  for (let v = Math.ceil(a / step) * step; v <= b + step * 1e-9; v += step) out.push(+v.toPrecision(12));
  return out;
}

function logTicks(a, b) {
  const lo = Math.floor(Math.log10(a)), hi = Math.ceil(Math.log10(b));
  let out = [];
  for (const subs of [[1], [1, 2, 5], [1, 1.5, 2, 3, 5, 7], [1, 1.25, 1.5, 2, 2.5, 3, 4, 5, 6, 8]]) {
    out = [];
    for (let e = lo; e <= hi; e++)
      for (const m of subs) {
        const v = +(m * 10 ** e).toPrecision(6);
        if (v >= a * 0.999 && v <= b * 1.001) out.push(v);
      }
    if (out.length >= 3) return out;
  }
  return out.length ? out : [a, b];
}

function extent(vals, log, pad = 0.08) {
  const v = vals.filter(x => Number.isFinite(x) && (!log || x > 0));
  if (!v.length) return log ? [0.1, 1] : [0, 1];
  let lo = Math.min(...v), hi = Math.max(...v);
  if (log) {
    if (lo === hi) { lo /= 2; hi *= 2; }
    const f = (hi / lo) ** pad;
    return [lo / f, hi * f];
  }
  if (lo === hi) { lo -= 1; hi += 1; }
  const d = (hi - lo) * pad;
  return [lo > 0 && lo - d < 0 ? 0 : lo - d, hi + d];
}

// Chart frame in container pixels, so text always has the same size
function frame(container, { height = 360, m = {} } = {}) {
  const W = Math.max(320, Math.floor(container.clientWidth || 900));
  const mm = Object.assign({ l: 64, r: 16, t: 14, b: 48 }, m);
  const svg = s("svg", { class: "chart", viewBox: `0 0 ${W} ${height}`, width: W, height });
  container.appendChild(svg);
  return { svg, m: mm, W, H: height, iw: W - mm.l - mm.r, ih: height - mm.t - mm.b };
}

function yAxis(c, y, label, x0 = c.m.l, width = c.iw, fmt = fmtTick) {
  const g = s("g", {}, c.svg);
  for (const t of y.ticks()) {
    const yy = y(t);
    s("line", { class: "grid", x1: x0, x2: x0 + width, y1: yy, y2: yy }, g);
    s("text", { class: "tick", x: x0 - 6, y: yy, "text-anchor": "end", "dominant-baseline": "middle" }, g, fmt(t));
  }
  if (label) s("text", { class: "axis-label", transform: `translate(14 ${c.m.t + c.ih / 2}) rotate(-90)`, "text-anchor": "middle" }, g, label);
}

function xAxis(c, x, label, y0 = c.m.t + c.ih, fmt = fmtTick) {
  const g = s("g", {}, c.svg);
  s("line", { class: "axis", x1: c.m.l, x2: c.m.l + c.iw, y1: y0, y2: y0 }, g);
  for (const t of x.ticks()) {
    const xx = x(t);
    s("line", { class: "grid", x1: xx, x2: xx, y1: c.m.t, y2: y0 }, g);
    s("text", { class: "tick", x: xx, y: y0 + 16, "text-anchor": "middle" }, g, fmt(t));
  }
  if (label) s("text", { class: "axis-label", x: c.m.l + c.iw / 2, y: c.H - 6, "text-anchor": "middle" }, g, label);
}

function hatch(svg, id, color) {
  let defs = svg.querySelector("defs") || s("defs", {}, svg);
  const p = s("pattern", { id, patternUnits: "userSpaceOnUse", width: 6, height: 6, patternTransform: "rotate(45)" }, defs);
  s("rect", { width: 6, height: 6, fill: "var(--surface)" }, p);
  s("line", { x1: 0, y1: 0, x2: 0, y2: 6, stroke: color, "stroke-width": 2.2 }, p);
  return `url(#${id})`;
}

let uid = 0;

// Charts ------------------------------------------------------------------------------------------------------------

// Grouped bar chart. series: [{name, color, values: [{v, status, tip}]}], base: e.g. 1 for ratios
function barChart(container, { cats, series, log = true, yLabel, base = null, limit = null, height = 540 }) {
  const long = cats.some(c => c.length > 12) || cats.length > 8;
  const c = frame(container, { height, m: { b: long ? 96 : 48 } });
  const vals = [];
  for (const sr of series) for (const p of sr.values) {
    if (p.status === "TIMEOUT" && limit) vals.push(limit);
    else if (Number.isFinite(p.v) && (!log || p.v > 0)) vals.push(p.v);
  }
  if (base !== null) vals.push(base);
  const [d0, d1] = extent(vals, log, 0.06);
  const y = scale(log, d0, d1, c.m.t + c.ih, c.m.t);
  yAxis(c, y, yLabel);
  const bw = c.iw / Math.max(1, cats.length), inner = (bw * 0.8) / Math.max(1, series.length);
  const y0 = base !== null ? y(base) : c.m.t + c.ih;
  series.forEach((sr, j) => {
    const fillHatch = hatch(c.svg, `h${++uid}`, sr.color);
    sr.values.forEach((p, i) => {
      const x = c.m.l + i * bw + bw * 0.1 + j * inner;
      if (p.status === "TIMEOUT" && limit) {
        const r = s("rect", { x, y: y(limit), width: Math.max(1, inner - 1), height: Math.max(0, y0 - y(limit)), fill: fillHatch, stroke: sr.color }, c.svg);
        hover(r, p.tip);
      } else if (!Number.isFinite(p.v) || (log && p.v <= 0)) {
        const t = s("text", { x: x + inner / 2, y: c.m.t + c.ih - 4, "text-anchor": "middle", fill: sr.color, style: `fill:${sr.color};font-weight:700` }, c.svg, "×");
        hover(t, p.tip);
      } else {
        const yy = y(p.v);
        const r = s("rect", { x, y: Math.min(yy, y0), width: Math.max(1, inner - 1), height: Math.max(1, Math.abs(y0 - yy)), fill: sr.color,
          stroke: p.status === "WRONG" ? "var(--bad)" : null, "stroke-width": p.status === "WRONG" ? 2 : null }, c.svg);
        hover(r, p.tip);
      }
    });
  });
  if (base !== null) s("line", { class: "ref", x1: c.m.l, x2: c.m.l + c.iw, y1: y0, y2: y0 }, c.svg);
  s("line", { class: "axis", x1: c.m.l, x2: c.m.l + c.iw, y1: c.m.t + c.ih, y2: c.m.t + c.ih }, c.svg);
  cats.forEach((cat, i) => {
    const x = c.m.l + i * bw + bw / 2, yy = c.m.t + c.ih + 14;
    s("text", long ? { class: "tick", x, y: yy, "text-anchor": "end", transform: `rotate(-32 ${x} ${yy})` }
      : { class: "tick", x, y: yy + 2, "text-anchor": "middle" }, c.svg, cat);
  });
}

// Line chart; series: [{name, color, points: [{x, y, tip}]}]
function lineChart(container, { series, xlog, ylog, xLabel, yLabel, step = false, height = 500, yDomain = null, yMax = null, refY = null, xTicks = null }) {
  const c = frame(container, { height });
  const xs = [], ys = [];
  for (const sr of series) for (const p of sr.points) { xs.push(p.x); ys.push(p.y); }
  const [x0, x1] = extent(xs, xlog, step ? 0 : 0.04);
  let [y0, y1] = yDomain || extent(ys, ylog);
  if (yMax !== null) { y1 = yMax; if (y0 >= y1) y0 = ylog ? y1 / 10 : 0; } // values above yMax are clipped
  const x = scale(xlog, x0, x1, c.m.l, c.m.l + c.iw);
  const y = scale(ylog, y0, y1, c.m.t + c.ih, c.m.t);
  if (xTicks) x.ticks = () => xTicks;
  yAxis(c, y, yLabel);
  xAxis(c, x, xLabel);
  if (refY !== null) s("line", { class: "ref", x1: c.m.l, x2: c.m.l + c.iw, y1: y(refY), y2: y(refY) }, c.svg);
  const clipId = `clip${++uid}`;
  s("rect", { x: c.m.l, y: c.m.t - 5, width: c.iw, height: c.ih + 10 }, s("clipPath", { id: clipId }, s("defs", {}, c.svg)));
  const plot = s("g", { "clip-path": `url(#${clipId})` }, c.svg);
  for (const sr of series) {
    let d = "", pen = false, prevY = null;
    for (const p of sr.points) {
      const good = Number.isFinite(p.x) && Number.isFinite(p.y) && (!xlog || p.x > 0) && (!ylog || p.y > 0);
      if (!good) { pen = false; continue; }
      const X = x(p.x).toFixed(1), Y = y(p.y).toFixed(1);
      if (!pen) d += `M${X},${Y}`;
      else d += step ? `H${X}V${Y}` : `L${X},${Y}`;
      pen = true;
      prevY = Y;
    }
    if (step && prevY !== null) d += `H${(c.m.l + c.iw).toFixed(1)}`;
    s("path", { d, fill: "none", stroke: sr.color, "stroke-width": step ? 2 : 1.8, "stroke-linejoin": "round" }, plot);
    if (!step)
      for (const p of sr.points) {
        if (!(Number.isFinite(p.x) && Number.isFinite(p.y) && (!ylog || p.y > 0))) continue;
        hover(s("circle", { cx: x(p.x), cy: y(p.y), r: 3.5, fill: sr.color, stroke: "var(--surface)", "stroke-width": 1 }, plot), p.tip);
      }
  }
  return c;
}

// Sortable table; columns: [{key, label, get, fmt, txt, best}]
function sortTable(id, columns, rows, defKey, defDir = 1) {
  const sk = `sort-${id}`;
  const cur = state.c[sk] || { key: defKey, dir: defDir };
  const col = columns.find(cl => cl.key === cur.key) || columns[0];
  const sorted = rows.slice().sort((r1, r2) => {
    const a = col.get(r1), b = col.get(r2);
    const na = typeof a === "number" && !Number.isFinite(a), nb = typeof b === "number" && !Number.isFinite(b);
    if (na !== nb) return na ? 1 : -1;
    return (typeof a === "string" ? a.localeCompare(b, "en", { numeric: true }) : a - b) * cur.dir;
  });
  const head = h("tr", {}, columns.map(cl => h("th", {
    class: "sortable" + (cl.txt ? " txt" : ""),
    onclick: () => { state.c[sk] = { key: cl.key, dir: cur.key === cl.key ? -cur.dir : 1 }; saveState(); render(); },
  }, cl.label, cur.key === cl.key ? h("span", { class: "arrow" }, cur.dir > 0 ? "▲" : "▼") : null)));
  const body = sorted.map(r => h("tr", {}, columns.map(cl => {
    const cellHtml = cl.fmt ? cl.fmt(r) : esc(cl.get(r));
    return h("td", { class: [cl.txt ? "txt" : "", cl.best && cl.best(r) ? "best" : ""].join(" ").trim() || null, html: cellHtml });
  })));
  return h("div", { class: "table-box" }, h("table", {}, h("thead", {}, head), h("tbody", {}, body)));
}

function timeCell(inst, a) {
  const st = inst.status[a];
  const t = st === "TIMEOUT" ? `&gt;${inst.limit}` : st === "CRASH" ? "–" : fmtTime(inst.times[a]);
  return t + (st && st !== "ok" ? `<span class="badge ${esc(st)}">${esc(st)}</span>` : "");
}

function timeTip(inst, a, extra = "") {
  const st = inst.status[a];
  const t = st === "TIMEOUT" ? `&gt; ${inst.limit} s` : st === "CRASH" ? "did not finish" : `${fmtTime(inst.times[a])} s`;
  return `<b>${esc(inst.label)}</b><br>${esc(a)}: ${t}${st && st !== "ok" ? ` (${esc(st)})` : ""}${extra}`;
}

// Views ------------------------------------------------------------------------------------------------------------
function panel(title, sub, controls, ...body) {
  return h("section", { class: "panel" },
    h("div", { class: "panel-head" },
      h("div", {}, h("h2", {}, title), sub ? h("p", { class: "muted small" }, sub) : null),
      controls ? h("div", { class: "controls" }, controls) : null),
    ...body);
}

// Collapsible panel, closed by default; the open state survives re-renders
function foldPanel(key, title, ...body) {
  const d = h("details", { class: "panel fold", open: state.c[key] === true },
    h("summary", {}, h("span", { class: "plus", "aria-hidden": "true" }), h("h2", {}, title)),
    h("div", { class: "fold-body" }, ...body));
  d.addEventListener("toggle", () => { state.c[key] = d.open; saveState(); });
  return d;
}

// Charts are drawn once the view is in the document, since only then is the width known
let pendingDraws = [];
function chartBox(draw) {
  const box = h("div", { class: "chart-box" });
  pendingDraws.push(() => draw(box));
  return box;
}

function emptyView() {
  return h("div", { class: "empty" },
    h("h2", {}, "No data"),
    h("p", {}, "After the tests run ", h("code", {}, "python3 results/Viz/plot_results.py"), " and reload the page."));
}

function prose(...paras) {
  return h("div", { class: "prose" }, paras.map(p => typeof p === "string" ? h("p", { html: p }) : p));
}

const ALG_INFO = [
  { name: "Tree", text: "Floyd–Warshall with shortest-path trees: for each pivot it relaxes only along the pivot's tree and skips subtrees that do not improve." },
  { name: "Edge-SCC", text: "Splits the graph into SCCs, solves each SCC with Tree, then relaxes paths between SCCs via every edge that leaves an SCC." },
  { name: "Vertex-SCC", text: "Same SCC split, but relaxes between SCCs via vertices (bridge step and relaxation via pivots, using the pivot's shortest-path tree). For each SCC a cost model picks the cheaper order: bridge → heads or tails → bridge. The bridge step of bridge → heads prunes the sources along in-trees of the tails (Hourglass)." },
  { name: "VertexNaive-SCC", text: "Bridge step, then relaxation via the heads to every reachable target, without the shortest-path tree. Shows how much the tree saves." },
  { name: "Vertex-SCC-reverse", text: "Vertex-SCC that always uses the order tails → bridge, without the cost model." },
  { name: "Vertex-SCC-hourglass", text: "Vertex-SCC-reverse that also prunes the sources in the relaxation via tails (Hourglass): a source only checks the destinations that improved for its parent in the in-tree of the tail." },
  { name: "Vertex-SCC-bridgeHG", text: "Vertex-SCC that always uses the order bridge → heads; the bridge step prunes the sources (Hourglass): a source only checks the heads that improved for its parent in the in-tree of the tail." },
  { name: "Adaptive-SCC", text: "Vertex-SCC with a third choice per SCC: relaxing over edges as in Edge-SCC. The cost model from the paper picks the cheapest of the three." },
];

function viewOverview() {
  return h("div", {},
    panel("Algorithms", null, null,
      h("div", { class: "cards" }, ALG_INFO.map(i => h("div", { class: "card" },
        h("h3", {}, h("span", { class: "sw-dot", style: `background:${colorOf(i.name)}` }), i.name),
        h("p", { class: "small" }, i.text))))),
    panel("Time limit", null, null, prose(
      `Each algorithm gets <b>${state.limit} s</b> per graph` +
      (() => {
        const other = ["gnutella", "snap"].filter(k => limitOf(k) !== state.limit)
          .map(k => `<b>${limitOf(k)} s</b> for ${KIND_NAMES[k]}`);
        return other.length ? ` (${other.join(", ")})` : "";
      })() +
      `. Runs that exceed it are marked <span class="badge TIMEOUT">TIMEOUT</span>.`)));
}

const SORT_OPTIONS = [
  { value: "N", label: "n" }, { value: "label", label: "name" }, { value: "E", label: "m" },
  { value: "largestSCC/N", label: "largest SCC share" }, { value: "sccCount", label: "#SCC" },
];

function viewReal(kind) {
  const ds = state.data[kind];
  if (!ds) return emptyView();
  const algs = visible(ds);
  const sortOpts = SORT_OPTIONS.filter(o => o.value === "label" || ds.instances.some(i => Number.isFinite(i.props[o.value])));
  const controls = [select("sort by", `${kind}-sort`, sortOpts, "N")];
  const sortKey = state.c[`${kind}-sort`] || "N";
  const inst = ds.instances.slice().sort((a, b) => sortKey === "label" ? a.label.localeCompare(b.label, "en", { numeric: true }) : (a.props[sortKey] ?? 0) - (b.props[sortKey] ?? 0));

  const chart = chartBox(box => {
    barChart(box, { cats: inst.map(i => i.label), limit: limitOf(kind), yLabel: "time (s), log", series: algs.map(a => ({
      name: a, color: colorOf(a),
      values: inst.map(i => ({ v: i.status[a] === "CRASH" ? NaN : i.times[a], status: i.status[a],
        tip: timeTip(i, a, `<br>n = ${fmtNum(i.props.N)}, m = ${fmtNum(i.props.E)}${i.props.runs > 1 ? `<br>mean of ${i.props.runs} runs` : ""}`) })) })) });
    box.append(h("div", { class: "legend" },
      algs.map(a => h("span", {}, h("span", { class: "sw", style: `background:${colorOf(a)}` }), a)),
      h("span", {}, h("span", { class: "sw", style: "background: repeating-linear-gradient(45deg, var(--muted) 0 2px, transparent 2px 5px); border: 1px solid var(--muted)" }), `TIMEOUT (> ${limitOf(kind)} s)`),
      h("span", {}, "× = CRASH")));
  });

  const has = key => ds.instances.some(i => Number.isFinite(i.props[key]));
  const propCols = [
    { key: "label", label: "graph", txt: true, get: i => i.label },
    { key: "N", label: "n", get: i => i.props.N, fmt: i => fmtNum(i.props.N) },
    { key: "E", label: "m", get: i => i.props.E, fmt: i => fmtNum(i.props.E) },
    has("sccCount") ? { key: "sccCount", label: "#SCC", get: i => i.props.sccCount, fmt: i => fmtNum(i.props.sccCount) } : null,
    { key: "largestSCCVertices", label: "largest SCC", get: i => i.props.largestSCCVertices, fmt: i => fmtNum(i.props.largestSCCVertices) },
    { key: "largestSCC/N", label: "share", get: i => i.props["largestSCC/N"], fmt: i => fmtRatio(i.props["largestSCC/N"]) },
    has("outgoingEdges") ? { key: "outgoingEdges", label: "|Out|", get: i => i.props.outgoingEdges, fmt: i => fmtNum(i.props.outgoingEdges) } : null,
  ].filter(Boolean);
  const timeCols = algs.map(a => ({ key: "t-" + a, label: a, get: i => (ok(i, a) ? i.times[a] : Infinity), fmt: i => timeCell(i, a),
    best: i => ok(i, a) && i.times[a] === bestTime(i, algs) }));

  return h("div", {},
    kind === "gnutella" ? describeGnutella(ds) : describeSnap(ds),
    panel(`${KIND_NAMES[kind]}: running time`, `${ds.instances.length} graphs · ${state.sources[kind] || ""}`, controls, chart),
    panel("Table", "Click a column header to sort. Bold = fastest." + (ds.instances.some(i => i.props.runs > 1) ? ` Times are means over ${Math.max(...ds.instances.map(i => i.props.runs))} runs.` : ""), null, sortTable(`${kind}-table`, propCols.concat(timeCols), ds.instances, "N")));
}

function distinct(ds, key) {
  return [...new Set(ds.instances.map(i => i.props[key]).filter(Number.isFinite))].sort((a, b) => a - b);
}

const range = (ds, key) => {
  const v = ds.instances.map(i => i.props[key]).filter(Number.isFinite);
  return v.length ? `${fmtNum(Math.min(...v))}–${fmtNum(Math.max(...v))}` : "?";
};

const pctRange = (ds, key) => {
  const v = ds.instances.map(i => i.props[key]).filter(Number.isFinite);
  return v.length ? `${Math.round(100 * Math.min(...v))}–${Math.round(100 * Math.max(...v))} %` : "?";
};

function describeGnutella(ds) {
  return foldPanel("fold-gnutella", "About the graphs", prose(
    "The <b>p2p-Gnutella</b> graphs from the SNAP collection are snapshots of the Gnutella peer-to-peer file-sharing network " +
    "from August 2002. Vertices are hosts and a directed edge (u, v) means that u is connected to v. " +
    "The graph number is the day of the snapshot (e.g. 04 = 4 August).",
    `They have ${range(ds, "N")} vertices and ${range(ds, "E")} edges, are very sparse, and have one large SCC ` +
    `(${pctRange(ds, "largestSCC/N")} of the vertices) and many small ones. The original edges are unweighted, ` +
    "so we assign random weights from [0, 1]."));
}

const SNAP_INFO = [
  [/wiki-?vote/i, "Wikipedia administrator elections (who voted for whom)"],
  [/wiki-?rfa/i, "votes on Wikipedia Requests for Adminship, up to 2013"],
  [/wikielec/i, "Wikipedia administrator elections with individual votes"],
  [/email/i, "e-mail between members of a European research institution (u sent a message to v)"],
  [/bitcoinalpha/i, "trust ratings between users of the Bitcoin Alpha platform"],
  [/bitcoinotc/i, "trust ratings between users of the Bitcoin OTC platform"],
  [/cit-?hep/i, "citations between arXiv hep-th papers (paper u cites v)"],
];

function describeSnap(ds) {
  const items = ds.instances.map(i => {
    const info = SNAP_INFO.find(([re]) => re.test(i.label));
    return h("li", {}, h("b", {}, i.label), ": ", info ? info[1] : "graph from the SNAP collection",
      h("span", { class: "muted" }, ` (n = ${fmtNum(i.props.N)}, m = ${fmtNum(i.props.E)})`));
  });
  return foldPanel("fold-snap", "About the graphs", prose(
    "Real directed graphs from the <b>SNAP</b> collection (Stanford Network Analysis Project). Vertex ids are remapped to 0..n−1, " +
    "self-loops are removed and edges get random weights from [0, 1]. The bitcoin graphs use their trust rating r ∈ [−10, 10] " +
    "instead: weight (10 − r) / 20, so distrust (−10) costs 1 and full trust (+10) costs 0. For wiki votes the sign is ignored.",
    h("ul", {}, items),
    "A graph with n vertices needs about 12·n² bytes (three n × n matrices), so on 8 GB of RAM graphs with more than ~30,000 vertices " +
    "swap and their times are not reliable."));
}

function runsPerCombo(ds) {
  const keys = ["N", "sccCount", "outProb", "inProb", "componentProb"];
  const combos = new Set(ds.instances.map(i => keys.map(k => i.props[k]).join("|")));
  return Math.round(ds.instances.length / Math.max(1, combos.size));
}

function describeRandom(ds, kind) {
  const list = key => distinct(ds, key).map(fmtTick).join(", ");
  return foldPanel(`fold-${kind}`, "How the graphs are generated", prose(
    `Each graph has n = ${distinct(ds, "N").map(fmtNum).join(", ")} vertices and exactly k strongly connected components ` +
    `(k ∈ {${[...new Set(ds.instances.map(i => i.props.kType))].sort((a, b) => (isNaN(a) - isNaN(b)) || (a < b ? -1 : 1)).join(", ")}}; ` +
    "log n is ⌊log₂ n⌋ and √n is ⌊√n⌋). " +
    (kind === "equal" ? "All components have the same size: n/k vertices." :
      "Component sizes are random: k − 1 random cuts split the vertices.") +
    " The vertex order is shuffled, so a component is not a block of the matrix.",
    h("ul", {},
      h("li", { html: "<b>Inside a component</b>: a directed cycle through all its vertices (makes it strongly connected), then every missing edge is added with probability <b>inProb</b>." }),
      h("li", { html: kind === "star"
        ? "<b>Between components</b>: each pair i &lt; j is connected with probability <b>componentProb</b>; if it is, either one tail of i (the same for all its pairs) gets an edge to every vertex of j, or every vertex of i gets an edge to one random head of j (chosen at random for each pair)."
        : "<b>Between components</b>: each pair i &lt; j is connected with probability <b>componentProb</b>; if it is, every possible edge from i to j is added with probability <b>outProb</b>." }),
      h("li", { html: "Edges between components only go forward (i → j, i &lt; j), so components never merge and the component graph is a DAG. Weights are random from [0, 1]." })),
    `Parameters: ${kind === "star" ? "" : `outProb ∈ {${list("outProb")}}, `}inProb ∈ {${list("inProb")}}, componentProb ∈ {${distinct(ds, "componentProb").map(compLabel).join(", ")}}. ` +
    `Each combination is run ${runsPerCombo(ds)}× on different random graphs, ${ds.instances.length} instances in total.`));
}

function viewRandom(kind) {
  const ds = state.data[kind];
  if (!ds) return emptyView();
  const algs = visible(ds);
  const ns = distinct(ds, "N"), ins = distinct(ds, "inProb"), comps = distinct(ds, "componentProb");
  // x axis: outProb, or the number of SCCs k for star graphs (they have no outProb)
  const xKey = kind === "star" ? "sccCount" : "outProb", xName = kind === "star" ? "k (#SCC)" : "outProb";
  const outs = distinct(ds, xKey);
  // the number of components depends on n (log n, √n), so the grid and the lines show one n at a time
  const nSel = key => select("n", key, opt(ns), ns[0]);
  const ksOf = n => [...new Set(ds.instances.filter(i => i.props.N === n).map(i => i.props.sccCount))].sort((a, b) => a - b);
  const all = { value: "all", label: "all (geo. mean)" };
  const opt = vals => vals.map(v => ({ value: v, label: fmtTick(v) }));
  const match = (i, key, cval) => cval === "all" || i.props[key] === Number(cval);
  const view = h("div");
  const mode = segmented(`${kind}-rnd-view`, [{ value: "grid", label: "Grid" }, { value: "lines", label: "Lines" }, { value: "table", label: "Table" }], "grid");
  view.append(describeRandom(ds, kind));
  view.append(h("div", { class: "controls", style: "margin-top:12px" }, mode,
    h("span", { class: "muted small" }, `${ds.instances.length} instances · n = ${distinct(ds, "N").map(fmtNum).join(", ")} · ${state.sources[kind] || ""}`)));

  // Time vs. outProb for one set of instances, as geometric means per outProb
  const seriesFor = sel => algs.map(a => ({ name: a, color: colorOf(a), points: outs.map(o => {
    const pts = sel.filter(i => i.props[xKey] === o);
    const good = pts.filter(i => ok(i, a));
    const y = geomean(good.map(i => i.times[a]));
    const fails = pts.length - good.length;
    return { x: o, y, tip: `<b>${esc(a)}</b><br>${xName} = ${o}<br>${fmtTime(y)} s (geo. mean of ${good.length} instances)${fails ? `<br>${fails}× TIMEOUT/CRASH` : ""}` };
  }) }));
  const legend = () => h("div", { class: "legend" }, algs.map(a => h("span", {}, h("span", { class: "sw", style: `background:${colorOf(a)}` }), a)));

  if (state.c[`${kind}-rnd-view`] === "grid") {
    const controls = [
      nSel(`${kind}-rg-n`),
      select("inProb", `${kind}-rg-in`, [all, ...opt(ins)], "all"),
      checkbox("log y axis", `${kind}-rg-ylog`, true),
      select("y axis", `${kind}-rg-yscale`, [
        { value: "row", label: "same per row" }, { value: "shared", label: "same for all" }, { value: "own", label: "own per chart" }], "row"),
    ];
    const ylog = state.c[`${kind}-rg-ylog`];
    const n = Number(state.c[`${kind}-rg-n`]), ks = kind === "star" ? [null] : ksOf(n); // null: all k
    const cells = ks.flatMap(k => comps.map(comp => {
      const sel = ds.instances.filter(i => i.props.N === n && (k === null || i.props.sccCount === k) && i.props.componentProb === comp && match(i, "inProb", state.c[`${kind}-rg-in`]));
      const tipTitle = `n = ${fmtNum(n)}, ${k === null ? "all k" : kLabel(k, n)}, componentProb = ${compLabel(comp)}`;
      const series = seriesFor(sel);
      series.forEach(sr => sr.points.forEach(p => { p.tip = `<b>${tipTitle}</b><br>` + p.tip; }));
      return { k, n, comp, series };
    }));
    // y range fitted to the data: of one row (same k), of the whole grid or of every chart on its own
    const ysOf = cs => cs.flatMap(c => c.series.flatMap(sr => sr.points.map(p => p.y)));
    const yscale = state.c[`${kind}-rg-yscale`];
    const shared = yscale === "shared" ? extent(ysOf(cells), ylog) : null;
    const rowDomain = new Map(ks.map(k => [k, extent(ysOf(cells.filter(c => c.k === k)), ylog)]));
    const yDomainOf = c => yscale === "row" ? rowDomain.get(c.k) : shared;
    view.append(panel(kind === "star" ? "Time vs. k: componentProb" : "Time vs. outProb: k × componentProb",
      `${kind === "star" ? "" : "Rows: number of SCCs k. "}Columns: componentProb. Each point is the geometric mean over the instances with these parameters. A missing point means every instance exceeded the limit.`, controls,
      h("div", { class: "facet-grid", style: `grid-template-columns: repeat(${comps.length}, minmax(0, 1fr))` },
        cells.map(c => h("div", { class: "facet" },
          h("div", { class: "facet-head" }, `${c.k === null ? `n = ${fmtNum(c.n)}` : kLabel(c.k, c.n)} · componentProb = ${compLabel(c.comp)}`),
          chartBox(box => lineChart(box, { series: c.series, xlog: true, ylog, xLabel: xName, yLabel: "time (s)", xTicks: outs, height: 300, yDomain: yDomainOf(c) }))))),
      legend()));
  } else if (state.c[`${kind}-rnd-view`] === "lines") {
    const nControl = nSel(`${kind}-rl-n`); // validates state.c[`${kind}-rl-n`] before it is read
    const n = Number(state.c[`${kind}-rl-n`]), ks = ksOf(n);
    const controls = [
      nControl,
      kind === "star" ? null : select("k (#SCC)", `${kind}-rl-k`, [all, ...ks.map(k => ({ value: k, label: kLabel(k, n).slice(4) }))], ks[0] ?? "all"),
      select("inProb", `${kind}-rl-in`, [all, ...opt(ins)], "all"),
      select("componentProb", `${kind}-rl-comp`, [all, ...comps.map(v => ({ value: v, label: compLabel(v) }))], "all"),
      checkbox("log y axis", `${kind}-rl-ylog`, true),
    ];
    const sel = ds.instances.filter(i => i.props.N === n && (kind === "star" || match(i, "sccCount", state.c[`${kind}-rl-k`])) && match(i, "inProb", state.c[`${kind}-rl-in`]) && match(i, "componentProb", state.c[`${kind}-rl-comp`]));
    const series = seriesFor(sel);
    view.append(panel(`Time vs. ${xName}`, `Each point is the geometric mean over all instances with the selected parameters. A missing point means every instance exceeded the limit.`, controls,
      chartBox(box => {
        lineChart(box, { series, xlog: true, ylog: state.c[`${kind}-rl-ylog`], xLabel: xName, yLabel: "time (s)", xTicks: outs });
        box.append(legend());
      })));
  } else {
    const controls = [select("n", `${kind}-rt-n`, [{ value: "all", label: "all" }, ...opt(ns)], "all")];
    const rows = ds.instances.filter(i => match(i, "N", state.c[`${kind}-rt-n`]));
    const cols = [
      { key: "N", label: "n", get: i => i.props.N, fmt: i => fmtNum(i.props.N) },
      { key: "sccCount", label: "k", get: i => i.props.sccCount, fmt: i => kLabel(i.props.sccCount, i.props.N).slice(4) },
      { key: "outProb", label: "outProb", get: i => i.props.outProb },
      { key: "inProb", label: "inProb", get: i => i.props.inProb },
      { key: "componentProb", label: "comp", get: i => i.props.componentProb, fmt: i => compLabel(i.props.componentProb) },
      { key: "E", label: "m", get: i => i.props.E, fmt: i => fmtNum(i.props.E) },
      { key: "outgoingEdges", label: "|Out|", get: i => i.props.outgoingEdges, fmt: i => fmtNum(i.props.outgoingEdges) },
      { key: "tails", label: "tails", get: i => i.props.tails, fmt: i => fmtNum(i.props.tails) },
      { key: "heads", label: "heads", get: i => i.props.heads, fmt: i => fmtNum(i.props.heads) },
      { key: "largestSCCVertices", label: "largest SCC", get: i => i.props.largestSCCVertices, fmt: i => fmtNum(i.props.largestSCCVertices) },
      ...algs.map(a => ({ key: "t-" + a, label: a, get: i => (ok(i, a) ? i.times[a] : Infinity), fmt: i => timeCell(i, a),
        best: i => ok(i, a) && i.times[a] === bestTime(i, algs) })),
    ];
    view.append(panel("All instances", `${rows.length} rows. Click a column header to sort. Bold = fastest.`, controls,
      sortTable("rnd-table", cols, rows, "sccCount")));
  }
  return view;
}

// Rendering --------------------------------------------------------------------------------------------------------
function renderAlgChips() {
  const box = document.getElementById("algs");
  box.replaceChildren();
  const algs = allAlgs();
  if (!algs.length) return;
  box.append(h("span", { class: "label" }, "Algorithms:"));
  for (const a of algs) {
    const off = state.hidden.has(a);
    box.append(h("button", { type: "button", class: "chip" + (off ? " off" : ""), "aria-pressed": String(!off),
      title: off ? "Show" : "Hide",
      onclick: () => { off ? state.hidden.delete(a) : state.hidden.add(a); saveState(); render(); } },
      h("span", { class: "dot", style: `background:${colorOf(a)}` }), a));
  }
}

function render() {
  tip.hidden = true;
  document.querySelectorAll("#tabs button").forEach(b => {
    const k = b.dataset.tab;
    b.setAttribute("aria-selected", String(k === state.tab));
    const n = state.data[k]?.instances.length;
    b.querySelector(".count")?.remove();
    if (n) b.append(h("span", { class: "count" }, n));
  });
  document.getElementById("source").textContent = state.generated ? `Data generated ${state.generated}` : "";
  renderAlgChips();
  const view = document.getElementById("view");
  pendingDraws = [];
  view.replaceChildren(state.tab === "overview" ? viewOverview() : RANDOM_KINDS.includes(state.tab) ? viewRandom(state.tab)
    : state.tab === "pdfs" ? viewPdfs() : viewReal(state.tab));
  for (const draw of pendingDraws) draw();
  pendingDraws = [];
}

// Drawn graphs and figures (PDF files next to the dashboard, listed in data.js) ----------------------------------------
const PDF_INFO = {
  "dag_graphviz_all.pdf": "Component graphs of the Random SCC test (run2.py): a node is an SCC with its size, an edge label is the number of edges between two SCCs.",
  "dag_graphviz_equal.pdf": "Component graphs of the Equal SCC test (run2.py --equal).",
  "dag_graphviz_star.pdf": "Component graphs of the Star SCC test (run2.py --star).",
  "graph.pdf": "One generated graph drawn vertex by vertex (draw_graph.py).",
};

function viewPdfs() {
  const pdfs = state.pdfs;
  if (!pdfs.length) return h("div", { class: "empty" },
    h("h2", {}, "No PDF files"),
    h("p", {}, "Draw the graphs (", h("code", {}, "python3 results/Viz/SCC/run2.py"), "), then run ",
      h("code", {}, "python3 results/Viz/plot_results.py --no-figures"), " and reload the page."));
  const groups = [...new Set(pdfs.map(p => p.group))];
  const options = groups.flatMap(g => pdfs.filter(p => p.group === g).map(p => ({ value: p.path, label: `${g}: ${p.name}` })));
  const control = select("file", "pdf-file", options, options[0].value);
  const cur = pdfs.find(p => p.path === state.c["pdf-file"]) || pdfs[0];
  return h("div", {},
    panel("Graph PDFs", PDF_INFO[cur.name] || null,
      [control, h("a", { class: "btn", href: cur.path, target: "_blank", rel: "noopener" }, "Open in new tab")],
      h("iframe", { class: "pdf-frame", src: cur.path, title: cur.name })),
    h("p", { class: "muted small" }, "The list is refreshed by plot_results.py; the files themselves are read when you open them."));
}

// Time limit of a test; Gnutella can have its own (GNUTELLA_INSTANCE_TIME)
const limitOf = kind => state.limits[kind] || state.limit;

// Loading ----------------------------------------------------------------------------------------------------------
function loadText(text, name, key) {
  const kind = LOADERS[key] ? key : detectKind(readRows(text)); // equal and star have the same columns as random
  if (!kind) { console.warn(`${name}: unknown CSV format`); return; }
  const limit = limitOf(kind);
  const ds = LOADERS[kind](readRows(text), limit);
  if (!ds.instances.length) { console.warn(`${name}: no result rows`); return; }
  for (const i of ds.instances) i.limit = limit;
  state.data[kind] = ds;
  state.sources[kind] = name;
}

function init() {
  restoreState();
  const hashTab = location.hash.slice(1); // e.g. index.html#snap opens the SNAP tab
  if (TABS.includes(hashTab)) state.tab = hashTab;
  if (!TABS.includes(state.tab)) state.tab = "overview";
  const D = window.APSP_DATA;
  if (D) {
    state.limit = D.maxTime || 60;
    state.limits = D.limits || {};
    state.generated = D.generated || null;
    for (const [key, f] of Object.entries(D.files || {})) loadText(f.text, f.name, key);
    state.pdfs = D.pdfs || [];
  }

  document.getElementById("tabs").addEventListener("click", e => {
    const b = e.target.closest("button[data-tab]");
    if (!b) return;
    state.tab = b.dataset.tab;
    saveState();
    render();
  });

  let timer = null, lastW = innerWidth;
  addEventListener("resize", () => {
    if (innerWidth === lastW) return;
    lastW = innerWidth;
    clearTimeout(timer);
    timer = setTimeout(render, 150);
  });
  render();
}

init();
