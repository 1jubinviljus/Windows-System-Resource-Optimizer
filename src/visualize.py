import sqlite3
import pandas as pd
import matplotlib.pyplot as plt

# Connect to your local database file
conn = sqlite3.connect("build/optimizer.db")

# Read tables
system_df = pd.read_sql_query("SELECT * FROM system_stats", conn, parse_dates=["timestamp"])
process_df = pd.read_sql_query("SELECT * FROM process_stats", conn, parse_dates=["timestamp"])

# Close the databse connection
conn.close()

# Convert timestamps to datetime
system_df['timestamp'] = pd.to_datetime(system_df['timestamp'])
process_df['timestamp'] = pd.to_datetime(process_df['timestamp'])

# Plot system resource usage
plt.figure(figsize=(11, 5))
plt.plot(system_df['timestamp'], system_df['cpu_usage'], label='CPU Usage (%)')
plt.plot(system_df['timestamp'], system_df['memory_usage'], label='Memory Usage (%)')
plt.plot(system_df['timestamp'], system_df['disk_usage'], label='Disk Usage (%)')
plt.xlabel("Time")
plt.ylabel("Usage (%)")
plt.title("System Resource Usage Over Time")
plt.legend()
plt.grid(False)
plt.xticks(rotation=0)
plt.tight_layout()
plt.show()
