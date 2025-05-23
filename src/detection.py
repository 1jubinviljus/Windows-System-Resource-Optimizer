import pandas as pd
import sqlite3
from datetime import datetime, timedelta

# Parameters
ROLLING_WINDOW = 10 # Numbers of entries to average
THRESHOLD = 10 # % above rolling window to be considered a spike in the data

def load_data():
    conn = sqlite3.connect('build/optimizer.db')
    df = pd.read_sql_query("SELECT * FROM system_stats", conn, parse_dates=["timestamp"])
    conn.close
    return df

def detect_spike(df, column, rolling_window, threshold):
    df = df.copy()
    df['rolling_avg'] = df[column].rolling(window=window).mean()
    df['spike'] = df[column] > (df['rolling_avg'] + threshold) # Creates a boolean to represent if a spike has occured
    spikes = df[df['spike']]
    return spikes[['timestamp', column]]


def main():
    df = load_data
    
    cpu_spikes = detect_spike(df, 'cpu_usage', ROLLING_WINDOW, THRESHOLD)
    mem_spikes = detect_spike(df, 'memory_usage', ROLLING_WINDOW, THRESHOLD)
    disk_spikes = detect_spike(df, 'disk_usage', ROLLING_WINDOW, THRESHOLD)