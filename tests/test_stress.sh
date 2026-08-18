#!/bin/sh
# test_stress.sh - Pathological & Adversarial Stress Test Suite for unipaste
# Inspired by cmark pathological_tests and lowdown stress harness
set -e

EXE="./unipaste"
PASSED=0
FAILED=0

pass() {
	printf "[PASS] %s\n" "$1"
	PASSED=$((PASSED + 1))
}

fail() {
	printf "[FAIL] %s: %s\n" "$1" "$2"
	FAILED=$((FAILED + 1))
}

printf "Running Pathological & Stress Test Suite...\n"

# Test 1: Deep Blockquote Nesting (2,000 levels)
python3 -c "print('<blockquote>' * 2000 + 'Deep content' + '</blockquote>' * 2000)" | $EXE > /dev/null 2>&1 && pass "Deep Blockquote Nesting (2,000 levels)" || fail "Deep Blockquote Nesting" "crashed"

# Test 2: Deep List Nesting (100 levels, exceeding MAX_LIST_DEPTH)
python3 -c "print('<ul>' * 100 + '<li>Deep Item</li>' + '</ul>' * 100)" | $EXE > /dev/null 2>&1 && pass "Deep List Nesting (100 levels)" || fail "Deep List Nesting" "crashed"

# Test 3: Giant Table Column Overflow (150 columns x 200 rows)
python3 -c "
cols = ''.join([f'<th>Col {i}</th>' for i in range(150)])
row = ''.join([f'<td>Cell {i}</td>' for i in range(150)])
rows = ''.join([f'<tr>{row}</tr>' for _ in range(200)])
print(f'<table><tr>{cols}</tr>{rows}</table>')
" | $EXE > /dev/null 2>&1 && pass "Giant Table Column Overflow (150 cols x 200 rows)" || fail "Giant Table Column Overflow" "crashed"

# Test 4: Unclosed Tag Avalanche (2,000 unclosed tags at EOF)
python3 -c "print('<p>Text ' + '<b><i><span><a>' * 500)" | $EXE > /dev/null 2>&1 && pass "Unclosed Tag Avalanche (2,000 tags)" || fail "Unclosed Tag Avalanche" "crashed"

# Test 5: Entity Flooding & Numeric Overflow
python3 -c "print(''.join([f'&#{10**i};' for i in range(1, 40)]) + '&#99999999999999999999999999999999;')" | $EXE > /dev/null 2>&1 && pass "Entity Flooding & Numeric Overflow" || fail "Entity Flooding" "crashed"

# Test 6: Broken & Truncated Multi-byte UTF-8 Streams
# 0xE2 0x82 (truncated 3-byte), 0xF0 0x9F (truncated 4-byte), 0xC0 (invalid leading byte), 0xFF (illegal byte)
printf '\xE2\x82\xF0\x9F\xC0\x80\xFF\xFE<p>Valid text \xE2\x82\xAC 100</p>\x80\xBF' | $EXE > /dev/null 2>&1 && pass "Corrupted & Truncated UTF-8 Byte Stream" || fail "Corrupted UTF-8" "crashed"

# Test 7: Embedded Null Bytes inside Tags & Attributes
printf '<p>Null \0 in text <a href="http://example.com/\0evil">link\0</a></p>' | $EXE > /dev/null 2>&1 && pass "Embedded Null Bytes in Tags & Content" || fail "Null Bytes" "crashed"

# Test 8: Large Input Stream (5 MB text throughput)
python3 -c "
chunk = '<p>Paragraph with <b>bold</b>, <i>italic</i>, and <a href=\"https://example.com\">link</a>.</p>\n'
print(chunk * 50000)
" | $EXE > /dev/null 2>&1 && pass "Large Stream Throughput (5 MB)" || fail "Large Stream" "crashed"

# Test 9: Rapid Malformed Tag Transitions
python3 -c "print('<><><<<<///><p><<br>>></p><<<>>>' * 1000)" | $EXE > /dev/null 2>&1 && pass "Malformed Rapid Tag Transitions (1,000 reps)" || fail "Malformed Tag Transitions" "crashed"

# Test 10: Jagged / Unbalanced Tables
python3 -c "print('<table><tr><td>Only 1 cell</td></tr><tr><td>1</td><td>2</td><td>3</td><td>4</td></tr><tr></tr></table>')" | $EXE > /dev/null 2>&1 && pass "Jagged & Empty Table Rows" || fail "Jagged Tables" "crashed"

printf "\n======================================\n"
printf "Stress Test Results: %d passed, %d failed\n" "$PASSED" "$FAILED"
printf "======================================\n"

if [ "$FAILED" -ne 0 ]; then
	exit 1
fi
