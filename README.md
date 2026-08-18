unipaste - suckless universal rich text converter and stream formatter
========================================================================
`unipaste` is a lightweight, zero-dependency POSIX CLI utility that parses rich
text, HTML fragments, and clipboard payloads, converting them into beautifully
formatted plain text, Markdown, or terminal output.

When copying content from Slack, Microsoft Teams, Discord, Google Chrome, or
web applications, pasting into plain-text destinations (Notepad.exe, nano, vim,
terminals, code editors) produces flattened, mangled garbage: tables collapse
into unreadable blobs, code blocks lose formatting, lists lose hierarchy, and
hyperlink URLs vanish.

`unipaste` reconstructs proper layout, alignments, and typography directly in
pure POSIX C99 with zero external libraries.

Features
--------
* **Zero external dependencies**: Strict C99 and standard POSIX libc headers (`stdio.h`, `stdlib.h`, `string.h`, `ctype.h`).
* **Suckless philosophy**: Minimal code footprint, clean tabbed formatting, fast linear streaming parser, simple `arg.h`.
* **Dynamic table formatter**: Automatically calculates column widths and renders clean ASCII box tables (`+---+---+`), Unicode grid tables (`┌─┬─┐`), Markdown pipe tables (`| a | b |`), or TSV.
* **List & task hierarchy**: Formats nested unordered lists (`* `, `  * `), numbered lists (`1. `, `2. `), and task checkboxes (`[ ]`, `[x]`).
* **Code block preservation**: Preserves code formatting, syntax language tags (```` ```lang ````), indentation, and newlines without reflowing.
* **Link fidelity**: Configurable hyperlink preservation (bracketed `Title (url)`, markdown `[Title](url)`, text-only, or footnote `[1]` references).
* **HTML entity decoder**: Comprehensive decoder for named (`&amp;`, `&quot;`, `&mdash;`, `&copy;`, etc.) and numeric (`&#160;`, `&#x2014;`) entities.
* **Windows CF_HTML aware**: Automatically recognizes and strips Windows clipboard `Version:0.9` / `<!--StartFragment-->` headers.
* **Compile-time plugin architecture**: Optional allowlist-based HTML sanitization pre-pass without compromising the zero-dependency default.
* **Memory safe**: Tested with AddressSanitizer and UndefinedBehaviorSanitizer.

Requirements
------------
In order to build `unipaste` you need a C99 compiler and `make`.

Installation & Setup
--------------------

### Option A: The "You Do It" / Suckless DIY Approach (Recommended)
True to suckless software philosophy, `unipaste` is 100% dependency-free, standard C99, and builds in milliseconds:

```sh
git clone https://github.com/riccivr/unipaste.git
cd unipaste
make
sudo make install
```
*(Customizable prefix and compiler flags can be edited directly in `config.mk`, `/usr/local` by default).*

---

### Option B: Package Managers

#### Homebrew (macOS & Linux)
```sh
brew tap riccivr/tap
brew install unipaste
```

#### Arch Linux (AUR)
```sh
yay -S unipaste
# or with paru:
paru -S unipaste
```

#### Debian / Ubuntu (.deb)
```sh
sudo dpkg -i unipaste_1.1.0_amd64.deb
```

#### Windows (Chocolatey)
```powershell
choco install unipaste
```

---

### Option C: Pre-built Binary Releases
Pre-compiled standalone binaries for Linux (`x86_64`), Windows (`unipaste.exe`), and Debian `.deb` packages are available on the [GitHub Releases](https://github.com/riccivr/unipaste/releases) page.

Plugin Architecture & HTML Sanitization
---------------------------------------
In line with suckless software principles, **the default build of unipaste is 100% dependency-free**.

However, when processing clipboard data or HTML from untrusted environments (e.g. public websites, unknown email clients, or untrusted user inputs), malicious payloads may embed `<script>` injections, event handlers (`onclick=`, `onerror=`), `javascript:` URIs, or tracker tags.

`unipaste` provides a **compile-time modular plugin architecture** via `plugin.h`. It incurs **zero runtime overhead** and avoids dynamic linker/`dlopen` complexity:

```
┌──────────────────────────────────────────────────────────┐
│                     Raw HTML Input                       │
└────────────────────────────┬─────────────────────────────┘
                             │
                             ▼
┌──────────────────────────────────────────────────────────┐
│  Plugin Hook: sanitize_html(input, len)                  │
│  • Default (plugin_none.c): No-op / passthrough          │
│  • Builtin (plugin_builtin.c): Allowlist tag/attr filter │
└────────────────────────────┬─────────────────────────────┘
                             │
                             ▼
┌──────────────────────────────────────────────────────────┐
│  unipaste Core Engine (parser.c, table.c, entity.c)      │
│  • Reconstructs tables, lists, code fences, markdown     │
└──────────────────────────────────────────────────────────┘
```

### Building with the Built-in HTML Sanitizer

To build `unipaste` with the built-in allowlist sanitizer enabled:

    make SANITIZE=builtin

When built with `SANITIZE=builtin`:
* **Disallowed tags are stripped**: `<script>`, `<style>`, `<iframe>`, `<object>`, `<embed>`, `<svg>`, and `<math>` are removed entirely along with their inner contents.
* **Safe structural tags are preserved**: `<p>`, `<h1>`-`<h6>`, `<table>`, `<ul>`, `<ol>`, `<li>`, `<pre>`, `<code>`, `<blockquote>`, `<a>`, `<b>`, `<i>`, `<input>`.
* **Dangerous event handlers are deleted**: `onclick`, `onload`, `onerror`, etc.
* **Safe URIs only**: `href` and `src` attributes are validated against dangerous schemes (`javascript:`, `data:`, `vbscript:` are blocked).

### Writing Custom Sanitizer Plugins

Custom backends (e.g., binding to external libraries like `libxml2` or Rust-based `ammonia`) can be integrated cleanly:

1. Create a C file (e.g. `plugin_custom.c`) implementing the hook:
   ```c
   #include "plugin.h"
   char *sanitize_html(const char *input, size_t len) {
       /* Your custom sanitization logic */
       /* Return newly allocated char* (freed by caller) or NULL */
   }
   ```
2. Build with your plugin:
   ```sh
   make PLUGIN_SRC=plugin_custom.c
   ```

Testing Suite & Verification
----------------------------
`unipaste` maintains a multi-tiered test matrix inspired by `cmark`, `lowdown`, and `sbase`:

### 1. Unit & Golden Fixture Tests
Runs all 37 core unit tests plus real-world golden snapshot fixtures (Slack, Teams, Google Docs, GitHub):

    make test

To run with the built-in HTML sanitizer enabled:

    make SANITIZE=builtin test

### 2. Pathological & Stress Tests
Tests 2,000-level nesting bombs, column overflows (150+ cols), corrupted UTF-8 byte streams, and null byte injections:

    make stress

### 3. Memory Sanitizers (ASan + UBSan)
Builds and executes all unit, fixture, and stress tests under AddressSanitizer and UndefinedBehaviorSanitizer:

    make sanitize

### 4. Valgrind Memory Leak Analysis
Runs dynamic memory analysis to verify 0 memory leaks and 0 uninitialized reads:

    make valgrind

### 5. Coverage-Guided LibFuzzer
Fuzzes the formatting engine with LLVM LibFuzzer over mutated seeds:

    make fuzz

Usage
-----
```
unipaste [-urv] [-m mode] [-t table] [-l link] [file ...]
```

### Options
* `-m mode`: Output formatting mode:
  * `plain` (default): Clean formatted ASCII plain text tailored for Notepad and standard text editors.
  * `markdown`: Clean GitHub-Flavored Markdown (GFM).
  * `terminal`: Formatted terminal output using ANSI escape sequences (bold headings, italic, etc.).
* `-t table`: Table formatting style:
  * `grid` (default): ASCII box borders (`+---+---+`).
  * `markdown`: Markdown pipe table (`| a | b |`).
  * `tsv`: Tab-separated values.
  * `simple`: Space-aligned columns without outer border.
* `-l link`: Hyperlink formatting style:
  * `bracket` (default for plain): `Title (https://...)`
  * `inline` (default for markdown): `[Title](https://...)`
  * `text`: Strip URLs and keep anchor text only.
  * `footnote`: Place reference numbers `[1]` and append footnotes at the bottom.
* `-u`: Use UTF-8 Unicode box-drawing characters for tables (`┌─┬─┐`).
* `-r`: Emit Windows CRLF (`\r\n`) line endings.
* `-v`: Print version information.
* `-h`: Display help message.

Examples
--------
### 1. Convert Slack clipboard HTML to clean Notepad text
Pipe X11 or Wayland clipboard HTML directly through `unipaste`:

    # On X11:
    xclip -selection clipboard -t text/html -o | unipaste

    # On Wayland:
    wl-paste -t text/html | unipaste

### 2. Convert rich document to GitHub-Flavored Markdown

    unipaste -m markdown document.html > document.md

### 3. Render clipboard tables with Unicode box borders

    xclip -selection clipboard -t text/html -o | unipaste -u

### 4. Convert HTML with Footnote link references

    unipaste -l footnote article.html

License
-------
MIT License. See LICENSE file for details.
