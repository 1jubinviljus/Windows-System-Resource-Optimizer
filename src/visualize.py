import sqlite3
import pandas as pd
import matplotlib.pyplot as plt
from datetime import datetime, timedelta
from detection import load_data, detect_spike, ROLLING_WINDOW, THRESHOLD

# --- SETTINGS ---
FOCUS_LAST_HOUR = False    # Set True to only view data from the last hour
SMOOTH_WINDOW = ROLLING_WINDOW
SAVE_PLOTS = False         # Set True to save plots as PNG files

# --- Load data ---
conn = sqlite3.connect("build/optimizer.db")
system_df = pd.read_sql_query("SELECT * FROM system_stats", conn, parse_dates=["timestamp"])
process_df = pd.read_sql_query("SELECT * FROM process_stats", conn, parse_dates=["timestamp"])
conn.close()

# --- Time filtering ---
if FOCUS_LAST_HOUR:
    cutoff = datetime.now() - timedelta(hours=1)
    system_df = system_df[system_df['timestamp'] > cutoff]
    process_df = process_df[process_df['timestamp'] > cutoff]

# --- Convert timestamps ---
system_df['timestamp'] = pd.to_datetime(system_df['timestamp'])
process_df['timestamp'] = pd.to_datetime(process_df['timestamp'])

# --- Check required columns ---
for col in ['cpu_usage', 'memory_usage', 'disk_usage']:
    if col not in system_df.columns:
        raise KeyError(f"Column '{col}' missing from system_stats table.")

for col in ['cpu_usage', 'process_name']:
    if col not in process_df.columns:
        raise KeyError(f"Column '{col}' missing from process_stats table.")

# --- Apply smoothing ---
system_df['cpu_smooth'] = system_df['cpu_usage'].rolling(window=SMOOTH_WINDOW, min_periods=1).mean()
system_df['memory_smooth'] = system_df['memory_usage'].rolling(window=SMOOTH_WINDOW, min_periods=1).mean()
system_df['disk_smooth'] = system_df['disk_usage'].rolling(window=SMOOTH_WINDOW, min_periods=1).mean()

# --- Plot system usage ---
plt.figure(figsize=(11, 5))
plt.plot(system_df['timestamp'], system_df['cpu_smooth'], label='CPU Usage (Smoothed)')
plt.plot(system_df['timestamp'], system_df['memory_smooth'], label='Memory Usage (Smoothed)')
plt.plot(system_df['timestamp'], system_df['disk_smooth'], label='Disk Usage (Smoothed)')
plt.xlabel("Time")
plt.ylabel("Usage (%)")
plt.title("System Resource Usage Over Time")
plt.legend()
plt.grid(True)
plt.xticks(rotation=30)
plt.tight_layout()
if SAVE_PLOTS:
    plt.savefig("system_usage_over_time.png")
plt.show()

# --- Top CPU-consuming processes ---
top_cpu = process_df.groupby('process_name')['cpu_usage'].mean().sort_values(ascending=False).head(5)
top_cpu.plot(kind='barh', figsize=(8, 4), title='Top 5 CPU-Hungry Processes', color='tomato')
plt.xlabel("Avg CPU Usage (%)")
plt.gca().invert_yaxis()
plt.tight_layout()
if SAVE_PLOTS:
    plt.savefig("top_cpu_processes.png")
plt.show()

# --- Plot Resource Spikes ---
def plot_spikes(system_df, cpu_spikes, mem_spikes, disk_spikes):
    plt.figure(figsize=(11, 5))
    plt.plot(system_df['timestamp'], system_df['cpu_smooth'], label='CPU Usage (Smoothed)')
    plt.plot(system_df['timestamp'], system_df['memory_smooth'], label='Memory Usage (Smoothed)')
    plt.plot(system_df['timestamp'], system_df['disk_smooth'], label='Disk Usage (Smoothed)')

    plt.scatter(cpu_spikes['timestamp'], cpu_spikes['cpu_usage'], color='red')
    plt.scatter(mem_spikes['timestamp'], mem_spikes['memory_usage'], color='red')
    plt.scatter(disk_spikes['timestamp'], disk_spikes['disk_usage'], color='red')
    plt.scatter([], [], color='red', label='Spikes')  # dummy for legend

    plt.xlabel("Time")
    plt.ylabel("Usage (%)")
    plt.title("System Resource Usage with Spikes Highlighted")
    plt.legend()
    plt.grid(True)
    plt.xticks(rotation=30)
    plt.tight_layout()
    plt.show()

# --- Detect Resource Spikes ---
spikes_df = load_data()
cpu_spikes = detect_spike(spikes_df, 'cpu_usage', SMOOTH_WINDOW, THRESHOLD)
mem_spikes = detect_spike(spikes_df, 'memory_usage', SMOOTH_WINDOW, THRESHOLD)
disk_spikes = detect_spike(spikes_df, 'disk_usage', SMOOTH_WINDOW, THRESHOLD)

plot_spikes(system_df, cpu_spikes, mem_spikes, disk_spikes)
