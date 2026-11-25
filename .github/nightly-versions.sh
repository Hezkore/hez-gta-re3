#!/usr/bin/env bash
# Lists the nightlies we still keep in the bug form's Version dropdown
set -euo pipefail

FORM=.github/ISSUE_TEMPLATE/1-bug.yml
SELF_BUILT="Built from source"

cd "$(git rev-parse --show-toplevel)"

# tags are nightly-YYYY-MM-DD, so a reverse sort puts the newest first
tags=$(gh release list --limit 100 --json tagName --jq '.[].tagName | select(startswith("nightly-"))' | sort -r)
if [ -z "$tags" ]; then
	echo "nightly-versions: no nightly releases found" >&2
	exit 1
fi

OPTIONS="$tags
Older than $(tail -n 1 <<< "$tags")
$SELF_BUILT" awk '
	/^    id: / { in_version = ($2 == "version") }
	in_version && /^      options:$/ {
		print
		count = split(ENVIRON["OPTIONS"], options, "\n")
		for (i = 1; i <= count; i++) print "        - " options[i]
		replacing = 1
		next
	}
	replacing && /^        - / { next }
	{ replacing = 0; print }
' "$FORM" > "$FORM.new"
mv "$FORM.new" "$FORM"
echo "nightly-versions: $(wc -l <<< "$tags") nightlies in $FORM"
