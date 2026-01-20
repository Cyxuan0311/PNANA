#!/usr/bin/env python3
"""
Git Diff to Commit Message Inference Script

This script loads a trained model and generates commit messages from git diff content.

Usage:
    python inference.py --model-path ./models/git-commit-model --diff-file changes.diff
    python inference.py --model-path ./models/git-commit-model --diff-content "diff content here"
    python inference.py --model-path ./models/git-commit-model --interactive
"""

import os
import torch
import argparse
import logging
from pathlib import Path
from typing import Optional, List

try:
    from transformers import AutoModelForCausalLM, AutoTokenizer, BitsAndBytesConfig
    from peft import PeftModel
except ImportError as e:
    print(f"Missing required dependencies. Please install them: {e}")
    exit(1)

# Configure logging
logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')
logger = logging.getLogger(__name__)


class CommitMessageGenerator:
    """Generator class for creating commit messages from git diffs."""

    def __init__(self,
                 base_model_name: str = "codellama/CodeLlama-7b-hf",
                 model_path: Optional[str] = None,
                 quantization_bits: int = 4,
                 device: Optional[str] = None):
        """
        Initialize the generator.

        Args:
            base_model_name: Base model name from Hugging Face
            model_path: Path to fine-tuned model (LoRA weights)
            quantization_bits: Quantization bits (4 or 8)
            device: Device to run on ('cuda', 'cpu', or auto)
        """
        self.base_model_name = base_model_name
        self.model_path = model_path
        self.quantization_bits = quantization_bits
        self.device = device or ("cuda" if torch.cuda.is_available() else "cpu")

        self.model = None
        self.tokenizer = None

        logger.info(f"Using device: {self.device}")
        if torch.cuda.is_available():
            logger.info(f"GPU: {torch.cuda.get_device_name(0)}")

        self.load_model()

    def load_model(self):
        """Load the model and tokenizer."""
        logger.info(f"Loading base model: {self.base_model_name}")

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
        self.tokenizer = AutoTokenizer.from_pretrained(self.base_model_name)
        self.tokenizer.pad_token = self.tokenizer.eos_token
        self.tokenizer.padding_side = "left"  # Better for generation

        # Load base model
        self.model = AutoModelForCausalLM.from_pretrained(
            self.base_model_name,
            quantization_config=quantization_config,
            device_map="auto" if self.device == "cuda" else None,
            torch_dtype=torch.bfloat16 if quantization_config else torch.float16,
            trust_remote_code=True
        )

        # Load LoRA weights if available
        if self.model_path and Path(self.model_path).exists():
            logger.info(f"Loading fine-tuned weights from: {self.model_path}")
            self.model = PeftModel.from_pretrained(self.model, self.model_path)
        else:
            logger.warning("No fine-tuned model found, using base model only")

        # Move to device if not using device_map
        if self.device != "cuda" or not quantization_config:
            self.model.to(self.device)

        # Set model to evaluation mode
        self.model.eval()

        logger.info("Model loaded successfully")

    def generate_commit_message(self,
                              diff_content: str,
                              max_new_tokens: int = 100,
                              temperature: float = 0.7,
                              top_p: float = 0.9,
                              top_k: int = 50,
                              num_beams: int = 1,
                              do_sample: bool = True,
                              repetition_penalty: float = 1.1,
                              length_penalty: float = 1.0) -> str:
        """
        Generate a commit message from git diff content.

        Args:
            diff_content: Git diff content
            max_new_tokens: Maximum number of new tokens to generate
            temperature: Sampling temperature (higher = more random)
            top_p: Nucleus sampling parameter
            top_k: Top-k sampling parameter
            num_beams: Number of beams for beam search
            do_sample: Whether to use sampling (vs greedy decoding)
            repetition_penalty: Penalty for repetition
            length_penalty: Penalty for length

        Returns:
            Generated commit message
        """
        # Construct prompt
        prompt = f"### Input:\n{diff_content}\n\n### Output:\n"

        # Tokenize input
        inputs = self.tokenizer(
            prompt,
            return_tensors="pt",
            truncation=True,
            max_length=1024  # Leave room for generation
        )

        # Move to device
        inputs = {k: v.to(self.device) for k, v in inputs.items()}

        # Generate
        with torch.no_grad():
            outputs = self.model.generate(
                **inputs,
                max_new_tokens=max_new_tokens,
                temperature=temperature,
                top_p=top_p,
                top_k=top_k,
                num_beams=num_beams,
                do_sample=do_sample,
                repetition_penalty=repetition_penalty,
                length_penalty=length_penalty,
                pad_token_id=self.tokenizer.eos_token_id,
                eos_token_id=self.tokenizer.eos_token_id,
                early_stopping=True
            )

        # Decode output
        generated_text = self.tokenizer.decode(outputs[0], skip_special_tokens=True)

        # Extract commit message (everything after "### Output:\n")
        if "### Output:\n" in generated_text:
            commit_message = generated_text.split("### Output:\n")[-1].strip()
        else:
            # Fallback: take everything after the input
            commit_message = generated_text[len(prompt):].strip()

        # Clean up the commit message
        commit_message = self._clean_commit_message(commit_message)

        return commit_message

    def generate_multiple_candidates(self,
                                  diff_content: str,
                                  num_candidates: int = 3,
                                  **kwargs) -> List[str]:
        """
        Generate multiple commit message candidates.

        Args:
            diff_content: Git diff content
            num_candidates: Number of candidates to generate
            **kwargs: Generation parameters

        Returns:
            List of commit message candidates
        """
        candidates = []

        for i in range(num_candidates):
            # Slightly vary parameters for diversity
            temp = kwargs.get('temperature', 0.7) + (i * 0.1)  # Increase temperature slightly
            candidate = self.generate_commit_message(
                diff_content,
                temperature=temp,
                **kwargs
            )
            candidates.append(candidate)

        return candidates

    def _clean_commit_message(self, message: str) -> str:
        """Clean and format the generated commit message."""
        if not message:
            return "Update code"  # Default fallback

        # Remove extra whitespace
        message = message.strip()

        # Remove common artifacts
        message = message.replace("### Output:", "").strip()
        message = message.replace("### Input:", "").strip()

        # Limit length
        if len(message) > 200:
            # Try to cut at sentence boundary
            sentences = message.split('.')
            message = '.'.join(sentences[:-1]) + '.' if len(sentences) > 1 else message[:200]

        # Ensure it starts with a capital letter
        if message and not message[0].isupper():
            message = message[0].upper() + message[1:]

        # Ensure it ends with proper punctuation
        if message and not message.endswith(('.', '!', '?')):
            message += '.'

        return message


