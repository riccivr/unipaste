#!/usr/bin/env python3
import json
import subprocess
import time
import os

CAST_FILE = "assets/demo.cast"
GIF_FILE = "assets/demo.gif"

os.makedirs("assets", exist_ok=True)

# Sample rich HTML datasets
DATA_TABLE = """<!DOCTYPE html>
<html>
<body>
<h2>Production Cluster Performance - Q3 Summary</h2>
<table border="1">
  <thead>
    <tr>
      <th>Cluster Region</th>
      <th>Nodes</th>
      <th>Throughput (req/s)</th>
      <th>P99 Latency</th>
      <th>Error Rate</th>
      <th>Status</th>
    </tr>
  </thead>
  <tbody>
    <tr>
      <td><b>us-east-1</b> (N. Virginia)</td>
      <td>128</td>
      <td>2,450,000</td>
      <td>4.2 ms</td>
      <td>0.001%</td>
      <td>[ACTIVE]</td>
    </tr>
    <tr>
      <td><b>us-west-2</b> (Oregon)</td>
      <td>96</td>
      <td>1,820,000</td>
      <td>3.8 ms</td>
      <td>0.000%</td>
      <td>[ACTIVE]</td>
    </tr>
    <tr>
      <td><b>eu-central-1</b> (Frankfurt)</td>
      <td>84</td>
      <td>1,490,000</td>
      <td>5.1 ms</td>
      <td>0.002%</td>
      <td>[ACTIVE]</td>
    </tr>
    <tr>
      <td><b>ap-northeast-1</b> (Tokyo)</td>
      <td>64</td>
      <td>980,000</td>
      <td>4.9 ms</td>
      <td>0.000%</td>
      <td>[ACTIVE]</td>
    </tr>
    <tr>
      <td><b>sa-east-1</b> (São Paulo)</td>
      <td>32</td>
      <td>410,000</td>
      <td>8.4 ms</td>
      <td>0.005%</td>
      <td>[STANDBY]</td>
    </tr>
  </tbody>
</table>
</body>
</html>"""

DATA_CHAT = """<!DOCTYPE html>
<html>
<body>
<h3>v2.4 Release Coordination & Checklist</h3>
<p>Meeting notes from sync with <b>@devops</b> and <b>@backend</b> teams (<a href="https://github.com/org/project/issues/142">#142</a>):</p>

<h4>Deployment Tasks</h4>
<ul>
  <li><input type="checkbox" checked> Run automated database migrations on primary cluster</li>
  <li><input type="checkbox" checked> Warm edge CDN caches for static API assets</li>
  <li><input type="checkbox"> Switch ingress traffic from Blue to Green deployment</li>
  <li>
    <ul>
      <li><input type="checkbox"> Verify P99 response time stays under 10ms</li>
      <li><input type="checkbox"> Monitor error logs for connection pooling anomalies</li>
    </ul>
  </li>
  <li><input type="checkbox"> Broadcast release notes to engineering channel</li>
</ul>

<blockquote>
  <p><i>"The zero-downtime traffic switchover script is prepared in the ops repository."</i></p>
</blockquote>

<h4>Health Check Probe</h4>
<pre><code class="language-bash"># Verify health check endpoint across all pods
curl -fsSL https://api.internal/healthz | jq '.status, .active_workers'
</code></pre>
</body>
</html>"""

DATA_DOC = """<!DOCTYPE html>
<html>
<body>
<h2>Distributed Consensus Protocol Overview</h2>
<p>Modern distributed systems rely on state machine replication protocols such as <a href="https://raft.github.io/">Raft</a> and <a href="https://en.wikipedia.org/wiki/Paxos_(computer_science)">Paxos</a> to guarantee linearizable consistency across unreliable network partitions.</p>

<h3>Key Protocol Attributes</h3>
<table border="1">
  <tr><th>Algorithm</th><th>Leader Model</th><th>Fault Tolerance</th><th>Complexity</th></tr>
  <tr><td><b>Raft</b></td><td>Strong Leader</td><td>(N-1)/2 failures</td><td>Understandable</td></tr>
  <tr><td><b>Multi-Paxos</b></td><td>Weak / Multi-Leader</td><td>(N-1)/2 failures</td><td>High</td></tr>
  <tr><td><b>EPaxos</b></td><td>Leaderless</td><td>(N-1)/2 failures</td><td>Very High</td></tr>
</table>

<p>For more architectural details, refer to the <a href="https://etcd.io/docs/">etcd documentation</a> and the <a href="https://pdos.csail.mit.edu/6.824/">MIT 6.824 Distributed Systems course</a>.</p>
</body>
</html>"""

# Write temporary files outside the repo
with open("/tmp/metrics.html", "w") as f:
    f.write(DATA_TABLE)

with open("/tmp/release_notes.html", "w") as f:
    f.write(DATA_CHAT)

with open("/tmp/consensus.html", "w") as f:
    f.write(DATA_DOC)

events = []
current_time = 0.0

def add_event(delta, text):
    global current_time
    current_time += delta
    events.append([round(current_time, 3), "o", text])

def type_command(cmd, prompt="riccivr@workstation:~$ ", typing_speed=0.03):
    add_event(0.2, prompt)
    for ch in cmd:
        add_event(typing_speed, ch)
    add_event(0.15, "\r\n")

def run_and_record(cmd_display, shell_cmd, prompt="riccivr@workstation:~$ ", pause_after=2.0):
    type_command(cmd_display, prompt)
    proc = subprocess.run(shell_cmd, shell=True, capture_output=True, text=True)
    out = proc.stdout
    # Normalize newlines to CRLF for terminal
    out_crlf = out.replace("\r\n", "\n").replace("\n", "\r\n")
    add_event(0.05, out_crlf)
    if not out_crlf.endswith("\r\n"):
        add_event(0.01, "\r\n")
    add_event(pause_after, "")

# Initial clear screen and prompt
add_event(0.0, "\x1b[2J\x1b[H")

# 1. Unicode formatted table with high data density
run_and_record("unipaste -u /tmp/metrics.html", "unipaste -u /tmp/metrics.html", pause_after=2.2)

# 2. Rich Slack/Teams checklist, nested tasks, code block, and quote
run_and_record("cat /tmp/release_notes.html | unipaste", "cat /tmp/release_notes.html | unipaste", pause_after=2.5)

# 3. Convert HTML document to GitHub-Flavored Markdown
run_and_record("unipaste -m markdown /tmp/consensus.html", "unipaste -m markdown /tmp/consensus.html", pause_after=2.2)

# 4. Convert document with footnote link bibliographies
run_and_record("unipaste -l footnote /tmp/consensus.html", "unipaste -l footnote /tmp/consensus.html", pause_after=2.5)

# Final prompt
add_event(0.2, "riccivr@workstation:~$ ")
add_event(1.5, "")

header = {
    "version": 2,
    "width": 94,
    "height": 34,
    "timestamp": int(time.time()),
    "env": {"SHELL": "/bin/bash", "TERM": "xterm-256color"},
    "title": "unipaste demo"
}

with open(CAST_FILE, "w") as f:
    f.write(json.dumps(header) + "\n")
    for ev in events:
        f.write(json.dumps(ev) + "\n")

print(f"Generated {CAST_FILE} ({len(events)} frames, duration {round(current_time, 2)}s)")

# Render to high-quality animated GIF using agg
cmd = f"agg --theme monokai --font-size 15 --fps-cap 30 {CAST_FILE} {GIF_FILE}"
print(f"Running: {cmd}")
subprocess.run(cmd, shell=True, check=True)
print(f"Rendered {GIF_FILE} successfully!")
