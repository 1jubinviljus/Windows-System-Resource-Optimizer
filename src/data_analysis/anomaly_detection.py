import pandas as pd
import matplotlib.pyplot as plt
from sklearn.ensemble import IsolationForest
import preprocess

# Load clean numerical data
df = preprocess.load_and_clean()  # Ensure this returns only relevant numeric columns
features = ['cpu_usage', 'memory_usage', 'memory_large_blocks', 'memory_virtual_total' ]  # Example features
data = df[features]

# Fit Isolation Forest
iso = IsolationForest(contamination=0.05, random_state=42)
df['anomaly'] = iso.fit_predict(data)

# Plot anomalies on one metric (e.g., CPU usage)
plt.figure(figsize=(10, 6))
plt.plot(df.index, df['cpu_usage'], label='CPU Usage')
plt.scatter(df[df['anomaly'] == -1].index, df[df['anomaly'] == -1]['cpu_usage'],
            color='red', label='Anomaly')
plt.legend()
plt.title("CPU Usage with Anomalies")
plt.show()

