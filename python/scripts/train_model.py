#!/usr/bin/env python3
"""
Git Diff to Commit Message Model Training Script

This script fine-tunes a CodeLlama model using QLoRA to generate commit messages
from git diff content. Optimized for RTX 4050 Laptop GPU (6GB VRAM).

Usage:
    python train_model.py --data-path data.jsonl --output-dir ./models/git-commit-model
"""

import os
import torch
import logging
import argparse
from typing import Optional, Dict, Any
from pathlib import Path

try:
    from transformers import (
        AutoModelForCausalLM,
        AutoTokenizer,
        TrainingArguments,
        Trainer,
        DataCollatorForLanguageModeling,
        BitsAndBytesConfig
    )
    from peft import LoraConfig, get_peft_model, prepare_model_for_kbit_training, PeftModel
    from datasets import load_dataset
    import wandb
except ImportError as e:
    print(f"Missing required dependencies. Please install them: {e}")
    exit(1)

# Configure logging
logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')
logger = logging.getLogger(__name__)


class CommitMessageTrainer:
    """Trainer class for git diff to commit message model."""

    def __init__(self,
                 model_name: str = "codellama/CodeLlama-7b-hf",
                 quantization_bits: int = 4,
                 lora_r: int = 8,
                 lora_alpha: int = 32,
                 max_length: int = 1024):
        """
        Initialize the trainer.

        Args:
            model_name: Hugging Face model name
            quantization_bits: Quantization bits (4 or 8)
            lora_r: LoRA rank
            lora_alpha: LoRA alpha parameter
            max_length: Maximum sequence length
        """
        self.model_name = model_name
        self.quantization_bits = quantization_bits
        self.lora_r = lora_r
        self.lora_alpha = lora_alpha
        self.max_length = max_length

        self.model = None
        self.tokenizer = None
        self.device = torch.device("cuda" if torch.cuda.is_available() else "cpu")

        logger.info(f"Using device: {self.device}")
        if torch.cuda.is_available():
            logger.info(f"GPU: {torch.cuda.get_device_name(0)}")
            logger.info(f"GPU Memory: {torch.cuda.get_device_properties(0).total_memory / 1024**3:.1f} GB")

    def load_model_and_tokenizer(self):
        """Load model and tokenizer with quantization."""
        logger.info(f"Loading model: {self.model_name}")

        # Configure quantization
        if self.quantization_bits == 4:
            quantization_config = BitsAndBytesConfig(
                load_in_4bit=True,
                bnb_4bit_compute_dtype=torch.bfloat16,
                bnb_4bit_use_double_quant=True,
                bnb_4bit_quant_type="nf4"
            )
        elif self.quantization_bits == 8:
            quantization_config = BitsAndBytesConfig(
                load_in_8bit=True,
                bnb_8bit_compute_dtype=torch.bfloat16
            )
        else:
            quantization_config = None

        # Load tokenizer
        self.tokenizer = AutoTokenizer.from_pretrained(self.model_name)
        self.tokenizer.pad_token = self.tokenizer.eos_token
        self.tokenizer.padding_side = "right"

        # Load model
        self.model = AutoModelForCausalLM.from_pretrained(
            self.model_name,
            quantization_config=quantization_config,
            device_map="auto",
            torch_dtype=torch.bfloat16 if quantization_config else torch.float16,
            trust_remote_code=True
        )

        # Prepare model for k-bit training if using quantization
        if quantization_config:
            self.model = prepare_model_for_kbit_training(self.model)

        # Configure LoRA
        lora_config = LoraConfig(
            r=self.lora_r,
            lora_alpha=self.lora_alpha,
            target_modules=["q_proj", "k_proj", "v_proj", "o_proj"],  # CodeLlama attention modules
            lora_dropout=0.05,
            bias="none",
            task_type="CAUSAL_LM"
        )

        self.model = get_peft_model(self.model, lora_config)

        # Print trainable parameters info
        self.model.print_trainable_parameters()

        logger.info("Model and tokenizer loaded successfully")

    def load_and_preprocess_dataset(self, data_path: str, test_size: float = 0.1):
        """Load and preprocess the dataset."""
        logger.info(f"Loading dataset from: {data_path}")

        # Load dataset
        dataset = load_dataset("json", data_files=data_path)

        # Tokenization function
        def tokenize_function(examples):
            return self.tokenizer(
                examples["text"],
                truncation=True,
                max_length=self.max_length,
                padding="max_length"
            )

        # Tokenize dataset
        logger.info("Tokenizing dataset...")
        tokenized_dataset = dataset.map(
            tokenize_function,
            batched=True,
            remove_columns=["text", "diff", "commit_message"],  # Remove original columns
            desc="Tokenizing"
        )

        # Split into train/test
        split_dataset = tokenized_dataset["train"].train_test_split(
            test_size=test_size,
            seed=42
        )

        logger.info(f"Dataset split - Train: {len(split_dataset['train'])}, Test: {len(split_dataset['test'])}")
        return split_dataset

    def train(self,
              train_dataset,
              eval_dataset,
              output_dir: str,
              batch_size: int = 1,
              gradient_accumulation_steps: int = 4,
              learning_rate: float = 2e-4,
              num_epochs: int = 3,
              save_steps: int = 500,
              logging_steps: int = 10,
              use_wandb: bool = False):
        """
        Train the model.

        Args:
            train_dataset: Training dataset
            eval_dataset: Evaluation dataset
            output_dir: Output directory for saving model
            batch_size: Per device batch size
            gradient_accumulation_steps: Gradient accumulation steps
            learning_rate: Learning rate
            num_epochs: Number of training epochs
            save_steps: Save checkpoint every N steps
            logging_steps: Log every N steps
            use_wandb: Whether to use Weights & Biases logging
        """
        logger.info("Starting training...")

        # Create output directory
        output_path = Path(output_dir)
        output_path.mkdir(parents=True, exist_ok=True)

        # Configure training arguments
        training_args = TrainingArguments(
            output_dir=str(output_path),
            per_device_train_batch_size=batch_size,
            per_device_eval_batch_size=batch_size,
            gradient_accumulation_steps=gradient_accumulation_steps,
            learning_rate=learning_rate,
            num_train_epochs=num_epochs,
            logging_steps=logging_steps,
            evaluation_strategy="steps",
            eval_steps=save_steps,
            save_strategy="steps",
            save_steps=save_steps,
            save_total_limit=2,  # Keep only last 2 checkpoints
            load_best_model_at_end=True,
            metric_for_best_model="eval_loss",
            greater_is_better=False,
            fp16=True,  # Use mixed precision
            bf16=False,  # RTX 4050 supports fp16 better than bf16
            optim="paged_adamw_8bit",  # 8-bit optimizer for memory efficiency
            report_to="wandb" if use_wandb else "none",
            run_name=f"git-commit-{self.model_name.split('/')[-1]}" if use_wandb else None,
            dataloader_pin_memory=False,  # Avoid memory issues
            dataloader_num_workers=0,  # Avoid multiprocessing issues in some environments
        )

        # Data collator for causal language modeling
        data_collator = DataCollatorForLanguageModeling(
            tokenizer=self.tokenizer,
            mlm=False  # Causal LM, not masked LM
        )

        # Initialize trainer
        trainer = Trainer(
            model=self.model,
            args=training_args,
            train_dataset=train_dataset,
            eval_dataset=eval_dataset,
            data_collator=data_collator,
        )

        # Start training
        trainer.train()

        # Save the final model
        logger.info(f"Saving model to {output_path}")
        trainer.save_model(str(output_path))
        self.tokenizer.save_pretrained(str(output_path))

        # Save training metrics
        if trainer.state.log_history:
            import json
            metrics_file = output_path / "training_metrics.json"
            with open(metrics_file, 'w') as f:
                json.dump(trainer.state.log_history, f, indent=2)

        logger.info("Training completed!")
        return trainer


