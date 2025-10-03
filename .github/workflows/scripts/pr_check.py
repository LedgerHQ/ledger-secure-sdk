#!/usr/bin/env python3
# coding: utf-8
# -----------------------------------------------------------------------------
"""
This module checks the currently closed PR and
automatically creates cherry-pick PRs on the targeted API_LEVEL branch.
"""
# -----------------------------------------------------------------------------

from argparse import ArgumentParser, ArgumentDefaultsHelpFormatter
import os
import re
import sys
import logging
import tempfile
from typing import List, Tuple
from github import Github, Auth
from github.Repository import Repository
from github.PullRequest import PullRequest
from git import Repo, GitCommandError


GITHUB_ORG_NAME = "LedgerHQ"
GITHUB_REPO_NAME = "ledger-secure-sdk"
GITHUB_REPO_PATH = f"{GITHUB_ORG_NAME}/{GITHUB_REPO_NAME}"
GITHUB_FULL_URL = f"https://github.com/{GITHUB_REPO_PATH}.git"

logger = logging.getLogger(__name__)


# ===============================================================================
#          Parameters
# ===============================================================================
def init_parser() -> ArgumentParser:
    """Initialize the argument parser for command line arguments."""
    parser = ArgumentParser(description="Automatically cherry-pick SDK commits",
                            formatter_class=ArgumentDefaultsHelpFormatter)
    parser.add_argument("--token", "-t", required=True, help="GitHub Access Token")
    parser.add_argument("--repo_path", "-r", help="Cloned repository path")
    parser.add_argument("--pull", "-p", type=int, required=True, help="Pull Request number")
    parser.add_argument("--branch", "-b", help="target branch (manual selection)")
    parser.add_argument("--dry_run", "-d", action='store_true', help="Dry Run mode")
    parser.add_argument("--verbose", "-v", action='store_true', help="Verbose mode")
    return parser


# ===============================================================================
#          Logging
# ===============================================================================
def set_logging(verbose: bool = False) -> None:
    """Set up logging configuration."""
    if verbose:
        logger.setLevel(level=logging.DEBUG)
    else:
        logger.setLevel(level=logging.INFO)
    logger.handlers.clear()
    handler = logging.StreamHandler()
    handler.setFormatter(logging.Formatter("[%(levelname)s] %(message)s"))
    logger.addHandler(handler)


def set_gh_summary(value: str) -> None:
    """Sets a summary status for a GitHub Actions workflow.
       This is used to write the summary of the workflow run.

    Args:
        value: Summary content
    """

    # Check if the GITHUB_STEP_SUMMARY environment variable is set
    gh = os.environ.get("GITHUB_STEP_SUMMARY")
    if gh:
        with open(gh, "a", encoding="utf-8") as outfile:
            outfile.write(f"{value}\n")


# ===============================================================================
#          Sub functions to retrieve objects
# ===============================================================================
def get_pull_request(git_repo: Repository, pull_number: int) -> PullRequest:
    """Retrieve the pull request object."""
    try:
        pull = git_repo.get_pull(pull_number)
        logger.debug("Pull Title: %s", pull.title)
    except Exception as e:
        logger.error("Failed to retrieve pull request: %s", e)
        sys.exit(1)
    return pull


def get_target_branches(pull: PullRequest) -> List[str]:
    """Extract target branches from the pull request description."""
    if not pull.body:
        logger.warning("Empty PR description. Exiting.")
        sys.exit(0)
    # Match lines like: [x] TARGET_API_LEVEL: API_LEVEL_XX or [ X ] TARGET_API_LEVEL: API_LEVEL_XX
    pattern = re.compile(
        r'^\[\s*[xX]\s*\]\s*TARGET_API_LEVEL:\s*(API_LEVEL_\d+)\b',
        re.MULTILINE | re.IGNORECASE
    )
    matches = pattern.finditer(pull.body)
    target_branches = [match.group(1) for match in matches]
    if len(target_branches) == 0:
        logger.warning("No target branch specified for cherry-pick. Exiting.")
        sys.exit(0)
    logger.debug("Target Branches: %s", target_branches)
    return target_branches


def get_all_branches(git_repo: Repository) -> List[str]:
    """Retrieve all branches in the repository."""
    try:
        branches = [br.name for br in git_repo.get_branches()]
        return branches
    except Exception as e:
        logger.error("Failed to retrieve branches: %s", e)
        sys.exit(1)


def validate_target_branches(target_branches: List[str], branches: List[str]) -> None:
    """Validate that all target branches exist in the repository."""
    for target_br in target_branches:
        if target_br not in branches:
            logger.error("Target branch '%s' doesn't exist! Aborting.", target_br)
            sys.exit(1)


