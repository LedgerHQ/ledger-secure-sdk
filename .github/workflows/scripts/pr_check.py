#!/usr/bin/env python3
# coding: utf-8
# -----------------------------------------------------------------------------
"""
This module checks the currently closed PR and
automatically creates cherry-pick PRs on the targeted API_LEVEL branch.
"""
# -----------------------------------------------------------------------------

from argparse import ArgumentParser, ArgumentDefaultsHelpFormatter
import re
import sys
import logging
import tempfile
from typing import List, Tuple
from github import Github
from github.Repository import Repository
from github.PullRequest import PullRequest
from git import Repo, GitCommandError


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


# ===============================================================================
#          Sub functions to retrieve objects
# ===============================================================================
def get_pull_request(repo: Repository, pull_number: int) -> PullRequest:
    """Retrieve the pull request object."""
    try:
        pull = repo.get_pull(pull_number)
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


def get_all_branches(repo: Repository) -> List[str]:
    """Retrieve all branches in the repository."""
    try:
        branches = [br.name for br in repo.get_branches()]
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


def clone_repo(github_url: str, local_path: str, token: str) -> None:
    """Clone the repo to local_path."""
    # Insert token into URL for authentication
    if github_url.startswith("https://"):
        protocol, rest = github_url.split("://", 1)
        github_url = f"{protocol}://{token}:x-oauth-basic@{rest}"
    logger.info("Cloning repo from %s to %s", rest, local_path)
    Repo.clone_from(github_url, local_path)


def create_or_get_branch(github_url: str,
                         target_br: str,
                         branches: List[str],
                         repo_path: str,
                         token: str,
                         dry_run: bool = False) -> Tuple[str, bool]:
    """Create a new branch if it doesn't exist, or get the existing branch."""
    branch_created: bool = False
    auto_branch = f"auto_update_{target_br}"
    if auto_branch not in branches:
        logger.info("Create branch: %s", auto_branch)
        branch_created = True
        # Branch does not exist yet, create it now
        local_repo = Repo(repo_path)
        origin = local_repo.remotes.origin
        origin.fetch()
        local_repo.git.checkout(target_br)
        local_repo.git.checkout('-b', auto_branch)
        if not dry_run:
            if github_url.startswith("https://"):
                protocol, rest = github_url.split("://", 1)
                github_url = f"{protocol}://x-access-token:{token}@{rest}"
                local_repo.git.remote("set-url", "origin", github_url)
            local_repo.git.push('--set-upstream', 'origin', auto_branch)
    return auto_branch, branch_created


def branch_delete(repo: Repo, auto_branch: str, default_branch: str, dry_run: bool = False) -> None:
    """Delete the specified branch."""
    repo.git.checkout(default_branch)
    # Delete the local branch
    try:
        repo.git.branch('-D', auto_branch)  # Force delete
        logger.info("Deleted local branch: %s", auto_branch)
    except GitCommandError as abort_err:
        logger.warning("Failed to delete the local branch: %s", abort_err)

    if not dry_run:
        # Delete the remote branch
        try:
            repo.git.push('origin', '--delete', auto_branch)
            logger.info("Deleted remote branch: %s", auto_branch)
        except GitCommandError as abort_err:
            logger.warning("Failed to delete the remote branch: %s", abort_err)


def cherry_pick_commits(repo_path: str,
                        commits: List[str],
                        auto_branch: str,
                        default_branch: str,
                        branch_created: bool,
                        dry_run: bool = False) -> bool:
    """Cherry-pick the specified commits onto the auto_update branch."""
    repo = Repo(repo_path)
    origin = repo.remotes.origin

    # Configure Git user identity for the cherry-pick
    repo.config_writer().set_value("user", "name", "github-actions[bot]").release()
    repo.config_writer().set_value("user", "email", "github-actions[bot]@users.noreply.github.com").release()

    # Fetch latest changes
    origin.fetch()

    # Checkout the target branch (or create a new one)
    repo.git.checkout(auto_branch)

    # update the target branch
    if not dry_run:
        repo.git.pull('origin', auto_branch)

    for sha in commits:
        try:
            logger.info("Cherry-picking %s onto %s", sha, auto_branch)
            repo.git.cherry_pick('-x', sha)
        except GitCommandError as err:
            logger.error("Failed to apply cherry-pick of %s:\n%s", sha, err)
            # Abort cherry-pick on conflict
            try:
                repo.git.cherry_pick('--abort')
            except GitCommandError:
                pass
            if branch_created:
                branch_delete(repo, auto_branch, default_branch, dry_run)
            return False

    # Push the branch if needed
    if not dry_run:
        repo.git.push('origin', auto_branch)

    return True


def create_pull_request_if_needed(repo: Repository,
                                  auto_branch: str,
                                  target_br: str,
                                  pull_number: int) -> None:
    """Create a pull request if one does not already exist."""
    pr_auto_title = f"[AUTO_UPDATE] Branch {target_br}"
    prs = repo.get_pulls(state="open", base=target_br)

    # Check if PR already exists
    existing_pr = None
    for pr in prs:
        if pr.title == pr_auto_title:
            existing_pr = pr
            logger.info("There is already an existing PR. Just update it!")
            break

    # Get original PR info for the body
    initialPR = repo.get_pull(pull_number)
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
    else:
        try:
            # Create a new pull request with the auto_update branch as
            # the head and the target branch as the base
            initialPR = repo.get_pull(pull_number)
            new_pr = repo.create_pull(
                title=pr_auto_title,
                body=f"Automated update from #{pull_number} ({initialPR.title})",
                head=auto_branch,
                base=target_br
            )
        except:
            logger.error("Failed to create PR!")
            sys.exit(1)

        try:
            # Assign the PR to the original author
            original_author = initialPR.user.login
            new_pr.create_review_request(reviewers=[original_author])
            logger.info("Assigned PR to original author: %s", original_author)
        except:
            logger.error("Failed to assign PR to original author: %s", original_author)
        logger.info("Created PR for branch '%s': %s", auto_branch, new_pr.html_url)


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
        github = Github(args.token)
    except Exception as err:
        logger.error("Github connection error: %s", err)
        sys.exit(1)

    # Retrieve repo object
    repo = github.get_repo(f"{GITHUB_ORG_NAME}/{args.repo}")
    logger.debug("Repo Name: %s", repo.name)
    default_branch = repo.default_branch

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

    # Prepare local repo path
    github_url = f"https://github.com/{GITHUB_ORG_NAME}/{args.repo}.git"

    # Loop for all target branches
    result = 0
    for target_br in target_branches:
        logger.info("-------------")
        logger.info("Target branch: %s", target_br)
        with tempfile.TemporaryDirectory() as local_repo_path:

            # Clone the repo
            clone_repo(github_url, local_repo_path, args.token)

            # Check if dedicated auto_update branch exists, else create it
            auto_branch, branch_created = create_or_get_branch(github_url,
                                                               target_br,
                                                               branches,
                                                               local_repo_path,
                                                               args.token,
                                                               args.dry_run)

            # Cherry-pick the commits onto the auto_update branch
            if not cherry_pick_commits(local_repo_path,
                                       commits,
                                       auto_branch,
                                       default_branch,
                                       branch_created,
                                       args.dry_run):
                result = 1
                continue

            if not args.dry_run:
                # Create a pull request if needed
                create_pull_request_if_needed(repo, auto_branch, target_br, args.pull)

    if result != 0:
        logger.error("One or more operations failed.")
        sys.exit(1)

if __name__ == "__main__":
    main()
