#!/usr/bin/env python3
"""
Git Diff to Commit Message Dataset Preparation Script

This script extracts git diff and commit message pairs from git repositories
and formats them for training a commit message generation model.

Usage:
    python data_preparation.py --repo-path /path/to/git/repo --output data.jsonl
    python data_preparation.py --use-huggingface-dataset --output data.jsonl
"""

import os
import json
import argparse
import logging
from pathlib import Path
from typing import List, Dict, Optional
import re

try:
    import git
    from datasets import load_dataset, Dataset
    from tqdm import tqdm
    # Configure Hugging Face Hub to use domestic mirror for better connectivity
    import os
    if not os.environ.get('HF_ENDPOINT'):
        os.environ['HF_ENDPOINT'] = 'https://hf-mirror.com'
        # logger.info("Using Hugging Face mirror: https://hf-mirror.com")
except ImportError as e:
    print(f"Missing required dependencies. Please install them: {e}")
    exit(1)

# Configure logging
logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')
logger = logging.getLogger(__name__)


class GitDiffExtractor:
    """Extract git diff and commit message pairs from git repositories."""

    def __init__(self, repo_path: str, max_commits: int = 10000):
        """
        Initialize the extractor.

        Args:
            repo_path: Path to the git repository
            max_commits: Maximum number of commits to extract
        """
        self.repo_path = Path(repo_path)
        self.max_commits = max_commits

        if not self.repo_path.exists():
            raise ValueError(f"Repository path does not exist: {repo_path}")

        try:
            self.repo = git.Repo(repo_path)
        except git.InvalidGitRepositoryError:
            raise ValueError(f"Invalid git repository: {repo_path}")

    def extract_commits(self, branch: str = 'main') -> List[Dict[str, str]]:
        """
        Extract diff and commit message pairs from the repository.

        Args:
            branch: Branch to extract commits from

        Returns:
            List of dictionaries containing diff and commit_message
        """
        logger.info(f"Extracting commits from branch: {branch}")

        try:
            # Try common branch names
            branches_to_try = [branch, 'master', 'develop', 'dev']
            commits = None

            for branch_name in branches_to_try:
                try:
                    commits = list(self.repo.iter_commits(branch_name, max_count=self.max_commits))
                    if commits:
                        logger.info(f"Successfully found {len(commits)} commits in branch '{branch_name}'")
                        break
                except git.GitCommandError:
                    continue

            if not commits:
                raise ValueError(f"No commits found in branches: {branches_to_try}")

        except Exception as e:
            logger.error(f"Error accessing repository: {e}")
            return []

        data = []
        skipped_count = 0

        for commit in tqdm(commits, desc="Processing commits"):
            try:
                # Skip merge commits and initial commits
                if len(commit.parents) != 1:
                    skipped_count += 1
                    continue

                # Get diff
                diff = self.repo.git.diff(commit.parents[0], commit, unified=0)

                if not diff.strip():
                    skipped_count += 1
                    continue

                # Clean commit message
                commit_message = self._clean_commit_message(commit.message)

                if not commit_message:
                    skipped_count += 1
                    continue

                # Clean and truncate diff if too long
                diff = self._clean_diff(diff)

                data.append({
                    "diff": diff,
                    "commit_message": commit_message,
                    "commit_hash": commit.hexsha,
                    "author": commit.author.name,
                    "date": commit.authored_datetime.isoformat()
                })

            except Exception as e:
                logger.warning(f"Error processing commit {commit.hexsha}: {e}")
                continue

        logger.info(f"Extracted {len(data)} valid commit-diff pairs, skipped {skipped_count} commits")
        return data

    def _clean_commit_message(self, message: str) -> str:
        """Clean and normalize commit message."""
        if not message:
            return ""

        # Remove leading/trailing whitespace
        message = message.strip()

        # Remove common prefixes like "fix:", "feat:", etc. if they're just prefixes
        # But keep them if they're part of meaningful content
        message = re.sub(r'^\s*(fix|feat|docs?|style|refactor|test|chore|perf|ci|build|revert):\s*', '', message, flags=re.IGNORECASE)

        # Remove multiple consecutive newlines
        message = re.sub(r'\n\s*\n', '\n', message)

        # Limit length
        if len(message) > 200:
            message = message[:200] + "..."

        return message.strip()

    def _clean_diff(self, diff: str) -> str:
        """Clean and truncate diff content."""
        # Remove binary files from diff
        lines = diff.split('\n')
        cleaned_lines = []

        skip_binary = False
        for line in lines:
            if line.startswith('diff --git'):
                skip_binary = False
            elif 'Binary files' in line or 'GIT binary patch' in line:
                skip_binary = True
                continue

            if not skip_binary:
                cleaned_lines.append(line)

        diff = '\n'.join(cleaned_lines)

        # Truncate very long diffs (keep first and last parts)
        if len(diff) > 10000:
            lines = diff.split('\n')
            if len(lines) > 100:
                # Keep first 50 and last 50 lines
                diff = '\n'.join(lines[:50] + ['... (diff truncated) ...'] + lines[-50:])

        return diff.strip()


