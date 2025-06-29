# Windows System Resource Optimizer

This tool helps you monitor and optimize your Windows PC. It tracks system and process resource usage (like CPU and memory), and tries to boost performance for engineering software.

## How it works

- **C programs** collect system and process data, and try to optimize important apps.
- **Python scripts** analyze the data and help you understand what's slowing things down.

## How to use

1. **Build the C code** (if not already built):
   ```
   make all
   ```
2. **Run the main controller:**
   ```
   python src/main_controller.py
   ```
   This will start collecting data, optimizing, and running analysis.

3. **(Optional) Run tests:**
   ```
   python test_system.py
   ```

## Requirements

- Windows 10 or later
- Python 3.7+
- A C compiler (like gcc)
- Python packages: see `requirements.txt`

## Project structure

- `src/data_collection/` – C code for collecting system/process data
- `src/resource_management/` – C code for process optimization
- `src/data_analysis/` – Python scripts for analysis and metrics
- `src/main_controller.py` – Orchestrates everything
- `test_system.py` – Tests the system
