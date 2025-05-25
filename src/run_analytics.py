from analytics import classify_idle_active_cpu_idle_active
from detection import detect_spike
import sqlite3
import pandas as pd

conn = sqlite3.connect('build/optimizer.db')
df = pd.read_sql_query("SELECT * FROM system_stats", conn, parse_dates=['timestamp'])

df = classify_idle_active_cpu_idle_active(df)
df = detect_spike(df)

# Save modified DataFrame back to DB (overwrite table)
df.to_sql("system_stats", conn, if_exists="replace", index=False)
conn.close()