class HuggingFaceDatasetLoader:
    """Load existing datasets from Hugging Face."""

    @staticmethod
    def load_git_commit_dataset(dataset_name: str = "Tavernari/git-commit-message-dt") -> List[Dict[str, str]]:
        """
        Load git commit diff dataset from Hugging Face.

        Args:
            dataset_name: Name of the Hugging Face dataset

        Returns:
            List of dictionaries containing diff and commit_message
        """
        logger.info(f"Loading dataset: {dataset_name}")

        try:
            dataset = load_dataset(dataset_name, split='train')
            logger.info(f"Loaded {len(dataset)} samples from {dataset_name}")

            data = []
            for item in tqdm(dataset, desc="Processing dataset"):
                # Handle different dataset formats
                diff = item.get('diff', item.get('patch', ''))
                commit_message = item.get('message', item.get('commit_message', ''))

                if diff and commit_message:
                    data.append({
                        "diff": str(diff),
                        "commit_message": str(commit_message)
                    })

            return data

        except Exception as e:
            logger.error(f"Error loading dataset {dataset_name}: {e}")
            return []


class DatasetFormatter:
    """Format dataset for training."""

    @staticmethod
    def format_for_training(data: List[Dict[str, str]], output_format: str = "instruction") -> List[Dict[str, str]]:
        """
        Format the dataset for training.

        Args:
            data: Raw data with diff and commit_message
            output_format: Format type ("instruction" or "simple")

        Returns:
            Formatted data ready for training
        """
        formatted_data = []

        for item in data:
            if output_format == "instruction":
                # Instruction tuning format
                text = f"### Input:\n{item['diff']}\n\n### Output:\n{item['commit_message']}"
            else:
                # Simple format
                text = f"{item['diff']}\n\n{item['commit_message']}"

            formatted_data.append({
                "text": text,
                **item  # Keep original fields
            })

        return formatted_data

    @staticmethod
    def save_to_jsonl(data: List[Dict[str, str]], output_file: str):
        """Save data to JSONL format."""
        logger.info(f"Saving {len(data)} samples to {output_file}")

        with open(output_file, 'w', encoding='utf-8') as f:
            for item in data:
                json.dump(item, f, ensure_ascii=False)
                f.write('\n')

        logger.info(f"Dataset saved to {output_file}")


def main():
    parser = argparse.ArgumentParser(description="Prepare git diff to commit message dataset")
    parser.add_argument("--repo-path", type=str, help="Path to git repository")
    parser.add_argument("--branch", type=str, default="main", help="Branch to extract from")
    parser.add_argument("--max-commits", type=int, default=5000, help="Maximum commits to extract")
    parser.add_argument("--use-huggingface-dataset", action="store_true", help="Use Hugging Face dataset instead of local repo")
    parser.add_argument("--hf-dataset-name", type=str, default="Tavernari/git-commit-message-dt", help="Hugging Face dataset name")
    parser.add_argument("--output", type=str, required=True, help="Output JSONL file path")
    parser.add_argument("--format", type=str, choices=["instruction", "simple"], default="instruction", help="Output format")

    args = parser.parse_args()

    # Extract data
    if args.use_huggingface_dataset:
        logger.info("Using Hugging Face dataset")
        loader = HuggingFaceDatasetLoader()
        raw_data = loader.load_git_commit_dataset(args.hf_dataset_name)
    else:
        if not args.repo_path:
            parser.error("--repo-path is required when not using --use-huggingface-dataset")
        logger.info(f"Extracting from local repository: {args.repo_path}")
        extractor = GitDiffExtractor(args.repo_path, args.max_commits)
        raw_data = extractor.extract_commits(args.branch)

    if not raw_data:
        logger.error("No data extracted!")
        return

    # Format data
    logger.info("Formatting data for training")
    formatter = DatasetFormatter()
    formatted_data = formatter.format_for_training(raw_data, args.format)

    # Save data
    formatter.save_to_jsonl(formatted_data, args.output)

    logger.info(f"Dataset preparation completed! Total samples: {len(formatted_data)}")


if __name__ == "__main__":
    main()
