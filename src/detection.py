import pandas as pd
import sqlite3
from datetime import datetime, timedelta

# Parameters
ROLLING_WINDOW = 3 # Numbers of entries to average (1 to ignore the window)
THRESHOLD = 5 # % above rolling window to be considered a spike in the data

def load_data():
    conn = sqlite3.connect('build/optimizer.db')
    df = pd.read_sql_query("SELECT * FROM system_stats", conn, parse_dates=["timestamp"])
    conn.close()
    return df

def detect_spike(df, column, rolling_window, threshold):
    df = df.copy()
    df['rolling_avg'] = df[column].rolling(window=rolling_window).mean()
    df['spike'] = df[column] > (df['rolling_avg'] + threshold) # Creates a boolean to represent if a spike has occured
    spikes = df[df['spike']]
    return spikes[['timestamp', column]]

def detect_long_spikes(df, column, rolling_window, threshold, min_duration_minutes, sample_interval_seconds):
    df = df.copy()
    df['rolling_avg'] = df[column].rolling(window=rolling_window, min_periods=1).mean().shift(1)
    df['spike'] = df[column] > (df['rolling_avg'] + threshold)

    # Calculate how many consecutive samples correspond to min_duration_minutes
    min_points = int((min_duration_minutes * 60) / sample_interval_seconds)

    # Identify consecutive spike groups
    df['spike_group'] = (df['spike'] != df['spike'].shift()).cumsum()
    spike_groups = df[df['spike']].groupby('spike_group')

    # Filter spike groups by duration
    long_spikes = spike_groups.filter(lambda g: len(g) >= min_points)

    return long_spikes[['timestamp', column]]

def main():
    df = load_data()

    cpu_spikes = detect_long_spikes(df, 'cpu_usage', ROLLING_WINDOW, THRESHOLD,
                                   min_duration_minutes=0.5, sample_interval_seconds=10)
    mem_spikes = detect_long_spikes(df, 'memory_usage', ROLLING_WINDOW, THRESHOLD,
                                   min_duration_minutes=0.5, sample_interval_seconds=10)
    disk_spikes = detect_long_spikes(df, 'disk_usage', ROLLING_WINDOW, THRESHOLD,
                                    min_duration_minutes=0.5, sample_interval_seconds=10)

    print("CPU spikes:")
    print(cpu_spikes)
    print("\nMemory spikes:")
    print(mem_spikes)
    print("\nDisk spikes:")
    print(disk_spikes)

if __name__ == "__main__":
    main()