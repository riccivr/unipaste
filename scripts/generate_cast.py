import json, subprocess, os

def run_unipaste(args, input_str):
    p = subprocess.Popen(["./unipaste"] + args, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    out, err = p.communicate(input=input_str)
    return out

events = []
t = 0.0

def emit(text, delay=0.03):
    global t
    t += delay
    events.append([round(t, 2), "o", text])

def emit_section(title):
    global t
    t += 0.4
    banner = f"\r\n\x1b[1;36m━━━ {title} ━━━\x1b[0m\r\n"
    emit(banner, 0.1)

def type_cmd(cmd, post_delay=0.3):
    global t
    prompt = "\x1b[1;32mriccivr@workstation\x1b[0m:\x1b[1;34m~\x1b[0m$ "
    emit(prompt, 0.15)
    for c in cmd:
        emit(c, 0.025)
    emit("\r\n", 0.1)
    t += post_delay

emit("\x1b[2J\x1b[H", 0.0)

# Section 1: Introduction & Version
emit_section("1. unipaste 1.2.0 Universal Rich Text & Stream Converter")
type_cmd("unipaste -v")
out = run_unipaste(["-v"], "")
for line in out.strip().split("\n"):
    emit(line + "\r\n", 0.03)

# Section 2: Core HTML Formatting (Headings, Lists, Checkboxes, Code)
emit_section("2. Rich Formatting: Headings, Task Lists & Code Blocks")
type_cmd("cat sprint.html | unipaste")
sample_core = """<h2>Sprint 42 Deliverables</h2>
<p>High priority updates for <b>production</b> and <i>staging</i> clusters:</p>
<ul>
  <li><input type="checkbox" checked> Deploy zero-downtime database migration</li>
  <li><input type="checkbox"> Scale Kubernetes pods to 16 replicas</li>
</ul>
<pre><code class="language-rust">fn main() {
    println!("Zero dependencies, pure C99!");
}</code></pre>"""
out = run_unipaste([], sample_core)
for line in out.strip().split("\n"):
    emit(line + "\r\n", 0.025)

# Section 3: Tables - Unicode Box, Markdown & ASCII Grids
emit_section("3. Tables: Unicode Box Drawing (-u) & Markdown Pipes")
type_cmd("cat metrics.html | unipaste -u")
sample_table = """<table>
  <thead><tr><th>Service</th><th>Latency (p99)</th><th>Status</th></tr></thead>
  <tbody>
    <tr><td>auth-api</td><td>14.2 ms</td><td>Healthy</td></tr>
    <tr><td>payment-gateway</td><td>28.5 ms</td><td>Healthy</td></tr>
    <tr><td>worker-pool</td><td>142.0 ms</td><td>Degraded</td></tr>
  </tbody>
</table>"""
out = run_unipaste(["-u"], sample_table)
for line in out.strip().split("\n"):
    emit(line + "\r\n", 0.025)

# Section 4: Raw TSV Spreadsheet Auto-Conversion
emit_section("4. Raw TSV Ingestion (Excel & Google Sheets clipboard)")
type_cmd("cat spreadsheet.tsv | unipaste -m markdown")
sample_tsv = "Product\tRegion\tQ1 Revenue\tGrowth\nCloud Pro\tUS-East\t$128,450\t+34.2%\nEnterprise\tEU-West\t$240,100\t+18.9%\nEdge Node\tAP-South\t$84,300\t+52.1%\n"
out = run_unipaste(["-m", "markdown"], sample_tsv)
for line in out.strip().split("\n"):
    emit(line + "\r\n", 0.025)

# Section 5: Telemetry & URL Tracking Stripper
emit_section("5. Privacy: Automatic URL Tracking & Telemetry Stripper")
type_cmd("cat marketing_link.html | unipaste")
sample_tracking = '<p>Check out our docs on <a href="https://github.com/riccivr/unipaste?utm_source=linkedin&utm_medium=feed&fbclid=IwAR3x&rcm=ACoAA#get-started">GitHub Docs</a>!</p>'
out = run_unipaste([], sample_tracking)
for line in out.strip().split("\n"):
    emit(line + "\r\n", 0.025)

# Section 6: MathML & KaTeX LaTeX Extraction
emit_section("6. Math: KaTeX / MathML LaTeX Formula Extraction")
type_cmd("cat formula.html | unipaste")
sample_math = '<p>Mass-energy is <span class="katex"><math><semantics><annotation encoding="application/x-tex">E = mc^2</annotation></semantics></math></span> and the Euler-Poisson integral:</p><span class="katex-display"><math display="block"><semantics><annotation encoding="application/x-tex">\\int_{-\\infty}^{\\infty} e^{-x^2} dx = \\sqrt{\\pi}</annotation></semantics></math></span>'
out = run_unipaste([], sample_math)
for line in out.strip().split("\n"):
    emit(line + "\r\n", 0.025)

# Section 7: Dialect Output Modes (Slack & Jira)
emit_section("7. Dialect Targets: Slack / Discord & Jira Wiki Markup")
type_cmd("cat announcement.html | unipaste -m slack")
sample_dialect = '<h3>Deployment Complete</h3><p>Version <b>v1.2.0</b> deployed. See <a href="https://ops.corp.internal">Ops Dashboard</a>.</p>'
out = run_unipaste(["-m", "slack"], sample_dialect)
for line in out.strip().split("\n"):
    emit(line + "\r\n", 0.025)

type_cmd("cat announcement.html | unipaste -m jira")
out = run_unipaste(["-m", "jira"], sample_dialect)
for line in out.strip().split("\n"):
    emit(line + "\r\n", 0.025)

# Section 8: Link Footnote Style (-l footnote)
emit_section("8. Footnote References (-l footnote)")
type_cmd("cat article.html | unipaste -l footnote")
sample_fn = '<p>Built with <a href="https://github.com/riccivr/unipaste">unipaste</a> and paired with <a href="https://github.com/riccivr/clipbridge">clipbridge</a>.</p>'
out = run_unipaste(["-l", "footnote"], sample_fn)
for line in out.strip().split("\n"):
    emit(line + "\r\n", 0.025)

emit("\r\n\x1b[1;32mriccivr@workstation\x1b[0m:\x1b[1;34m~\x1b[0m$ ", 0.5)

header = {
    "version": 2,
    "width": 94,
    "height": 38,
    "timestamp": 1788100000,
    "env": {"SHELL": "/bin/bash", "TERM": "xterm-256color"},
    "title": "unipaste 1.2.0 feature showcase"
}

with open("assets/demo.cast", "w", encoding="utf-8") as f:
    f.write(json.dumps(header) + "\n")
    for event in events:
        f.write(json.dumps(event) + "\n")
print(f"Generated assets/demo.cast ({t:.1f}s, {len(events)} events)")