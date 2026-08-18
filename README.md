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
* **Memory safe**: Tested with AddressSanitizer and UndefinedBehaviorSanitizer.

Requirements
------------
In order to build `unipaste` you need a C99 compiler and `make`.

Installation
------------
Edit `config.mk` to match your local setup (`unipaste` is installed into the
`/usr/local` namespace by default).

Afterwards enter the following command to build and install unipaste:

    make
    make install

Running tests
-------------
To execute the automated test suite:

    make test

To run AddressSanitizer and UndefinedBehaviorSanitizer tests:

    make sanitize

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
