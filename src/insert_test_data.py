import sqlite3
from datetime import datetime, timedelta

def insert_test_data():
    conn = sqlite3.connect('build/optimizer.db')
    cursor = conn.cursor()

    # Clear existing data (optional, be careful!)
    # cursor.execute("DELETE FROM system_stats")

    # Base timestamp (now)
    base_time = datetime.now()

    # Create test data with normal values then spikes
    data = []
    # 10 normal points with cpu_usage = 10
    for i in range(10):
        timestamp = base_time + timedelta(seconds=i * 10)
        data.append((timestamp, 10, 10, 10))  # cpu, memory, disk all 10%

    # 5 spike points with cpu_usage = 20 (above threshold)
    for i in range(10, 15):
        timestamp = base_time + timedelta(seconds=i * 10)
        data.append((timestamp, 20, 10, 10))

    # Insert rows into system_stats (make sure your columns match!)
    cursor.executemany(
        "INSERT INTO system_stats (timestamp, cpu_usage, memory_usage, disk_usage) VALUES (?, ?, ?, ?)",
        data
    )

    conn.commit()
    conn.close()
    print("Test data inserted.")

if __name__ == "__main__":
    insert_test_data()
