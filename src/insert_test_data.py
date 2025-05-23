import sqlite3
import pandas as pd
import random
from datetime import datetime, timedelta

# Configuration
NUM_ENTRIES = 100
START_TIME = datetime.now() - timedelta(minutes=NUM_ENTRIES * 1)  # 1-minute intervals
DB_PATH = 'build/optimizer.db'

def create_test_data():
    timestamps = [START_TIME + timedelta(minutes=i) for i in range(NUM_ENTRIES)]

    # Simulate system CPU usage with some pattern
    system_data = pd.DataFrame({
        'timestamp': timestamps,
        'cpu_usage': [random.uniform(5, 25) for _ in range(NUM_ENTRIES)],
        'memory_usage': [random.uniform(30, 80) for _ in range(NUM_ENTRIES)],
        'disk_usage': [random.uniform(20, 70) for _ in range(NUM_ENTRIES)]
    })

    # Simulate process data with multiple PIDs contributing to CPU
    process_data = []
    for timestamp in timestamps:
        for pid in range(1, 6):  # Simulate 5 processes
            process_data.append({
                'timestamp': timestamp,
                'pid': pid,
                'process_name': f'proc_{pid}',
                'cpu_usage': random.uniform(0.5, 5)
            })
    process_data = pd.DataFrame(process_data)

    return system_data, process_data

def populate_database(system_data, process_data):
    conn = sqlite3.connect(DB_PATH)

    # Drop existing tables to reset schema
    conn.execute("DROP TABLE IF EXISTS system_stats")
    conn.execute("DROP TABLE IF EXISTS process_stats")

    # Recreate tables with expected schema
    conn.execute('''
        CREATE TABLE system_stats (
            timestamp TEXT PRIMARY KEY,
            cpu_usage REAL,
            memory_usage REAL,
            disk_usage REAL
        )
    ''')
    conn.execute('''
        CREATE TABLE process_stats (
            timestamp TEXT,
            pid INTEGER,
            process_name TEXT,
            cpu_usage REAL
        )
    ''')

    # Insert test data
    system_data.to_sql('system_stats', conn, if_exists='append', index=False)
    process_data.to_sql('process_stats', conn, if_exists='append', index=False)

    conn.commit()
    conn.close()


if __name__ == "__main__":
    system_data, process_data = create_test_data()
    populate_database(system_data, process_data)
    print("Test data populated.")
