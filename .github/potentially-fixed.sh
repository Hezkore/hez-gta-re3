#!/usr/bin/env bash
# Asks once on every potentially fixed issue to confirm the fix in the latest nightly, and closes the ones nobody answers
set -euo pipefail

LABEL="potentially fixed"
BOT="github-actions[bot]"
DAYS_TO_ANSWER=3
SECONDS_PER_DAY=86400
MOST_ISSUES=1000

# What to do with an issue, from its labels and comments in the order they happened.
# My own comments are no answer, but each one gives the others the full time again.
NEXT_STEP='
	(map(select(.event == "labeled" and .label.name == $label)) | last | .created_at) as $labeled_at
	| map(select(.event == "commented" and .created_at > $labeled_at)) as $comments
	| ($comments | map(select(.user.login == $bot)) | first) as $question
	| if $question == null then "ask"
	  else
		($comments | map(select(.created_at > $question.created_at))) as $later
		| (($later | last) // $question | .created_at | fromdateiso8601) as $waiting_since
		| if any($later[]; .author_association != "OWNER") then "answered"
		  elif now - $waiting_since >= $time_to_answer then "close"
		  else "wait"
		  end
	  end
'

next_step() {
	gh api "repos/{owner}/{repo}/issues/$1/timeline" --paginate \
		--jq '.[] | select(.event == "labeled" or .event == "commented")' |
		jq -rs --arg label "$LABEL" --arg bot "$BOT" \
			--argjson time_to_answer "$((DAYS_TO_ANSWER * SECONDS_PER_DAY))" "$NEXT_STEP"
}

latest_nightly=$(gh api "repos/{owner}/{repo}/releases" \
	--jq '[.[] | select(.tag_name | startswith("nightly-"))] | first | "[\(.name)](\(.html_url))"')

gh issue list --label "$LABEL" --state open --limit "$MOST_ISSUES" --json number --jq '.[].number' |
while read -r number; do
	step=$(next_step "$number")
	echo "potentially-fixed: #$number $step"

	case $step in
	ask)
		gh issue comment "$number" --body "This issue is potentially fixed in the latest nightly release, $latest_nightly.
Please confirm."
		;;
	close)
		gh issue close "$number" --comment "No one responded in $DAYS_TO_ANSWER days, so I am closing this issue.
Reply if the problem persists in the latest nightly."
		;;
	esac
done
