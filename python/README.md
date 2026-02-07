<div align="center">

# Git Diff to Commit Message Generator

基于CodeLlama模型的Git差异到提交信息生成器，专为RTX 4050 Laptop GPU优化。

[中文](README.md) | [English](README_EN.md)

</div>

## 🚀 特性

- **硬件优化**：专为RTX 4050 (6GB VRAM) 等中端GPU优化
- **高效微调**：使用QLoRA技术，大幅降低显存占用
- **多数据源**：支持本地Git仓库和Hugging Face数据集
- **高质量生成**：基于CodeLlama-7B的专业代码理解能力
- **灵活推理**：支持命令行、文件输入和交互式模式

## 📋 系统要求

### 硬件要求
- **GPU**: RTX 4050 Laptop (6GB VRAM) 或同等性能GPU
- **RAM**: 至少16GB系统内存
- **存储**: 至少50GB可用磁盘空间（模型+数据集）

### 软件要求
- **WSL2**: Ubuntu 22.04 (推荐)
- **CUDA**: 12.1+
- **Python**: 3.8+
- **PyTorch**: 2.0+ (CUDA版本)

## 🛠️ 安装和设置

### 1. 克隆项目并进入Python目录

```bash
cd /mnt/f/My__StudyStack/My_Project/pnana/python
```

### 2. 安装依赖

```bash
pip install -r requirements.txt
```

### 3. 验证CUDA环境

```bash
python -c "import torch; print(f'CUDA available: {torch.cuda.is_available()}'); print(f'GPU: {torch.cuda.get_device_name(0)}')"
```

## 📊 数据准备

### 使用现有数据集（推荐新手）

从Hugging Face下载预处理的数据集：

```bash
python scripts/data_preparation.py --use-huggingface-dataset --output data/train_data.jsonl
```

### 从本地Git仓库提取数据

```bash
# 从当前项目的Git仓库提取
python scripts/data_preparation.py --repo-path /mnt/f/My__StudyStack/My_Project/pnana --output data/train_data.jsonl

# 从其他仓库提取
python scripts/data_preparation.py --repo-path /path/to/your/git/repo --output data/train_data.jsonl --max-commits 2000
```

### 数据验证

```python
from utils import DataValidator

# 加载并验证数据
import json
with open('data/train_data.jsonl', 'r') as f:
    data = [json.loads(line) for line in f]

validator = DataValidator()
stats = validator.validate_dataset(data)
print(f"数据质量统计: {stats}")
```

## 🏋️ 模型训练

### 基础训练（推荐）

```bash
python scripts/train_model.py \
    --data-path data/train_data.jsonl \
    --output-dir models/git-commit-model \
    --batch-size 1 \
    --gradient-accumulation 4 \
    --learning-rate 2e-4 \
    --num-epochs 3
```

### 高级训练配置

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

### 训练参数说明

| 参数 | 默认值 | 说明 |
|------|--------|------|
| `--batch-size` | 1 | RTX 4050显存限制，建议保持1 |
| `--gradient-accumulation` | 4 | 梯度累积，模拟更大的batch size |
| `--learning-rate` | 2e-4 | 学习率，建议1e-4到3e-4 |
| `--lora-r` | 8 | LoRA秩，越小显存占用越少 |
| `--quantization-bits` | 4 | 量化位数，4位最佳平衡 |

## 🎯 推理使用

### 从文件生成提交信息

```bash
# 创建测试diff文件
git diff HEAD~1 > test_diff.diff

# 生成提交信息
python scripts/inference.py \
    --model-path models/git-commit-model \
    --diff-file test_diff.diff
```

### 直接输入diff内容

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

### 交互式模式

```bash
python scripts/inference.py --model-path models/git-commit-model --interactive
```

在交互模式中，你可以：
- 直接粘贴diff内容
- 使用 `file:/path/to/diff` 加载文件
- 输入 `quit` 退出

### 生成多个候选

```bash
python scripts/inference.py \
    --model-path models/git-commit-model \
    --diff-file changes.diff \
    --num-candidates 3 \
    --temperature 0.8
```

### 高级推理参数

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

## ⚙️ 配置和优化

