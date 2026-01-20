<div align="center">

# Git Diff to Commit Message Generator

A CodeLlama-based Git diff to commit message generator, optimized for RTX 4050 Laptop GPU.

[中文](README.md) | [English](README_EN.md)

</div>

## 🚀 Features

- **Hardware Optimized**: Specifically optimized for RTX 4050 (6GB VRAM) and similar mid-range GPUs
- **Efficient Fine-tuning**: Uses QLoRA technology to significantly reduce VRAM usage
- **Multiple Data Sources**: Supports local Git repositories and Hugging Face datasets
- **High-Quality Generation**: Based on CodeLlama-7B's professional code understanding capabilities
- **Flexible Inference**: Supports command-line, file input, and interactive modes

## 📋 System Requirements

### Hardware Requirements
- **GPU**: RTX 4050 Laptop (6GB VRAM) or equivalent performance GPU
- **RAM**: At least 16GB system memory
- **Storage**: At least 50GB available disk space (models + datasets)

### Software Requirements
- **WSL2**: Ubuntu 22.04 (recommended)
- **CUDA**: 12.1+
- **Python**: 3.8+
- **PyTorch**: 2.0+ (CUDA version)

## 🛠️ Installation and Setup

### 1. Navigate to Python Directory

```bash
cd /mnt/f/My__StudyStack/My_Project/pnana/python
```

### 2. Install Dependencies

```bash
pip install -r requirements.txt
```

### 3. Verify CUDA Environment

```bash
python -c "import torch; print(f'CUDA available: {torch.cuda.is_available()}'); print(f'GPU: {torch.cuda.get_device_name(0)}')"
```

## 📊 Data Preparation

### Using Existing Dataset (Recommended for Beginners)

Download pre-processed datasets from Hugging Face:

```bash
python scripts/data_preparation.py --use-huggingface-dataset --output data/train_data.jsonl
```

### Extract Data from Local Git Repository

```bash
# Extract from current project's Git repository
python scripts/data_preparation.py --repo-path /mnt/f/My__StudyStack/My_Project/pnana --output data/train_data.jsonl

# Extract from other repositories
python scripts/data_preparation.py --repo-path /path/to/your/git/repo --output data/train_data.jsonl --max-commits 2000
```

### Data Validation

```python
from utils import DataValidator

# Load and validate data
import json
with open('data/train_data.jsonl', 'r') as f:
    data = [json.loads(line) for line in f]

validator = DataValidator()
stats = validator.validate_dataset(data)
print(f"Data quality statistics: {stats}")
```

## 🏋️ Model Training

### Basic Training (Recommended)

```bash
python scripts/train_model.py \
    --data-path data/train_data.jsonl \
    --output-dir models/git-commit-model \
    --batch-size 1 \
    --gradient-accumulation 4 \
    --learning-rate 2e-4 \
    --num-epochs 3
```

### Advanced Training Configuration

```bash
python scripts/train_model.py \
    --data-path data/train_data.jsonl \
    --output-dir models/git-commit-model \
    --model-name codellama/CodeLlama-7b-hf \
    --quantization-bits 4 \
    --lora-r 8 \
    --lora-alpha 32 \
    --batch-size 1 \
    --gradient-accumulation 4 \
    --learning-rate 2e-4 \
    --num-epochs 3 \
    --max-length 1024 \
    --use-wandb \
    --wandb-project git-commit-training
```

### Training Parameters Description

| Parameter | Default | Description |
|-----------|---------|-------------|
| `--batch-size` | 1 | RTX 4050 VRAM limitation, recommended to keep at 1 |
| `--gradient-accumulation` | 4 | Gradient accumulation to simulate larger batch sizes |
| `--learning-rate` | 2e-4 | Learning rate, recommended 1e-4 to 3e-4 |
| `--lora-r` | 8 | LoRA rank, smaller values use less VRAM |
| `--quantization-bits` | 4 | Quantization bits, 4-bit provides best balance |

## 🎯 Inference Usage

### Generate Commit Message from File

```bash
# Create test diff file
git diff HEAD~1 > test_diff.diff

# Generate commit message
python scripts/inference.py \
    --model-path models/git-commit-model \
    --diff-file test_diff.diff
```

### Direct Diff Content Input

```bash
python scripts/inference.py \
    --model-path models/git-commit-model \
    --diff-content "diff --git a/file.txt b/file.txt
index 1234567..abcdefg 100644
--- a/file.txt
+++ b/file.txt
@@ -1 +1 @@
-old content
+new content"
```

### Interactive Mode

```bash
python scripts/inference.py --model-path models/git-commit-model --interactive
```

In interactive mode, you can:
- Directly paste diff content
- Use `file:/path/to/diff` to load files
- Type `quit` to exit

### Generate Multiple Candidates

```bash
python scripts/inference.py \
    --model-path models/git-commit-model \
    --diff-file changes.diff \
    --num-candidates 3 \
    --temperature 0.8
```

