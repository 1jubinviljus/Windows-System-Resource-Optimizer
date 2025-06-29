import pandas as pd
import matplotlib.pyplot as plt
from sklearn.ensemble import IsolationForest
from data_analysis import preprocess

def detect_anomalies(db_path="system_metrics.db"):
    """
    Detect anomalies in system metrics data
    
    Args:
        db_path (str): Path to the SQLite database
        
    Returns:
        list: List of anomaly timestamps or None if no anomalies found
    """
    try:
        # Load clean numerical data
        df = preprocess.load_and_clean()  # Ensure this returns only relevant numeric columns
        
        # Select features for anomaly detection
        features = ['cpu_usage', 'memory_usage', 'memory_large_blocks', 'memory_virtual_total']
        
        # Filter to only include features that exist in the dataframe
        available_features = [f for f in features if f in df.columns]
        
        if len(available_features) < 2:
            print("Warning: Not enough features available for anomaly detection")
            return None
            
        data = df[available_features]
        
        # Remove any rows with NaN values
        data = data.dropna()
        
        if len(data) < 10:
            print("Warning: Not enough data points for anomaly detection")
            return None
        
        # Fit Isolation Forest
        iso = IsolationForest(contamination=0.05, random_state=42)
        df['anomaly'] = iso.fit_predict(data)
        
        # Get anomaly timestamps
        anomalies = df[df['anomaly'] == -1]
        
        if len(anomalies) > 0:
            print(f"Found {len(anomalies)} anomalies")
            return anomalies['timestamp'].tolist()
        else:
            print("No anomalies detected")
            return None
            
    except Exception as e:
        print(f"Error in anomaly detection: {e}")
        return None

def plot_anomalies():
    """
    Plot anomalies on CPU usage metric
    """
    try:
        # Load clean numerical data
        df = preprocess.load_and_clean()
        features = ['cpu_usage', 'memory_usage', 'memory_large_blocks', 'memory_virtual_total']
        available_features = [f for f in features if f in df.columns]
        
        if len(available_features) < 2:
            print("Not enough features for anomaly detection")
            return
            
        data = df[available_features].dropna()
        
        if len(data) < 10:
            print("Not enough data points for anomaly detection")
            return
        
        # Fit Isolation Forest
        iso = IsolationForest(contamination=0.05, random_state=42)
        df['anomaly'] = iso.fit_predict(data)
        
        # Plot anomalies on CPU usage
        plt.figure(figsize=(10, 6))
        plt.plot(df.index, df['cpu_usage'], label='CPU Usage')
        plt.scatter(df[df['anomaly'] == -1].index, df[df['anomaly'] == -1]['cpu_usage'],
                    color='red', label='Anomaly')
        plt.legend()
        plt.title("CPU Usage with Anomalies")
        plt.show()
        
    except Exception as e:
        print(f"Error plotting anomalies: {e}")

# Run the plot if this file is executed directly
if __name__ == "__main__":
    plot_anomalies()