def load_diff_from_file(file_path: str) -> str:
    """Load git diff content from file."""
    try:
        with open(file_path, 'r', encoding='utf-8') as f:
            return f.read().strip()
    except Exception as e:
        logger.error(f"Error reading diff file: {e}")
        return ""


def interactive_mode(generator: CommitMessageGenerator):
    """Run interactive mode for testing."""
    print("=== Git Diff to Commit Message Generator ===")
    print("Enter 'quit' or 'exit' to quit")
    print("Enter 'file:<path>' to load from file")
    print()

    while True:
        print("Paste your git diff (or command):")
        user_input = input().strip()

        if user_input.lower() in ['quit', 'exit', 'q']:
            break

        if user_input.startswith('file:'):
            file_path = user_input[5:].strip()
            diff_content = load_diff_from_file(file_path)
            if not diff_content:
                print("Error: Could not read file or file is empty")
                continue
        else:
            diff_content = user_input

        if not diff_content:
            print("Error: Empty diff content")
            continue

        print("\nGenerating commit message...")
        try:
            commit_message = generator.generate_commit_message(diff_content)
            print(f"\nGenerated commit message:\n{commit_message}")
        except Exception as e:
            print(f"Error generating commit message: {e}")

        print("\n" + "="*50 + "\n")


def main():
    parser = argparse.ArgumentParser(description="Generate commit messages from git diffs")
    parser.add_argument("--model-path", type=str, help="Path to fine-tuned model directory")
    parser.add_argument("--base-model", type=str, default="codellama/CodeLlama-7b-hf", help="Base model name")
    parser.add_argument("--diff-file", type=str, help="Path to git diff file")
    parser.add_argument("--diff-content", type=str, help="Git diff content as string")
    parser.add_argument("--output-file", type=str, help="Output file for commit message")
    parser.add_argument("--quantization-bits", type=int, choices=[4, 8, None], default=4, help="Quantization bits")
    parser.add_argument("--interactive", action="store_true", help="Run in interactive mode")
    parser.add_argument("--num-candidates", type=int, default=1, help="Number of candidates to generate")
    parser.add_argument("--temperature", type=float, default=0.7, help="Generation temperature")
    parser.add_argument("--max-tokens", type=int, default=100, help="Maximum new tokens")
    parser.add_argument("--top-p", type=float, default=0.9, help="Top-p sampling parameter")
    parser.add_argument("--top-k", type=int, default=50, help="Top-k sampling parameter")

    args = parser.parse_args()

    # Set quantization_bits to None if not specified
    if args.quantization_bits is None:
        args.quantization_bits = None

    # Validate arguments
    if not args.interactive and not args.diff_file and not args.diff_content:
        parser.error("Must specify --diff-file, --diff-content, or --interactive")

    try:
        # Initialize generator
        generator = CommitMessageGenerator(
            base_model_name=args.base_model,
            model_path=args.model_path,
            quantization_bits=args.quantization_bits
        )

        if args.interactive:
            # Interactive mode
            interactive_mode(generator)
        else:
            # Single generation mode
            if args.diff_file:
                diff_content = load_diff_from_file(args.diff_file)
            else:
                diff_content = args.diff_content

            if not diff_content:
                logger.error("No diff content provided")
                return

            # Generate commit message(s)
            if args.num_candidates > 1:
                candidates = generator.generate_multiple_candidates(
                    diff_content,
                    num_candidates=args.num_candidates,
                    temperature=args.temperature,
                    max_new_tokens=args.max_tokens,
                    top_p=args.top_p,
                    top_k=args.top_k
                )

                print("Generated commit message candidates:")
                for i, candidate in enumerate(candidates, 1):
                    print(f"{i}. {candidate}")

                # Use first candidate for output file
                commit_message = candidates[0]
            else:
                commit_message = generator.generate_commit_message(
                    diff_content,
                    temperature=args.temperature,
                    max_new_tokens=args.max_tokens,
                    top_p=args.top_p,
                    top_k=args.top_k
                )
                print(f"Generated commit message:\n{commit_message}")

            # Save to file if requested
            if args.output_file:
                with open(args.output_file, 'w', encoding='utf-8') as f:
                    f.write(commit_message)
                logger.info(f"Commit message saved to: {args.output_file}")

    except Exception as e:
        logger.error(f"Inference failed: {e}")
        raise


if __name__ == "__main__":
    main()