def get_commits_to_cherry_pick(pull: PullRequest) -> List[str]:
    """Retrieve the list of commits to cherry-pick from the pull request."""
    commits = [commit.sha for commit in pull.get_commits()]
    if len(commits) == 0:
        logger.error("No commits found. Exiting.")
        sys.exit(1)
    logger.info("Found commits: %s", commits)
    set_gh_summary(f":loudspeaker: Found {len(commits)} commits")
    return commits


def create_or_get_branch(local_repo: Repo,
                         target_br: str,
                         branches: List[str],
                         dry_run: bool = False) -> Tuple[str, bool]:
    """Create a new branch if it doesn't exist, or get the existing branch."""
    origin = local_repo.remotes.origin
    origin.fetch()

    branch_created: bool = False
    auto_branch = f"auto_update_{target_br}"
    if auto_branch not in branches:
        logger.info("Create branch: %s", auto_branch)
        branch_created = True
        # Checkout the branch we want to base the new branch on (e.g. API_LEVEL_25)
        local_repo.git.checkout(target_br)
        # Create the new branch now
        local_repo.git.checkout('-b', auto_branch)
        if not dry_run:
            local_repo.git.push('--set-upstream', 'origin', auto_branch)
    else:
        # Branch already exists, just checkout
        logger.info("Branch already exists: %s", auto_branch)
        # Checkout the target branch (or create a new one)
        local_repo.git.checkout(auto_branch)
        # update the target branch
        local_repo.git.pull('origin', auto_branch)

    return auto_branch, branch_created


def branch_delete(local_repo: Repo,
                  auto_branch: str,
                  default_branch: str,
                  dry_run: bool = False) -> None:
    """Delete the specified branch."""
    local_repo.git.checkout(default_branch)
    # Delete the local branch
    try:
        local_repo.git.branch('-D', auto_branch)  # Force delete
        logger.info("Deleted local branch: %s", auto_branch)
    except GitCommandError as abort_err:
        logger.warning("Failed to delete the local branch: %s", abort_err)

    if not dry_run:
        # Delete the remote branch
        try:
            local_repo.git.push('origin', '--delete', auto_branch)
            logger.info("Deleted remote branch: %s", auto_branch)
        except GitCommandError as abort_err:
            logger.warning("Failed to delete the remote branch: %s", abort_err)


def cherry_pick_commits(local_repo: Repo,
                        commits: List[str],
                        auto_branch: str,
                        default_branch: str,
                        branch_created: bool,
                        dry_run: bool = False) -> bool:
    """Cherry-pick the specified commits onto the auto_update branch."""

    for sha in commits:
        try:
            logger.info("Cherry-picking %s", sha)
            local_repo.git.cherry_pick('-x', sha)
        except GitCommandError as err:
            logger.error("Failed to apply cherry-pick of %s:\n%s", sha, err)
            # Abort cherry-pick on conflict
            try:
                local_repo.git.cherry_pick('--abort')
            except GitCommandError:
                pass
            if branch_created:
                branch_delete(local_repo, auto_branch, default_branch, dry_run)
            return False

    # Push the branch if needed
    if not dry_run:
        try:
            local_repo.git.push('origin', auto_branch)
        except GitCommandError as err:
            logger.error("Failed to push branch %s:\n%s", auto_branch, err)
            return False

    return True


def create_pull_request_if_needed(git_repo: Repository,
                                  auto_branch: str,
                                  target_br: str,
                                  pull_number: int) -> bool:
    """Create a pull request if one does not already exist."""
    pr_auto_title = f"[AUTO_UPDATE] Branch {target_br}"
    prs = git_repo.get_pulls(state="open", base=target_br)

    # Check if PR already exists
    existing_pr = None
    for pr in prs:
        if pr.title == pr_auto_title:
            existing_pr = pr
            logger.info("There is already an existing PR. Just update it!")
            break

    # Get original PR info for the body
    initialPR = git_repo.get_pull(pull_number)
    new_body = f"Automated update from #{pull_number} ({initialPR.title})"

    if existing_pr:
        # Update existing PR
        try:
            # Append to existing body or replace it
            current_body = existing_pr.body or ""
            if f"#{pull_number}" not in current_body:
                # Only add if this PR reference isn't already in the body
                updated_body = f"{current_body}\n\n{new_body}" if current_body else new_body
                existing_pr.edit(body=updated_body)
                logger.info("Update existing PR #%d body", existing_pr.number)
            else:
                logger.info("PR #%d already references #%d, skipping body update",
                            existing_pr.number, pull_number)
        except Exception as e:
            logger.error("Failed to update existing PR: %s", e)
            return False
    else:
        try:
            # Create a new pull request with the auto_update branch as
            # the head and the target branch as the base
            initialPR = git_repo.get_pull(pull_number)
            new_pr = git_repo.create_pull(
                title=pr_auto_title,
                body=f"Automated update from #{pull_number} ({initialPR.title})",
                head=auto_branch,
                base=target_br
            )
        except:
            logger.error("Failed to create PR!")
            return False

        try:
            # Assign the PR to the original author
            original_author = initialPR.user.login
            new_pr.create_review_request(reviewers=[original_author])
            logger.info("Assigned PR to original author: %s", original_author)
        except:
            logger.error("Failed to assign PR to original author: %s", original_author)
        logger.info("Created PR for branch '%s': %s", auto_branch, new_pr.html_url)
    return True