### Advanced Inference Parameters

```bash
python scripts/inference.py \
    --model-path models/git-commit-model \
    --diff-file changes.diff \
    --temperature 0.7 \
    --top-p 0.9 \
    --top-k 50 \
    --max-tokens 100 \
    --output-file commit_message.txt
```

## ⚙️ Configuration and Optimization

### VRAM Optimization Strategies

1. **Use 4-bit quantization** (default): Maximizes VRAM efficiency
2. **Reduce batch size**: Keep batch_size=1
3. **Gradient accumulation**: Use gradient_accumulation_steps=4
4. **Reduce sequence length**: max_length=512 (if training OOM)

### Training Time Optimization

- **Dataset size**: Start with 1000-2000 samples
- **Training epochs**: 3-5 epochs are usually sufficient
- **Evaluation frequency**: Evaluate every 500 steps

### Quality Improvement Tips

1. **Data quality**: Use high-quality diff-commit pairs
2. **Data cleaning**: Remove meaningless commit messages
3. **Data diversity**: Include various types of code changes
4. **Post-processing**: Apply simple post-processing to generated results

## 📈 Model Evaluation

```python
from utils import ModelEvaluator

# Prepare test data
test_predictions = ["Add user authentication", "Fix login bug"]
test_references = ["Implement user login system", "Fix authentication issue"]

evaluator = ModelEvaluator()

# BLEU score
bleu_score = evaluator.calculate_bleu_score(test_predictions, test_references)
print(f"BLEU Score: {bleu_score}")

# ROUGE scores
rouge_scores = evaluator.calculate_rouge_score(test_predictions, test_references)
print(f"ROUGE Scores: {rouge_scores}")

# Diversity analysis
diversity = evaluator.analyze_generation_diversity(test_predictions)
print(f"Diversity Analysis: {diversity}")
```

## 🔧 Troubleshooting

### CUDA Related Issues

**Problem**: `CUDA out of memory`
```bash
# Solution: Reduce batch size and sequence length
python scripts/train_model.py --batch-size 1 --max-length 512 --gradient-accumulation 8
```

**Problem**: `CUDA not available`
```bash
# Check CUDA installation
nvidia-smi
python -c "import torch; print(torch.cuda.is_available())"
```

### Data Related Issues

**Problem**: Empty or poor quality dataset
```bash
# Check data extraction
python -c "
from utils import DataValidator
import json
with open('data/train_data.jsonl') as f:
    data = [json.loads(line) for line in f]
print(f'Loaded {len(data)} samples')
stats = DataValidator().validate_dataset(data)
print(f'Validation: {stats}')
"
```

### Model Related Issues

**Problem**: Model loading failed
```bash
# Check model path and files
ls -la models/git-commit-model/
# Re-download base model
python -c "from transformers import AutoModelForCausalLM; AutoModelForCausalLM.from_pretrained('codellama/CodeLlama-7b-hf')"
```

## 📝 Usage Examples

### Complete Workflow

```bash
# 1. Prepare data
python scripts/data_preparation.py --repo-path /your/project --output data.jsonl

# 2. Train model
python scripts/train_model.py --data-path data.jsonl --output-dir my-model

# 3. Generate commit messages
echo "your diff content" > changes.diff
python scripts/inference.py --model-path my-model --diff-file changes.diff

# 4. Evaluate model
python -c "
from utils import ModelEvaluator
# ... evaluation code ...
"
```

### Integration with Git Workflow

Create a `.git/hooks/prepare-commit-msg` hook:

```bash
#!/bin/bash
# Get staged diff
git diff --cached > /tmp/current_diff.diff

# Generate commit message suggestion
python /path/to/python/scripts/inference.py \
    --model-path /path/to/trained-model \
    --diff-file /tmp/current_diff.diff \
    --output-file /tmp/suggested_commit.txt

# Display suggestion
if [ -f /tmp/suggested_commit.txt ]; then
    echo "Suggested commit message:"
    cat /tmp/suggested_commit.txt
fi
```

## 🤝 Contributing

Welcome to submit Issues and Pull Requests!

### Development Environment Setup

```bash
# Install development dependencies
pip install pytest black flake8 pre-commit

# Run tests
pytest

# Code formatting
black .
flake8 .
```

## 📄 License

This project is licensed under the MIT License.

## 🙏 Acknowledgments

- [CodeLlama](https://github.com/facebookresearch/codellama) - Base language model
- [PEFT](https://github.com/huggingface/peft) - Parameter-efficient fine-tuning library
- [Transformers](https://github.com/huggingface/transformers) - Model loading and training framework
- [Hugging Face Datasets](https://github.com/huggingface/datasets) - Dataset processing

---

**Note**: This project is designed for learning and research purposes. Generated content is for reference only. Please adjust commit messages based on actual code changes.
