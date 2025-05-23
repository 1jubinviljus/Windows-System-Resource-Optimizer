# Windows System Resource Optimizer

A powerful system monitoring and optimization tool that tracks CPU, memory, and disk usage patterns in Windows systems. This tool helps identify resource-intensive processes and system performance bottlenecks through data collection and analysis.

## Features

- **Real-time System Monitoring**
  - CPU usage tracking
  - Memory usage monitoring
  - Disk usage analysis
  - Process-level resource consumption

- **Advanced Analytics**
  - Resource spike detection
  - Idle/Active state classification
  - Process-system correlation analysis
  - Long-duration spike identification

- **Data Visualization**
  - System resource usage trends
  - Top CPU-consuming processes
  - Resource spike highlighting
  - Smoothed usage graphs

## Prerequisites

- Windows 10 or later
- Python 3.7+
- C compiler (for the data collection component)
- SQLite3

## Installation

1. Clone the repository:
   ```bash
   git clone https://github.com/yourusername/Windows-System-Resource-Optimizer.git
   cd Windows-System-Resource-Optimizer
   ```

2. Install Python dependencies:
   ```bash
   pip install pandas matplotlib sqlite3
   ```

3. Compile the C data collector:
   ```bash
   mkdir build
   gcc src/main.c -o build/collector.exe -lpsapi
   ```

## Usage

1. Start the data collector:
   ```bash
   ./build/collector.exe
   ```
   This will begin collecting system and process statistics in the SQLite database.

2. Run analysis tools:
   ```bash
   python src/analytics.py    # Basic system state analysis
   python src/visualize.py    # Generate usage graphs
   python src/process_spike_correlation.py  # Analyze process correlations
   ```

## Configuration

Key parameters can be adjusted in the source files:

- `src/analytics.py`:
  - `CPU_THRESHOLD`: Idle CPU usage threshold (default: 12%)
  - `MEM_THRESHOLD`: Idle memory usage threshold (default: 10%)
  - `DISK_THRESHOLD`: Idle disk usage threshold (default: 10%)

- `src/detection.py`:
  - `ROLLING_WINDOW`: Data smoothing window size
  - `THRESHOLD`: Spike detection sensitivity

## Project Structure

- `src/`
  - `main.c`: Core data collection system (C)
  - `analytics.py`: System state analysis
  - `detection.py`: Resource spike detection
  - `visualize.py`: Data visualization
  - `process_spike_correlation.py`: Process analysis
  - `insert_test_data.py`: Test data generation

- `build/`: Compiled binaries and database
- `logs/`: Application logs
- `include/`: C header files