### 显存优化策略

1. **使用4位量化**（默认）：最大限度减少显存占用
2. **减小批次大小**：保持batch_size=1
3. **梯度累积**：使用gradient_accumulation_steps=4
4. **降低序列长度**：max_length=512（如果训练OOM）

### 训练时间优化

- **数据集大小**：从1000-2000个样本开始
- **训练轮数**：3-5轮通常足够
- **评估频率**：每500步评估一次

### 质量提升技巧

1. **数据质量**：使用高质量的diff-commit对
2. **数据清洗**：移除无意义的提交信息
3. **多样化数据**：包含不同类型的代码变更
4. **后处理**：对生成结果进行简单后处理

## 📈 模型评估

```python
from utils import ModelEvaluator

# 准备测试数据
test_predictions = ["Add user authentication", "Fix login bug"]
test_references = ["Implement user login system", "Fix authentication issue"]

evaluator = ModelEvaluator()

# BLEU分数
bleu_score = evaluator.calculate_bleu_score(test_predictions, test_references)
print(f"BLEU Score: {bleu_score}")

# ROUGE分数
rouge_scores = evaluator.calculate_rouge_score(test_predictions, test_references)
print(f"ROUGE Scores: {rouge_scores}")

# 多样性分析
diversity = evaluator.analyze_generation_diversity(test_predictions)
print(f"Diversity Analysis: {diversity}")
```

## 🔧 故障排除

### CUDA相关问题

**问题**: `CUDA out of memory`
```bash
# 解决方案：减少批次大小和序列长度
python scripts/train_model.py --batch-size 1 --max-length 512 --gradient-accumulation 8
```

**问题**: `CUDA not available`
```bash
# 检查CUDA安装
nvidia-smi
python -c "import torch; print(torch.cuda.is_available())"
```

### 数据相关问题

**问题**: 数据集为空或质量差
```bash
# 检查数据提取
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

### 模型相关问题

**问题**: 模型加载失败
```bash
# 检查模型路径和文件
ls -la models/git-commit-model/
# 重新下载基础模型
python -c "from transformers import AutoModelForCausalLM; AutoModelForCausalLM.from_pretrained('codellama/CodeLlama-7b-hf')"
```

## 📝 使用示例

### 完整工作流

```bash
# 1. 准备数据
python scripts/data_preparation.py --repo-path /your/project --output data.jsonl

# 2. 训练模型
python scripts/train_model.py --data-path data.jsonl --output-dir my-model

# 3. 生成提交信息
echo "你的diff内容" > changes.diff
python scripts/inference.py --model-path my-model --diff-file changes.diff

# 4. 评估模型
python -c "
from utils import ModelEvaluator
# ... 评估代码 ...
"
```

### 集成到Git工作流

创建 `.git/hooks/prepare-commit-msg` 钩子：

```bash
#!/bin/bash
# 获取暂存区的diff
git diff --cached > /tmp/current_diff.diff

# 生成提交信息建议
python /path/to/python/scripts/inference.py \
    --model-path /path/to/trained-model \
    --diff-file /tmp/current_diff.diff \
    --output-file /tmp/suggested_commit.txt

# 显示建议
if [ -f /tmp/suggested_commit.txt ]; then
    echo "Suggested commit message:"
    cat /tmp/suggested_commit.txt
fi
```

## 🤝 贡献

欢迎提交Issue和Pull Request！

### 开发环境设置

```bash
# 安装开发依赖
pip install pytest black flake8 pre-commit

# 运行测试
pytest

# 代码格式化
black .
flake8 .
```

## 📄 许可证

本项目采用MIT许可证。

## 🙏 致谢

- [CodeLlama](https://github.com/facebookresearch/codellama) - 基础语言模型
- [PEFT](https://github.com/huggingface/peft) - 参数高效微调库
- [Transformers](https://github.com/huggingface/transformers) - 模型加载和训练框架
- [Hugging Face Datasets](https://github.com/huggingface/datasets) - 数据集处理

---

**注意**: 本项目专为学习和研究目的设计。生成的内容仅供参考，请根据实际代码变更情况调整提交信息。
