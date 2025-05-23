import sqlite3
import pandas as pd
from detection import detect_spike, load_data, ROLLING_WINDOW, THRESHOLD

def main():
    # --- Load data ---
    conn = sqlite3.connect("build/optimizer.db")
    system_df = pd.read_sql_query("SELECT * FROM system_stats", conn, parse_dates=["timestamp"])
    process_df = pd.read_sql_query("SELECT * FROM process_stats", conn, parse_dates=["timestamp"])
    conn.close()

    # --- Detect CPU spikes in system stats ---
    spikes_df = load_data()
    cpu_spikes = detect_spike(spikes_df, 'cpu_usage', ROLLING_WINDOW, THRESHOLD)

    if cpu_spikes.empty:
        print("No CPU spikes detected.")
        return

    # --- For each spike, find top process(es) at that time ---
    spike_processes = []
    for _, spike in cpu_spikes.iterrows():
        spike_time = spike['timestamp']
        # Find processes running at the spike time (allowing for small time difference)
        time_window = pd.Timedelta(seconds=2)
        mask = (process_df['timestamp'] >= spike_time - time_window) & (process_df['timestamp'] <= spike_time + time_window)
        processes_at_spike = process_df[mask]
        if not processes_at_spike.empty:
            top_proc = processes_at_spike.loc[processes_at_spike['cpu_usage'].idxmax()]
            spike_processes.append(top_proc['process_name'])

    # --- Aggregate spike counts per process ---
    spike_counts = pd.Series(spike_processes).value_counts()
    print("Processes most frequently associated with CPU spikes:")
    print(spike_counts.head(10))

if __name__ == "__main__":
    main()