#!/usr/bin/env python3
"""
Git Diff to Commit Message Generator Scripts

This module allows running the pipeline scripts as a module.

Usage:
    python -m scripts --help
    python -m scripts run-pipeline --config config/config.yaml --stage all
"""

import sys
import argparse
from pathlib import Path

# Add parent directory to path for imports
sys.path.insert(0, str(Path(__file__).parent.parent))

def main():
    parser = argparse.ArgumentParser(
        description="Git Diff to Commit Message Generator Scripts",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  python -m scripts run-pipeline --config config/config.yaml --stage all
  python -m scripts data-prep --repo-path /path/to/repo --output data.jsonl
  python -m scripts train --data-path data.jsonl --output-dir models/
  python -m scripts inference --model-path models/ --diff-file changes.diff
        """
    )

    subparsers = parser.add_subparsers(dest='command', help='Available commands')

    # Run pipeline command
    pipeline_parser = subparsers.add_parser('run-pipeline', help='Run complete pipeline')
    pipeline_parser.add_argument('--config', type=str, default='config/config.yaml', help='Configuration file path')
    pipeline_parser.add_argument('--stage', type=str, choices=['data', 'train', 'inference', 'all'],
                                default='all', help='Pipeline stage to run')
    pipeline_parser.add_argument('--validate-only', action='store_true', help='Only validate environment')

    # Data preparation command
    data_parser = subparsers.add_parser('data-prep', help='Prepare training data')
    data_parser.add_argument('--repo-path', type=str, help='Git repository path')
    data_parser.add_argument('--use-huggingface-dataset', action='store_true', help='Use Hugging Face dataset')
    data_parser.add_argument('--hf-dataset-name', type=str, default='codeparrot/git-commit-diffs', help='Hugging Face dataset name')
    data_parser.add_argument('--output', type=str, required=True, help='Output JSONL file path')
    data_parser.add_argument('--max-commits', type=int, default=5000, help='Maximum commits to extract')
    data_parser.add_argument('--branch', type=str, default='main', help='Branch to extract from')
    data_parser.add_argument('--format', type=str, choices=['instruction', 'simple'], default='instruction', help='Output format')

    # Training command
    train_parser = subparsers.add_parser('train', help='Train the model')
    train_parser.add_argument('--data-path', type=str, required=True, help='Path to training data JSONL file')
    train_parser.add_argument('--output-dir', type=str, required=True, help='Output directory for trained model')
    train_parser.add_argument('--model-name', type=str, default='codellama/CodeLlama-7b-hf', help='Base model name')
    train_parser.add_argument('--quantization-bits', type=int, choices=[4, 8], default=4, help='Quantization bits')
    train_parser.add_argument('--lora-r', type=int, default=8, help='LoRA rank')
    train_parser.add_argument('--lora-alpha', type=int, default=32, help='LoRA alpha')
    train_parser.add_argument('--max-length', type=int, default=1024, help='Maximum sequence length')
    train_parser.add_argument('--batch-size', type=int, default=1, help='Per device batch size')
    train_parser.add_argument('--gradient-accumulation', type=int, default=4, help='Gradient accumulation steps')
    train_parser.add_argument('--learning-rate', type=float, default=2e-4, help='Learning rate')
    train_parser.add_argument('--num-epochs', type=int, default=3, help='Number of training epochs')
    train_parser.add_argument('--use-wandb', action='store_true', help='Use Weights & Biases logging')
    train_parser.add_argument('--wandb-project', type=str, default='git-commit-model', help='Wandb project name')

    # Inference command
    inference_parser = subparsers.add_parser('inference', help='Run inference')
    inference_parser.add_argument('--model-path', type=str, help='Path to fine-tuned model directory')
    inference_parser.add_argument('--base-model', type=str, default='codellama/CodeLlama-7b-hf', help='Base model name')
    inference_parser.add_argument('--diff-file', type=str, help='Path to git diff file')
    inference_parser.add_argument('--diff-content', type=str, help='Git diff content as string')
    inference_parser.add_argument('--output-file', type=str, help='Output file for commit message')
    inference_parser.add_argument('--quantization-bits', type=int, choices=[4, 8], default=4, help='Quantization bits')
    inference_parser.add_argument('--interactive', action='store_true', help='Run in interactive mode')
    inference_parser.add_argument('--num-candidates', type=int, default=1, help='Number of candidates to generate')
    inference_parser.add_argument('--temperature', type=float, default=0.7, help='Generation temperature')
    inference_parser.add_argument('--max-tokens', type=int, default=100, help='Maximum new tokens')
    inference_parser.add_argument('--top-p', type=float, default=0.9, help='Top-p sampling parameter')
    inference_parser.add_argument('--top-k', type=int, default=50, help='Top-k sampling parameter')

    args = parser.parse_args()

    if not args.command:
        parser.print_help()
        return

    try:
        if args.command == 'run-pipeline':
            from .run_pipeline import main as run_pipeline_main
            # Convert args to sys.argv format
            sys.argv = ['run_pipeline.py', '--config', args.config, '--stage', args.stage]
            if args.validate_only:
                sys.argv.append('--validate-only')
            run_pipeline_main()

        elif args.command == 'data-prep':
            from .data_preparation import main as data_prep_main
            # Convert args to sys.argv format
            sys.argv = ['data_preparation.py']
            if args.repo_path:
                sys.argv.extend(['--repo-path', args.repo_path])
            elif args.use_huggingface_dataset:
                sys.argv.append('--use-huggingface-dataset')
                sys.argv.extend(['--hf-dataset-name', args.hf_dataset_name])
            else:
                print("Error: Must specify --repo-path or --use-huggingface-dataset")
                return
            sys.argv.extend([
                '--output', args.output,
                '--max-commits', str(args.max_commits),
                '--branch', args.branch,
                '--format', args.format
            ])
            data_prep_main()

        elif args.command == 'train':
            from .train_model import main as train_main
            # Convert args to sys.argv format
            sys.argv = ['train_model.py']
            for key, value in vars(args).items():
                if key in ['command']:
                    continue
                if value is not None and value is not False:
                    if isinstance(value, bool):
                        sys.argv.append(f'--{key.replace("_", "-")}')
                    else:
                        sys.argv.extend([f'--{key.replace("_", "-")}', str(value)])
            train_main()

        elif args.command == 'inference':
            from .inference import main as inference_main
            # Convert args to sys.argv format
            sys.argv = ['inference.py']
            for key, value in vars(args).items():
                if key in ['command']:
                    continue
                if value is not None and value is not False:
                    if isinstance(value, bool):
                        sys.argv.append(f'--{key.replace("_", "-")}')
                    else:
                        sys.argv.extend([f'--{key.replace("_", "-")}', str(value)])
            inference_main()

    except Exception as e:
        print(f"Error running command '{args.command}': {e}")
        sys.exit(1)

if __name__ == '__main__':
    main()
