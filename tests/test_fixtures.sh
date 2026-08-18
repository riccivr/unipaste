#!/bin/sh
# test_fixtures.sh - Golden Fixture Snapshot Regression Suite for unipaste
set -e

EXE="./unipaste"
PASSED=0
FAILED=0

pass() {
	printf "[PASS] Fixture: %s\n" "$1"
	PASSED=$((PASSED + 1))
}

fail() {
	printf "[FAIL] Fixture: %s\n" "$1"
	printf "=== Expected ===\n%s\n" "$2"
	printf "=== Actual ===\n%s\n" "$3"
	FAILED=$((FAILED + 1))
}

printf "Running Golden Fixtures Snapshot Suite...\n"

check_fixture() {
	NAME="$1"
	HTML_FILE="$2"
	EXPECTED_FILE="$3"
	ARGS="$4"

	EXPECTED=$(cat "$EXPECTED_FILE")
	ACTUAL=$($EXE $ARGS < "$HTML_FILE")

	# Trim trailing newlines for robust comparison
	EXPECTED_TRIM=$(printf "%s" "$EXPECTED" | sed -e 's/[[:space:]]*$//')
	ACTUAL_TRIM=$(printf "%s" "$ACTUAL" | sed -e 's/[[:space:]]*$//')

	if [ "$ACTUAL_TRIM" = "$EXPECTED_TRIM" ]; then
		pass "$NAME"
	else
		fail "$NAME" "$EXPECTED" "$ACTUAL"
	fi
}

check_fixture "Slack quarterly deployment table" "tests/fixtures/slack_message.html" "tests/fixtures/slack_message.expected" ""
check_fixture "Microsoft Teams release checklist" "tests/fixtures/teams_checklist.html" "tests/fixtures/teams_checklist.expected" ""
check_fixture "Google Docs hierarchical outline" "tests/fixtures/docs_nested_outline.html" "tests/fixtures/docs_nested_outline.expected" ""
check_fixture "GitHub C syntax & blockquote (Markdown)" "tests/fixtures/github_code.html" "tests/fixtures/github_code.expected" "-m markdown"

printf "\n======================================\n"
printf "Fixture Test Results: %d passed, %d failed\n" "$PASSED" "$FAILED"
printf "======================================\n"

if [ "$FAILED" -ne 0 ]; then
	exit 1
fi