def main():
    parser = argparse.ArgumentParser(description="Train git diff to commit message model")
    parser.add_argument("--data-path", type=str, required=True, help="Path to training data JSONL file")
    parser.add_argument("--model-name", type=str, default="codellama/CodeLlama-7b-hf", help="Base model name")
    parser.add_argument("--output-dir", type=str, required=True, help="Output directory for trained model")
    parser.add_argument("--quantization-bits", type=int, choices=[4, 8, None], default=4, help="Quantization bits")
    parser.add_argument("--lora-r", type=int, default=8, help="LoRA rank")
    parser.add_argument("--lora-alpha", type=int, default=32, help="LoRA alpha")
    parser.add_argument("--max-length", type=int, default=1024, help="Maximum sequence length")
    parser.add_argument("--batch-size", type=int, default=1, help="Per device batch size")
    parser.add_argument("--gradient-accumulation", type=int, default=4, help="Gradient accumulation steps")
    parser.add_argument("--learning-rate", type=float, default=2e-4, help="Learning rate")
    parser.add_argument("--num-epochs", type=int, default=3, help="Number of training epochs")
    parser.add_argument("--use-wandb", action="store_true", help="Use Weights & Biases logging")
    parser.add_argument("--wandb-project", type=str, default="git-commit-model", help="Wandb project name")

    args = parser.parse_args()

    # Set quantization_bits to None if not specified
    if args.quantization_bits is None:
        args.quantization_bits = None

    # Initialize wandb if requested
    if args.use_wandb:
        wandb.init(project=args.wandb_project, config=vars(args))

    try:
        # Initialize trainer
        trainer = CommitMessageTrainer(
            model_name=args.model_name,
            quantization_bits=args.quantization_bits,
            lora_r=args.lora_r,
            lora_alpha=args.lora_alpha,
            max_length=args.max_length
        )

        # Load model and tokenizer
        trainer.load_model_and_tokenizer()

        # Load and preprocess dataset
        dataset = trainer.load_and_preprocess_dataset(args.data_path)

        # Train model
        trainer.train(
            train_dataset=dataset["train"],
            eval_dataset=dataset["test"],
            output_dir=args.output_dir,
            batch_size=args.batch_size,
            gradient_accumulation_steps=args.gradient_accumulation,
            learning_rate=args.learning_rate,
            num_epochs=args.num_epochs,
            use_wandb=args.use_wandb
        )

        logger.info(f"Model training completed! Saved to: {args.output_dir}")

    except Exception as e:
        logger.error(f"Training failed: {e}")
        raise


if __name__ == "__main__":
    main()
