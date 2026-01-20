#!/usr/bin/env python3
"""
Utility functions for git diff to commit message project.

This module provides helper functions for data processing, validation,
evaluation, and other common operations.
"""

import os
import re
import json
import yaml
import logging
from pathlib import Path
from typing import Dict, List, Optional, Tuple, Any
from dataclasses import dataclass

try:
    import git
    from transformers import AutoTokenizer
except ImportError:
    pass  # Will be handled by calling functions

logger = logging.getLogger(__name__)


@dataclass
class DiffStats:
    """Statistics about a git diff."""
    files_changed: int = 0
    insertions: int = 0
    deletions: int = 0
    lines_changed: int = 0
    binary_files: int = 0
    new_files: int = 0
    deleted_files: int = 0
    modified_files: int = 0


class GitDiffProcessor:
    """Process and analyze git diff content."""

    @staticmethod
    def parse_diff_stats(diff_content: str) -> DiffStats:
        """Parse basic statistics from git diff content."""
        stats = DiffStats()
        lines = diff_content.split('\n')

        current_file = None
        in_diff = False

        for line in lines:
            if line.startswith('diff --git'):
                stats.files_changed += 1
                current_file = line.split()[-1]  # Get the file path
                in_diff = True

            elif line.startswith('new file mode'):
                stats.new_files += 1
            elif line.startswith('deleted file mode'):
                stats.deleted_files += 1
            elif in_diff and line.startswith('+++') and not line.endswith('/dev/null'):
                if current_file and not current_file.endswith('/dev/null'):
                    stats.modified_files += 1

            elif line.startswith('Binary files'):
                stats.binary_files += 1

            elif line.startswith('+') and not line.startswith('+++'):
                stats.insertions += 1
                stats.lines_changed += 1
            elif line.startswith('-') and not line.startswith('---'):
                stats.deletions += 1
                stats.lines_changed += 1

        return stats

    @staticmethod
    def clean_diff_content(diff_content: str, max_lines: int = 1000) -> str:
        """Clean and truncate diff content for processing."""
        lines = diff_content.split('\n')

        # Remove binary file diffs
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

        # Truncate if too long
        if len(cleaned_lines) > max_lines:
            truncated = cleaned_lines[:max_lines//2] + \
                       ['... (diff truncated due to length) ...'] + \
                       cleaned_lines[-(max_lines//2):]
            cleaned_lines = truncated

        return '\n'.join(cleaned_lines)

    @staticmethod
    def extract_file_types(diff_content: str) -> List[str]:
        """Extract file types from diff content."""
        file_types = set()
        lines = diff_content.split('\n')

        for line in lines:
            if line.startswith('diff --git'):
                # Extract file extension
                parts = line.split()
                if len(parts) >= 3:
                    file_path = parts[-1]  # Usually the target file
                    if '.' in file_path:
                        ext = file_path.split('.')[-1].lower()
                        if ext and len(ext) < 10:  # Reasonable extension length
                            file_types.add(ext)

        return sorted(list(file_types))

    @staticmethod
    def is_meaningful_diff(diff_content: str, min_changes: int = 1) -> bool:
        """Check if diff contains meaningful changes."""
        if not diff_content.strip():
            return False

        lines = diff_content.split('\n')
        change_count = 0

        for line in lines:
            if line.startswith(('+', '-')) and not line.startswith(('+++', '---')):
                change_count += 1
                if change_count >= min_changes:
                    return True

        return False


class CommitMessageProcessor:
    """Process and analyze commit messages."""

    @staticmethod
    def clean_commit_message(message: str) -> str:
        """Clean and normalize commit message."""
        if not message:
            return ""

        # Remove leading/trailing whitespace and newlines
        message = message.strip()

        # Remove conventional commit prefixes if they're just noise
        message = re.sub(r'^\s*(fix|feat|docs?|style|refactor|test|chore|perf|ci|build|revert):\s*',
                        '', message, flags=re.IGNORECASE)

        # Remove multiple consecutive spaces
        message = re.sub(r'\s+', ' ', message)

        # Remove excessive punctuation
        message = re.sub(r'[.!?]+$', '.', message)

        # Capitalize first letter
        if message and not message[0].isupper():
            message = message[0].upper() + message[1:]

        return message.strip()

    @staticmethod
    def analyze_commit_quality(message: str) -> Dict[str, Any]:
        """Analyze the quality of a commit message."""
        analysis = {
            'length': len(message),
            'has_period': '.' in message,
            'starts_capital': message and message[0].isupper(),
            'has_imperative': False,
            'too_short': len(message) < 10,
            'too_long': len(message) > 200,
            'has_numbers': bool(re.search(r'\d', message)),
            'has_special_chars': bool(re.search(r'[^a-zA-Z0-9\s.,!?-]', message)),
        }

        # Check for imperative mood (common commit verbs)
        imperative_verbs = ['add', 'fix', 'update', 'remove', 'change', 'refactor',
                          'improve', 'modify', 'create', 'delete', 'merge', 'revert']
        first_word = message.lower().split()[0] if message else ""
        analysis['has_imperative'] = first_word in imperative_verbs

        # Calculate quality score (0-10)
        score = 10
        if analysis['too_short']: score -= 3
        if analysis['too_long']: score -= 2
        if not analysis['starts_capital']: score -= 1
        if not analysis['has_imperative']: score -= 1
        if analysis['has_special_chars'] and not analysis['has_numbers']: score -= 1

        analysis['quality_score'] = max(0, score)

        return analysis

    @staticmethod
    def categorize_commit_type(message: str) -> str:
        """Categorize commit by type based on content."""
        message_lower = message.lower()

        categories = {
            'bug_fix': ['fix', 'bug', 'error', 'issue', 'problem'],
            'feature': ['add', 'new', 'feature', 'implement'],
            'refactor': ['refactor', 'clean', 'restructure', 'optimize'],
            'docs': ['doc', 'readme', 'comment', 'documentation'],
            'test': ['test', 'spec', 'assert'],
            'style': ['style', 'format', 'lint', 'prettier'],
            'chore': ['chore', 'build', 'ci', 'config', 'dependency'],
            'performance': ['perf', 'performance', 'speed', 'optimize'],
            'security': ['security', 'auth', 'encrypt', 'vulnerability']
        }

        for category, keywords in categories.items():
            if any(keyword in message_lower for keyword in keywords):
                return category

        return 'other'


class DataValidator:
    """Validate and check data quality."""

    @staticmethod
    def validate_diff_commit_pair(diff: str, commit_message: str) -> Dict[str, Any]:
        """Validate a diff-commit message pair."""
        issues = []

        # Check diff
        if not diff or not diff.strip():
            issues.append("empty_diff")
        elif not GitDiffProcessor.is_meaningful_diff(diff):
            issues.append("meaningless_diff")
        elif len(diff) > 50000:  # Too long
            issues.append("diff_too_long")

        # Check commit message
        if not commit_message or not commit_message.strip():
            issues.append("empty_commit_message")
        elif len(commit_message) < 5:
            issues.append("commit_message_too_short")
        elif len(commit_message) > 300:
            issues.append("commit_message_too_long")

        # Check for common issues
        if commit_message and commit_message.count('.') > 3:
            issues.append("too_many_sentences")

        return {
            'valid': len(issues) == 0,
            'issues': issues,
            'diff_stats': GitDiffProcessor.parse_diff_stats(diff) if diff else None,
            'commit_analysis': CommitMessageProcessor.analyze_commit_quality(commit_message) if commit_message else None
        }

    @staticmethod
    def validate_dataset(data: List[Dict[str, str]]) -> Dict[str, Any]:
        """Validate an entire dataset."""
        total_samples = len(data)
        valid_samples = 0
        issues_count = {}

        for item in data:
            diff = item.get('diff', '')
            commit_message = item.get('commit_message', '')

            validation = DataValidator.validate_diff_commit_pair(diff, commit_message)
            if validation['valid']:
                valid_samples += 1
            else:
                for issue in validation['issues']:
                    issues_count[issue] = issues_count.get(issue, 0) + 1

        return {
            'total_samples': total_samples,
            'valid_samples': valid_samples,
            'invalid_samples': total_samples - valid_samples,
            'validity_rate': valid_samples / total_samples if total_samples > 0 else 0,
            'common_issues': sorted(issues_count.items(), key=lambda x: x[1], reverse=True)
        }


class ConfigManager:
    """Manage configuration files."""

    @staticmethod
    def load_config(config_path: str) -> Dict[str, Any]:
        """Load configuration from YAML or JSON file."""
        path = Path(config_path)
        if not path.exists():
            raise FileNotFoundError(f"Config file not found: {config_path}")

        with open(path, 'r', encoding='utf-8') as f:
            if path.suffix.lower() in ['.yaml', '.yml']:
                return yaml.safe_load(f)
            elif path.suffix.lower() == '.json':
                return json.load(f)
            else:
                raise ValueError(f"Unsupported config format: {path.suffix}")

    @staticmethod
    def save_config(config: Dict[str, Any], config_path: str):
        """Save configuration to file."""
        path = Path(config_path)
        path.parent.mkdir(parents=True, exist_ok=True)

        with open(path, 'w', encoding='utf-8') as f:
            if path.suffix.lower() in ['.yaml', '.yml']:
                yaml.dump(config, f, default_flow_style=False, indent=2)
            elif path.suffix.lower() == '.json':
                json.dump(config, f, indent=2)
            else:
                raise ValueError(f"Unsupported config format: {path.suffix}")


class ModelEvaluator:
    """Evaluate model performance."""

    def __init__(self, tokenizer_name: str = "codellama/CodeLlama-7b-hf"):
        """Initialize evaluator with tokenizer."""
        try:
            self.tokenizer = AutoTokenizer.from_pretrained(tokenizer_name)
        except Exception as e:
            logger.warning(f"Could not load tokenizer: {e}")
            self.tokenizer = None

    def calculate_bleu_score(self, predictions: List[str], references: List[str]) -> float:
        """Calculate BLEU score between predictions and references."""
        try:
            from nltk.translate.bleu_score import sentence_bleu
        except ImportError:
            logger.warning("nltk not available for BLEU calculation")
            return 0.0

        if len(predictions) != len(references):
            raise ValueError("Predictions and references must have same length")

        scores = []
        for pred, ref in zip(predictions, references):
            # Tokenize for BLEU
            pred_tokens = pred.lower().split()
            ref_tokens = ref.lower().split()
            score = sentence_bleu([ref_tokens], pred_tokens)
            scores.append(score)

        return sum(scores) / len(scores) if scores else 0.0

    def calculate_rouge_score(self, predictions: List[str], references: List[str]) -> Dict[str, float]:
        """Calculate ROUGE scores."""
        try:
            from rouge_score import rouge_scorer
        except ImportError:
            logger.warning("rouge-score not available for ROUGE calculation")
            return {}

        scorer = rouge_scorer.RougeScorer(['rouge1', 'rouge2', 'rougeL'], use_stemmer=True)

        scores = {'rouge1': [], 'rouge2': [], 'rougeL': []}

        for pred, ref in zip(predictions, references):
            score = scorer.score(ref, pred)
            for key in scores.keys():
                scores[key].append(score[key].fmeasure)

        return {key: sum(values) / len(values) for key, values in scores.items()}

    def analyze_generation_diversity(self, predictions: List[str]) -> Dict[str, float]:
        """Analyze diversity of generated text."""
        if not predictions:
            return {}

        # Calculate average length
        lengths = [len(pred.split()) for pred in predictions]
        avg_length = sum(lengths) / len(lengths)

        # Calculate unique n-grams (simple diversity metric)
        unigrams = set()
        bigrams = set()

        for pred in predictions:
            tokens = pred.lower().split()
            unigrams.update(tokens)
            bigrams.update([f"{tokens[i]}_{tokens[i+1]}" for i in range(len(tokens)-1)])

        return {
            'avg_length': avg_length,
            'unique_unigrams': len(unigrams),
            'unique_bigrams': len(bigrams),
            'diversity_ratio': len(unigrams) / sum(lengths) if sum(lengths) > 0 else 0
        }


def setup_logging(log_level: str = "INFO", log_file: Optional[str] = None):
    """Setup logging configuration."""
    level_map = {
        'DEBUG': logging.DEBUG,
        'INFO': logging.INFO,
        'WARNING': logging.WARNING,
        'ERROR': logging.ERROR,
        'CRITICAL': logging.CRITICAL
    }

    level = level_map.get(log_level.upper(), logging.INFO)

    handlers = [logging.StreamHandler()]
    if log_file:
        handlers.append(logging.FileHandler(log_file))

    logging.basicConfig(
        level=level,
        format='%(asctime)s - %(name)s - %(levelname)s - %(message)s',
        handlers=handlers
    )


def get_project_root() -> Path:
    """Get the project root directory."""
    current_file = Path(__file__).resolve()
    return current_file.parent.parent


def ensure_dir(path: str) -> Path:
    """Ensure directory exists, create if necessary."""
    dir_path = Path(path)
    dir_path.mkdir(parents=True, exist_ok=True)
    return dir_path


def format_bytes(size: int) -> str:
    """Format bytes to human readable format."""
    for unit in ['B', 'KB', 'MB', 'GB']:
        if size < 1024.0:
            return ".1f"
        size /= 1024.0
    return ".1f"