# ===============================================================================
#          Main entry
# ===============================================================================
def main():
    """Main entry point for the script."""

    parser = init_parser()
    args = parser.parse_args()

    set_logging(args.verbose)
    if args.dry_run:
        logger.info("/!\\ DRY-RUN Mode /!\\")

    # Connect to GitHub LedgerHQ
    try:
        auth = Auth.Token(args.token)
        github = Github(auth=auth)
    except Exception as err:
        logger.error("Github connection error: %s", err)
        sys.exit(1)

    # Retrieve remote repo object
    git_repo = github.get_repo(GITHUB_REPO_PATH)
    logger.debug("Repo Name: %s", git_repo.name)
    default_branch = git_repo.default_branch

    # Retrieve PR object
    pull = get_pull_request(git_repo, args.pull)

    if args.branch:
        # check branch naming
        if not args.branch.startswith("API_LEVEL_"):
            logger.error("Invalid branch: %s", args.branch)
            sys.exit(1)
        target_branches = [args.branch]
    else:
        # Search target branches from PR description
        target_branches = get_target_branches(pull)

    # Retrieve all available branches
    branches = get_all_branches(git_repo)

    # Check if all target branches exist
    validate_target_branches(target_branches, branches)

    # Retrieve the list of commits to cherry-pick
    commits = get_commits_to_cherry_pick(pull)

    # Clone the repo if not already done
    if args.repo_path and os.path.isdir(args.repo_path):
        local_repo_path = args.repo_path
        logger.info("Using existing repository at %s", local_repo_path)
    else:
        # Prepare local repo path
        local_repo_path = tempfile.mkdtemp()
        logger.info("Cloning repo from %s to %s", GITHUB_FULL_URL, local_repo_path)
        try:
            Repo.clone_from(GITHUB_FULL_URL, local_repo_path)
        except Exception as err:
            logger.error("Failed to clone repository: %s", err)
            sys.exit(1)

    # Open the local repository
    try:
        local_repo = Repo(local_repo_path)
    except Exception as err:
        logger.error("Failed to open local repository: %s", err)
        sys.exit(1)
    # Configure Git user identity for the cherry-pick
    local_repo.config_writer().set_value("user", "name", "github-actions[bot]").release()
    local_repo.config_writer().set_value("user", "email", "github-actions[bot]@users.noreply.github.com").release()

    # Loop for all target branches
    result = 0
    for target_br in target_branches:
        logger.info("-------------")
        logger.info("Target branch: %s", target_br)

        # Ensure that we are on the default branch to start cleanly
        local_repo.git.checkout(default_branch)

        # Check if dedicated auto_update branch exists, else create it
        auto_branch, branch_created = create_or_get_branch(local_repo,
                                                           target_br,
                                                           branches,
                                                           args.dry_run)

        # Cherry-pick the commits onto the auto_update branch
        if not cherry_pick_commits(local_repo,
                                   commits,
                                   auto_branch,
                                   default_branch,
                                   branch_created,
                                   args.dry_run):
            result = 1

        if result == 0 and not args.dry_run:
            # Create a pull request if needed
            if not create_pull_request_if_needed(git_repo, auto_branch, target_br, args.pull):
                result = 1

        try:
            # Return to default branch
            local_repo.git.checkout(default_branch)
        except GitCommandError as e:
            logger.warning("Failed to checkout to default branch (%s): %s", default_branch, e)

        # Set result in summary
        if result == 0:
            set_gh_summary(f":white_check_mark: {target_br}")
        else:
            set_gh_summary(f":x: {target_br}")


    if result != 0:
        logger.error("One or more operations failed.")
        sys.exit(1)


if __name__ == "__main__":
    main()
