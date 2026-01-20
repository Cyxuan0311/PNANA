#!/usr/bin/env python3
"""
Git Diff to Commit Message Pipeline Runner

This script provides a convenient way to run the complete pipeline:
1. Data preparation
2. Model training
3. Inference testing

Usage:
    python run_pipeline.py --config config/config.yaml --stage all
    python run_pipeline.py --config config/config.yaml --stage data
    python run_pipeline.py --config config/config.yaml --stage train
    python run_pipeline.py --config config/config.yaml --stage inference
"""

import os
import sys
import argparse
import logging
from pathlib import Path
from typing import Dict, Any

# Add parent directory to path for imports
sys.path.append(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

try:
    from utils import ConfigManager, setup_logging, ensure_dir
except ImportError as e:
    print(f"Error importing utils: {e}")
    sys.exit(1)

# Setup logging
setup_logging()
logger = logging.getLogger(__name__)


class PipelineRunner:
    """Run the complete git diff to commit message pipeline."""

    def __init__(self, config_path: str):
        """Initialize with configuration."""
        self.config = ConfigManager.load_config(config_path)
        self.config_path = config_path

        # Create necessary directories
        ensure_dir(self.config['paths']['data_dir'])
        ensure_dir(self.config['paths']['logs_dir'])

        logger.info("Pipeline initialized with config: %s", config_path)

    def run_data_preparation(self):
        """Run data preparation stage."""
        logger.info("Starting data preparation...")

        from .data_preparation import main as data_prep_main

        # Prepare command line arguments
        args = [
            "--output", f"{self.config['paths']['data_dir']}/train_data.jsonl",
            "--max-commits", str(self.config['data_prep']['max_commits']),
            "--branch", self.config['data_prep']['branch'],
            "--format", self.config['data_prep']['output_format']
        ]

        if self.config['data_prep']['repo_path']:
            args.extend(["--repo-path", self.config['data_prep']['repo_path']])
        else:
            args.append("--use-huggingface-dataset")
            if self.config['data_prep']['hf_dataset_name']:
                args.extend(["--hf-dataset-name", self.config['data_prep']['hf_dataset_name']])

        # Run data preparation
        sys.argv = ["data_preparation.py"] + args
        try:
            data_prep_main()
            logger.info("Data preparation completed successfully")
            return True
        except Exception as e:
            logger.error(f"Data preparation failed: {e}")
            return False

    def run_training(self):
        """Run model training stage."""
        logger.info("Starting model training...")

        from .train_model import main as train_main

        # Prepare command line arguments
        args = [
            "--data-path", f"{self.config['paths']['data_dir']}/train_data.jsonl",
            "--output-dir", self.config['paths']['output_dir'],
            "--model-name", self.config['model']['name'],
            "--quantization-bits", str(self.config['model']['quantization_bits']),
            "--lora-r", str(self.config['model']['lora_r']),
            "--lora-alpha", str(self.config['model']['lora_alpha']),
            "--max-length", str(self.config['model']['max_length']),
            "--batch-size", str(self.config['training']['batch_size']),
            "--gradient-accumulation", str(self.config['training']['gradient_accumulation_steps']),
            "--learning-rate", str(self.config['training']['learning_rate']),
            "--num-epochs", str(self.config['training']['num_epochs'])
        ]

        if self.config['tracking']['use_wandb']:
            args.append("--use-wandb")
            if self.config['tracking']['wandb_project']:
                args.extend(["--wandb-project", self.config['tracking']['wandb_project']])

        # Run training
        sys.argv = ["train_model.py"] + args
        try:
            train_main()
            logger.info("Model training completed successfully")
            return True
        except Exception as e:
            logger.error(f"Model training failed: {e}")
            return False

    def run_inference_test(self):
        """Run inference testing."""
        logger.info("Starting inference test...")

        from .inference import main as inference_main

        # Create a simple test diff
        test_diff = """diff --git a/example.py b/example.py
index 83db48f..bf269f4 100644
--- a/example.py
+++ b/example.py
@@ -1,5 +1,8 @@
 def hello_world():
-    print("Hello")
+    print("Hello World")
+
+def add_numbers(a, b):
+    return a + b

 if __name__ == "__main__":
     hello_world()
"""

        # Save test diff
        test_diff_file = f"{self.config['paths']['data_dir']}/test_diff.diff"
        with open(test_diff_file, 'w') as f:
            f.write(test_diff)

        # Prepare command line arguments
        args = [
            "--model-path", self.config['paths']['output_dir'],
            "--diff-file", test_diff_file,
            "--temperature", str(self.config['inference']['temperature']),
            "--top-p", str(self.config['inference']['top_p']),
            "--max-tokens", str(self.config['inference']['max_new_tokens'])
        ]

        # Run inference
        sys.argv = ["inference.py"] + args
        try:
            inference_main()
            logger.info("Inference test completed successfully")
            return True
        except Exception as e:
            logger.error(f"Inference test failed: {e}")
            return False

    def validate_environment(self) -> bool:
        """Validate that the environment is ready."""
        logger.info("Validating environment...")

        try:
            import torch
            cuda_available = torch.cuda.is_available()
            logger.info(f"CUDA available: {cuda_available}")

            if cuda_available:
                logger.info(f"GPU: {torch.cuda.get_device_name(0)}")
                logger.info(f"GPU Memory: {torch.cuda.get_device_properties(0).total_memory / 1024**3:.1f} GB")
            else:
                logger.warning("CUDA not available. Training will be slow on CPU.")

            # Check if model directory exists for inference-only runs
            model_path = Path(self.config['paths']['output_dir'])
            if not model_path.exists():
                logger.warning(f"Model directory not found: {model_path}")

            return True

        except Exception as e:
            logger.error(f"Environment validation failed: {e}")
            return False

    def run_stage(self, stage: str) -> bool:
        """Run a specific pipeline stage."""
        if stage == "data":
            return self.run_data_preparation()
        elif stage == "train":
            return self.run_training()
        elif stage == "inference":
            return self.run_inference_test()
        elif stage == "all":
            success = True
            if not self.run_data_preparation():
                success = False
            if not self.run_training():
                success = False
            if not self.run_inference_test():
                success = False
            return success
        else:
            logger.error(f"Unknown stage: {stage}")
            return False


def main():
    parser = argparse.ArgumentParser(description="Run git diff to commit message pipeline")
    parser.add_argument("--config", type=str, default="config/config.yaml", help="Configuration file path")
    parser.add_argument("--stage", type=str, choices=["data", "train", "inference", "all"],
                       default="all", help="Pipeline stage to run")
    parser.add_argument("--validate-only", action="store_true", help="Only validate environment")

    args = parser.parse_args()

    try:
        # Initialize pipeline
        runner = PipelineRunner(args.config)

        # Validate environment
        if not runner.validate_environment():
            logger.error("Environment validation failed")
            sys.exit(1)

        if args.validate_only:
            logger.info("Environment validation completed successfully")
            return

        # Run requested stage
        logger.info(f"Running pipeline stage: {args.stage}")
        success = runner.run_stage(args.stage)

        if success:
            logger.info(f"Pipeline stage '{args.stage}' completed successfully!")
        else:
            logger.error(f"Pipeline stage '{args.stage}' failed!")
            sys.exit(1)

    except Exception as e:
        logger.error(f"Pipeline execution failed: {e}")
        sys.exit(1)


if __name__ == "__main__":
    main()
