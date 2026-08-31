import json, subprocess, os

def run_unipaste(args, input_str):
    p = subprocess.Popen(["./unipaste"] + args, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    out, err = p.communicate(input=input_str)
    return out

events = []
t = 0.0

def emit(text, delay=0.04):
    global t
    t += delay
    events.append([round(t, 2), "o", text])

def type_cmd(cmd, post_delay=0.35):
    global t
    prompt = "\x1b[1;32mriccivr@workstation\x1b[0m:\x1b[1;34m~\x1b[0m$ "
    emit(prompt, 0.2)
    for c in cmd:
        emit(c, 0.03)
    emit("\r\n", 0.12)
    t += post_delay

emit("\x1b[2J\x1b[H", 0.0)

# 1. Version Check
type_cmd("unipaste -v")
out = run_unipaste(["-v"], "")
for line in out.strip().split("\n"):
    emit(line + "\r\n", 0.04)

# 2. Table with Unicode box borders
type_cmd("curl -s https://api.internal/metrics | unipaste -u")
sample_table = "<h3>Cluster Performance</h3><table><thead><tr><th>Node</th><th>CPU</th><th>Status</th></tr></thead><tbody><tr><td>node-01</td><td>12.4%</td><td>Healthy</td></tr><tr><td>node-02</td><td>89.1%</td><td>High Load</td></tr><tr><td>node-03</td><td>4.2%</td><td>Healthy</td></tr></tbody></table>"
out = run_unipaste(["-u"], sample_table)
for line in out.strip().split("\n"):
    emit(line + "\r\n", 0.03)

# 3. URL Tracking Stripper
type_cmd("xclip -o | unipaste -m markdown  # Automatic URL tracking stripper")
sample_url = '<p>Check release notes on <a href="https://github.com/riccivr/unipaste?utm_source=linkedin&utm_medium=social&fbclid=IwAR3x#features">GitHub Releases</a>!</p>'
out = run_unipaste(["-m", "markdown"], sample_url)
for line in out.strip().split("\n"):
    emit(line + "\r\n", 0.03)

# 4. Slack & Jira Dialects
type_cmd("pbpaste | unipaste -m slack     # Slack / Discord mrkdwn")
sample_rich = '<h2>Sprint Planning</h2><p>Review <b>Q3 Roadmap</b> with <i>backend team</i>. Ref: <a href="https://jira.corp.net">Dashboard</a></p><pre><code class="language-python">def deploy(): pass</code></pre>'
out = run_unipaste(["-m", "slack"], sample_rich)
for line in out.strip().split("\n"):
    emit(line + "\r\n", 0.03)

type_cmd("pbpaste | unipaste -m jira      # Jira / Confluence wiki")
out = run_unipaste(["-m", "jira"], sample_rich)
for line in out.strip().split("\n"):
    emit(line + "\r\n", 0.03)

# 5. KaTeX / MathML LaTeX Extraction
type_cmd("cat formula.html | unipaste -m markdown")
sample_math = '<p>Mass-energy is <span class="katex"><math><semantics><annotation encoding="application/x-tex">E = mc^2</annotation></semantics></math></span> and integral:</p><span class="katex-display"><math display="block"><semantics><annotation encoding="application/x-tex">\\int_{-\\infty}^{\\infty} e^{-x^2} dx = \\sqrt{\\pi}</annotation></semantics></math></span>'
out = run_unipaste(["-m", "markdown"], sample_math)
for line in out.strip().split("\n"):
    emit(line + "\r\n", 0.03)

# 6. Raw TSV Ingestion
type_cmd("cat data.tsv | unipaste -m markdown  # Auto-detects Excel & Google Sheets TSV")
sample_tsv = "Product\tStock\tPrice\nMacBook M3\t14\t$1,499.00\nThinkPad X1\t28\t$1,249.00\nDell XPS 15\t9\t$1,399.00\n"
out = run_unipaste(["-m", "markdown"], sample_tsv)
for line in out.strip().split("\n"):
    emit(line + "\r\n", 0.03)

emit("\x1b[1;32mriccivr@workstation\x1b[0m:\x1b[1;34m~\x1b[0m$ ", 0.5)

header = {
    "version": 2,
    "width": 92,
    "height": 38,
    "timestamp": 1788100000,
    "env": {"SHELL": "/bin/bash", "TERM": "xterm-256color"},
    "title": "unipaste 1.2.0 showcase"
}

with open("assets/demo.cast", "w", encoding="utf-8") as f:
    f.write(json.dumps(header) + "\n")
    for event in events:
        f.write(json.dumps(event) + "\n")
print(f"Done: assets/demo.cast ({t:.1f}s, {len(events)} events)")