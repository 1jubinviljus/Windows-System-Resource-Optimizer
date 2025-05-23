import pandas as pd
import sqlite3
from detection import detect_long_spikes, ROLLING_WINDOW, THRESHOLD

CPU_THRESHOLD = 12 # Idle CPU usage %
MEM_THRESHOLD = 10 # Idle memory usage %
DISK_THRESHOLD = 10 # Idle disk usage %

def classify_idle_active_cpu(df, cpu_threshold):
    df = df.copy()
    df['state'] = df['cpu_usage'].apply(lambda x: 'idle' if x < cpu_threshold else 'active')
    return df[['timestamp', 'cpu_usage', 'state']]

def classify_idle_active_mem(df, mem_threshold):
    df = df.copy()
    df['state'] = df['mem_usage'].apply(lambda x: 'idle' if x < mem_threshold else 'active')
    return df[['timestamp', 'mem_usage', 'state']]

def classify_idle_active_disk(df, disk_threshold):
    df = df.copy()
    df['state'] = df['disk_usage'].apply(lambda x: 'idle' if x < disk_threshold else 'active')
    return df[['timestamp', 'disk_usage', 'state']]


def load_process_data():
    conn = sqlite3.connect('build/optimizer.db')
    df = pd.read_sql_query("SELECT * FROM process_stats", conn, parse_dates=["timestamp"])
    conn.close()
    return df

def aggregate_process_cpu(df_process):
    df_agg = df_process.groupby('timestamp')['cpu_usage'].sum().reset_index()
    df_agg.rename(columns={'cpu_usage': 'total_process_cpu'}, inplace=True)
    return df_agg

def correlate_system_and_process_cpu(df_system, df_process_agg):
    df_merged = pd.merge(df_system, df_process_agg, on='timestamp', how='inner')
    correlation = df_merged['cpu_usage'].corr(df_merged['total_process_cpu'])
    return correlation

def load_system_data():
    conn = sqlite3.connect('build/optimizer.db')
    df = pd.read_sql_query("SELECT * FROM system_stats", conn, parse_dates=["timestamp"])
    conn.close()
    return df

def merge_spikes_and_states(df_classified, df_spikes):
    df_classified = df_classified.copy()
    df_classified['spike'] = df_classified['timestamp'].isin(df_spikes['timestamp'])
    spikes_with_state = df_classified[df_classified['spike']]
    counts = spikes_with_state['state'].value_counts()
    print("\nSpike counts by state:")
    print(counts)
    return df_classified

def main():
    df_system = load_system_data()
    df_system_classified_cpu = classify_idle_active_cpu(df_system, CPU_THRESHOLD)
    df_system_classified_mem = classify_idle_active_mem(df_system, MEM_THRESHOLD)
    df_system_classified_disk = classify_idle_active_disk(df_system, DISK_THRESHOLD)

    print("Idle/Active Classification:")
    print(df_system_classified_cpu.tail())
    print(df_system_classified_mem.tail())
    print(df_system_classified_disk.tail())

    # Load spikes (you can save them from detection.py or detect here)
    from detection import detect_long_spikes, ROLLING_WINDOW, THRESHOLD  # Reuse constants
    cpu_spikes = detect_long_spikes(df_system, 'cpu_usage', ROLLING_WINDOW, THRESHOLD,
                                    min_duration_minutes=0.5, sample_interval_seconds=10)

    # Merge classification and spike info
    df_with_spike_flags = merge_spikes_and_states(df_system_classified, cpu_spikes)

    df_process = load_process_data()
    df_process_agg = aggregate_process_cpu(df_process)

    corr = correlate_system_and_process_cpu(df_system, df_process_agg)
    print(f"\nCorrelation between system and total process CPU usage: {corr:.2f}")


if __name__ == "__main__":
    main()
