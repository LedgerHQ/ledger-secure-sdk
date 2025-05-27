#!/usr/bin/env python3
# coding: utf-8
# -----------------------------------------------------------------------------
"""
This module checks the currently closed PR and
automatically create cherry-pick PR on the targeted API_LEVEL branch.
"""
# -----------------------------------------------------------------------------

from argparse import ArgumentParser, ArgumentDefaultsHelpFormatter
import re
import sys
import logging
from typing import List
from github import Github
from github.Repository import Repository
from github.PullRequest import PullRequest


GITHUB_ORG_NAME = "LedgerHQ"

logger = logging.getLogger(__name__)


# ===============================================================================
#          Parameters
# ===============================================================================
def init_parser() -> ArgumentParser:
    """Initialize the argument parser for command line arguments."""
    parser = ArgumentParser(description="Automatically cherry-pick SDK commits",
                            formatter_class=ArgumentDefaultsHelpFormatter)
    parser.add_argument("--token", "-t", required=True, help="GitHub Access Token")
    parser.add_argument("--repo", "-r", required=True, help="GitHub Repository")
    parser.add_argument("--pull", "-p", type=int, required=True, help="Pull Request number")
    parser.add_argument("--verbose", "-v", action='store_true', help="Verbose mode")
    return parser


# ===============================================================================
#          Parameters
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


# ===============================================================================
#          Sub functions to retrieve objects
# ===============================================================================
def get_pull_request(repo: Repository, pull_number: int) -> PullRequest:
    """Retrieve the pull request object."""
    try:
        pull = repo.get_pull(pull_number)
        logger.debug("Pull Title: %s", pull.title)
        logger.debug("Pull Body:\n%s", pull.body)
        return pull
    except Exception as e:
        logger.error("Failed to retrieve pull request: %s", e)
        sys.exit(1)


def get_target_branches(pull: PullRequest) -> List[str]:
    """Extract target branches from the pull request description."""
    if not pull.body:
        logger.warning("Empty PR description. Exiting.")
        sys.exit(0)
    # Match lines like: [x] TARGET_API_LEVEL: API_LEVEL_XX or [ X ] TARGET_API_LEVEL: API_LEVEL_XX
    pattern = re.compile(r'^\[\s*[xX]\s*\]\s*TARGET_API_LEVEL:\s*(API_LEVEL_\d+)', re.MULTILINE)
    matches = pattern.finditer(pull.body)
    target_branches = [match.group(1) for match in matches]
    if len(target_branches) == 0:
        logger.warning("No target branch specified for cherry-pick. Exiting.")
        sys.exit(0)
    logger.debug("Target Branches: %s", target_branches)
    return target_branches


def get_all_branches(repo: Repository) -> List[str]:
    """Retrieve all branches in the repository."""
    try:
        branches = [br.name for br in repo.get_branches()]
        logger.debug("Branches: %s", branches)
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
    return commits


def create_or_get_branch(repo: Repository, target_br: str, branches: List[str]) -> str:
    """Create a new branch if it doesn't exist, or get the existing branch."""
    auto_branch = f"auto_update_{target_br}"
    if auto_branch not in branches:
        # Branch does not exist yet, create it!
        sha = repo.get_branch(target_br).commit.sha
        logger.debug("Found sha: %s", sha)
        repo.create_git_ref(f"refs/heads/{auto_branch}", sha)
    return auto_branch


def cherry_pick_commits(repo: Repository, commits: List[str], auto_branch: str) -> None:
    """Cherry-pick the specified commits onto the target branch."""
    # Get the latest commit SHA of the auto_update branch
    my_br = repo.get_branch(auto_branch)
    br_sha = my_br.commit.sha
    logger.info("Target branch '%s' sha: %s", my_br.name, br_sha)

    # Apply the cherry-picks
    for com in commits:
        # Get the commit object for the current commit SHA
        commit = repo.get_commit(com)

        # Retrieve the GitTree object using the commit's tree SHA
        tree = repo.get_git_tree(commit.commit.tree.sha)

        # Create a new commit with the same message and tree,
        # but with the current branch's latest commit as the parent
        try:
            new_commit = repo.create_git_commit(
                message=commit.commit.message,
                tree=tree,
                parents=[repo.get_commit(br_sha).commit]
            )
        except:
            logger.error("Failed to create commit!")
            sys.exit(1)

        # Update the branch SHA to the new commit's SHA
        br_sha = new_commit.sha
        logger.debug("Cherry-picked commit %s to %s", commit.sha, new_commit.sha)

    # Update the reference of the branch to the new commit
    try:
        ref = repo.get_git_ref(f"heads/{auto_branch}")
    except:
        logger.error("Failed to get git reference!")
        sys.exit(1)
    ref.edit(sha=br_sha)
    logger.info("Updated branch '%s' to new commit %s", auto_branch, br_sha)


def create_pull_request_if_needed(repo: Repository, auto_branch: str, target_br: str) -> None:
    """Create a pull request if one does not already exist."""
    pr_auto_title = f"[AUTO_UPDATE] Branch {target_br}"
    prs = repo.get_pulls(state="open", base=target_br)
    if not any(pr.title == pr_auto_title for pr in prs):
        # Create a new pull request with the auto_update branch as
        # the head and the target branch as the base
        try:
            repo.create_pull(
                title=pr_auto_title,
                body="Automated update",
                head=auto_branch,
                base=target_br
            )
        except:
            logger.error("Failed to create PR!")
            sys.exit(1)
        logger.info("Created PR for branch '%s'", auto_branch)


# ===============================================================================
#          Main entry
# ===============================================================================
def main():
    """Main entry point for the script."""

    parser = init_parser()
    args = parser.parse_args()

    set_logging(args.verbose)

    # Connect to GitHub LedgerHQ
    try:
        github = Github(args.token)
    except Exception as err:
        logger.error("Github connection error: %s", err)
        sys.exit(1)

    # Retrieve repo object
    repo = github.get_repo(f"{GITHUB_ORG_NAME}/{args.repo}")
    logger.debug("Repo Name: %s", repo.name)

    # Retrieve PR object
    pull = get_pull_request(repo, args.pull)

    # Search target branches from PR description
    target_branches = get_target_branches(pull)

    # Retrieve all available branches
    branches = get_all_branches(repo)

    # Check if all target branches exist
    validate_target_branches(target_branches, branches)

    # Retrieve the list of commits to cherry-pick
    commits = get_commits_to_cherry_pick(pull)

    # Loop for all target branches
    for target_br in target_branches:
        # Check if dedicated auto_update branch exists, else create it
        auto_branch = create_or_get_branch(repo, target_br, branches)

        # Cherry-pick the commits onto the auto_update branch
        cherry_pick_commits(repo, commits, auto_branch)

        # Create a pull request if needed
        create_pull_request_if_needed(repo, auto_branch, target_br)


if __name__ == "__main__":
    main()
