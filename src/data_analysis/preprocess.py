import pandas as pd
import sqlite3
import numpy as np
from sklearn.preprocessing import StandardScaler, MinMaxScaler
from datetime import datetime

def convert_size_to_bytes(size_str):
    # Convert size strings (KB, MB, GB, B) to bytes
    if pd.isna(size_str):
        return 0
    
    # If already numeric, return as is
    if isinstance(size_str, (int, float)):
        return size_str
        
    if isinstance(size_str, str):
        if size_str == "0.00 B":
            return 0
            
        multipliers = {
            'B': 1,
            'KB': 1024,
            'MB': 1024**2,
            'GB': 1024**3
        }
        
        number = float(size_str.split()[0])
        unit = size_str.split()[1]
        return number * multipliers[unit]
    return size_str

def convert_latency_to_ms(latency_str):
    # Convert latency strings (ms, s) to milliseconds
    if pd.isna(latency_str):
        return 0
        
    # If already numeric, assume it's in milliseconds
    if isinstance(latency_str, (int, float)):
        return latency_str
        
    if isinstance(latency_str, str):
        if latency_str == "0.00 ms":
            return 0
        
        if 'ms' in latency_str:
            return float(latency_str.split()[0])
        elif 's' in latency_str:
            return float(latency_str.split()[0]) * 1000
    return latency_str

def convert_percentage(value):
    # Handle percentage values that could be either strings with % or floats
    if pd.isna(value):
        return 0.0
        
    # If already numeric, assume decimal form
    if isinstance(value, (int, float)):
        return value if value <= 1 else value / 100
        
    if isinstance(value, str):
        # Remove % if present and convert to float
        value = value.rstrip('%')
        return float(value) / 100
    return value

def load_and_clean():
    # Load data from SQLite database and perform data cleaning operations
    # Returns cleaned DataFrame ready for ML analysis
    conn = sqlite3.connect('system_metrics.db')
    df = pd.read_sql_query("SELECT * FROM system_metrics", conn)
    conn.close()
    
    # Convert timestamp to datetime if it's not already
    if df['timestamp'].dtype != 'datetime64[ns]':
        df['timestamp'] = pd.to_datetime(df['timestamp'])
    
    # Clean percentage metrics
    percentage_cols = ['cpu_usage', 'memory_usage', 'memory_frag', 'virtual_usage']
    for col in percentage_cols:
        if col in df.columns:
            df[col] = df[col].apply(convert_percentage)
    
    # Convert size metrics to bytes
    size_cols = ['memory_free', 'largest_free', 'total_free', 'page_file', 
                 'disk_read', 'disk_write', 'virtual_total', 'virtual_free']
    for col in size_cols:
        if col in df.columns:
            df[col] = df[col].apply(convert_size_to_bytes)
    
    # Convert latency metrics to milliseconds
    latency_cols = ['read_latency', 'write_latency']
    for col in latency_cols:
        if col in df.columns:
            df[col] = df[col].apply(convert_latency_to_ms)
    
    # Integer metrics (no conversion needed)
    int_cols = ['block_count', 'blocks_small', 'blocks_medium', 'blocks_large',
                'split_ios', 'crashes', 'errors', 'warnings']
    for col in int_cols:
        if col in df.columns:
            df[col] = pd.to_numeric(df[col], errors='coerce')
    
    # Calculate derived features
    if all(col in df.columns for col in ['memory_free', 'virtual_total']):
        df['memory_utilization'] = 1 - (df['memory_free'] / df['virtual_total'])
    
    if 'split_ios' in df.columns:
        df['io_pressure'] = df['split_ios'] / df['split_ios'].mean()
    
    if all(col in df.columns for col in ['read_latency', 'write_latency']):
        df['latency_ratio'] = df['read_latency'] / (df['write_latency'] + 1)
    
    # Remove duplicates
    df = df.drop_duplicates()
    
    # Handle missing values
    df = df.fillna(df.mean(numeric_only=True))
    
    # Scale the features appropriately
    scaler = StandardScaler()
    minmax = MinMaxScaler()
    
    # Use StandardScaler for continuous metrics
    continuous_cols = ['cpu_usage', 'memory_utilization', 'io_pressure', 'latency_ratio']
    continuous_cols = [col for col in continuous_cols if col in df.columns]
    if continuous_cols:
        df[continuous_cols] = scaler.fit_transform(df[continuous_cols])
    
    # Use MinMaxScaler for bounded metrics (0-1)
    bounded_cols = ['memory_usage', 'memory_frag', 'virtual_usage']
    bounded_cols = [col for col in bounded_cols if col in df.columns]
    if bounded_cols:
        df[bounded_cols] = minmax.fit_transform(df[bounded_cols])
    
    # Log transform highly skewed metrics
    skewed_cols = ['disk_read', 'disk_write', 'read_latency', 'write_latency']
    skewed_cols = [col for col in skewed_cols if col in df.columns]
    for col in skewed_cols:
        df[col] = np.log1p(df[col])
    
    return df
        
