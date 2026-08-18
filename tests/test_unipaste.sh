#!/bin/sh
# test_unipaste.sh - Automated test suite for unipaste
set -e

PASSED=0
FAILED=0
EXE="./unipaste"

test_contains() {
	NAME="$1"
	INPUT="$2"
	SUBSTRING="$3"
	ARGS="$4"

	ACTUAL=$(printf "%s" "$INPUT" | $EXE $ARGS)
	case "$ACTUAL" in
		*"$SUBSTRING"*)
			printf "[PASS] %s\n" "$NAME"
			PASSED=$((PASSED + 1))
			;;
		*)
			printf "[FAIL] %s (missing '%s')\n" "$NAME" "$SUBSTRING"
			printf "=== Actual ===\n%s\n" "$ACTUAL"
			FAILED=$((FAILED + 1))
			;;
	esac
}

echo "Running unipaste test suite..."

# Test 1: Basic text and paragraphs
test_contains "Paragraphs" "<p>First paragraph.</p><p>Second paragraph.</p>" "First paragraph." ""
test_contains "Paragraphs spacing" "<p>First paragraph.</p><p>Second paragraph.</p>" "Second paragraph." ""

# Test 2: HTML Entity decoding
test_contains "Entities" 'Tom &amp; Jerry &quot;Show&quot; &mdash; 5 &lt; 10 &gt; 2 &copy; 2026' 'Tom & Jerry "Show" — 5 < 10 > 2 © 2026' ""

# Test 3: Table formatting (ASCII Grid)
SLACK_TABLE='<table><tr><th>Product</th><th>Price</th><th>Stock</th></tr><tr><td>Widget A</td><td>$10.00</td><td>In Stock</td></tr><tr><td>Widget B</td><td>$25.50</td><td>Out of Stock</td></tr></table>'
test_contains "Table Header" "$SLACK_TABLE" '| Product  | Price  | Stock        |' ""
test_contains "Table Row 1" "$SLACK_TABLE" '| Widget A | $10.00 | In Stock     |' ""
test_contains "Table Row 2" "$SLACK_TABLE" '| Widget B | $25.50 | Out of Stock |' ""
test_contains "Table Border" "$SLACK_TABLE" '+----------+--------+--------------+' ""

# Test 4: Table formatting (Markdown mode)
test_contains "Table Markdown" "$SLACK_TABLE" '| Product  | Price  | Stock        |' "-m markdown"
test_contains "Table Markdown Separator" "$SLACK_TABLE" '| -------- | ------ | ------------ |' "-m markdown"

# Test 5: Lists (Unordered & Ordered)
LIST_HTML='<ul><li>Item 1</li><li>Item 2<ul><li>Nested Sub 1</li><li>Nested Sub 2</li></ul></li><li>Item 3</li></ul>'
test_contains "List Top Level" "$LIST_HTML" "* Item 1" ""
test_contains "List Nested" "$LIST_HTML" "  * Nested Sub 1" ""

OL_HTML='<ol><li>First</li><li>Second</li><li>Third</li></ol>'
test_contains "Ordered List" "$OL_HTML" "1. First" ""
test_contains "Ordered List 2" "$OL_HTML" "2. Second" ""

# Test 6: Code blocks with pre
CODE_HTML='<p>Code sample:</p><pre><code class="language-c">#include &lt;stdio.h&gt;

int main(void) {
    printf("Hello\n");
    return 0;
}</code></pre>'
test_contains "Code block markdown fence" "$CODE_HTML" '```c' "-m markdown"
test_contains "Code block content" "$CODE_HTML" 'printf("Hello\n");' ""

# Test 7: Links
LINK_HTML='<p>Visit <a href="https://example.com">Example Domain</a> today.</p>'
test_contains "Link Bracket" "$LINK_HTML" "Visit Example Domain (https://example.com) today." ""
test_contains "Link Inline" "$LINK_HTML" "Visit [Example Domain](https://example.com) today." "-m markdown"
test_contains "Link Textonly" "$LINK_HTML" "Visit Example Domain today." "-l text"
test_contains "Link Footnote" "$LINK_HTML" "Visit Example Domain [1] today." "-l footnote"
test_contains "Footnote Definition" "$LINK_HTML" "[1] https://example.com" "-l footnote"

# Test 8: Blockquotes
QUOTE_HTML='<blockquote>This is a blockquote.<br>Second line of quote.</blockquote>'
test_contains "Blockquote" "$QUOTE_HTML" "> This is a blockquote." ""

# Test 9: Windows CF_HTML Fragment
CF_HTML='Version:0.9
StartHTML:0000000105
EndHTML:0000000280
StartFragment:0000000180
EndFragment:0000000250
<html><body>
<!--StartFragment--><p>Copied from <b>Slack</b>!</p><!--EndFragment-->
</body></html>'
test_contains "CF_HTML Fragment extraction" "$CF_HTML" "Copied from Slack!" ""

# Test 10: Unicode box tables
test_contains "Unicode Table" "$SLACK_TABLE" "┌" "-u"
test_contains "Unicode Table Border" "$SLACK_TABLE" "│ Product" "-u"

# Test 11: Task lists
TASK_HTML='<ul><li><input type="checkbox" checked> Task done</li><li><input type="checkbox"> Task pending</li></ul>'
test_contains "Task list checked" "$TASK_HTML" "[x] Task done" ""
test_contains "Task list unchecked" "$TASK_HTML" "[ ] Task pending" ""

# Test 12: TSV Table mode
test_contains "TSV Table" "$SLACK_TABLE" 'Widget A	$10.00	In Stock' "-t tsv"

echo ""
echo "======================================"
echo "Results: $PASSED passed, $FAILED failed"
echo "======================================"

if [ "$FAILED" -ne 0 ]; then
	exit 1
fi
