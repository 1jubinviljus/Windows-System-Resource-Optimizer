import sqlite3
import pandas as pd
from tabulate import tabulate
from datetime import datetime

def format_bytes(bytes_value):
    try:
        bytes_value = float(bytes_value)
        for unit in ['B', 'KB', 'MB', 'GB']:
            if bytes_value < 1024:
                return f"{bytes_value:.2f} {unit}"
            bytes_value /= 1024
        return f"{bytes_value:.2f} TB"
    except (ValueError, TypeError):
        return "N/A"

def format_percentage(value):
    try:
        return f"{float(value):.2f}%"
    except (ValueError, TypeError):
        return "N/A"

def format_latency(value):
    try:
        value = float(value)
        if value < 1:
            return f"{value*1000:.2f} ms"
        return f"{value:.2f} s"
    except (ValueError, TypeError):
        return "N/A"

def display_basic_metrics(limit=10):
    conn = sqlite3.connect('system_metrics.db')
    
    # Basic system metrics
    query = """
    SELECT 
        timestamp,
        cpu_usage,
        cpu_temperature,
        memory_usage,
        page_file_usage,
        disk_read_bytes,
        disk_write_bytes,
        network_bytes_in,
        network_bytes_out
    FROM system_metrics 
    ORDER BY timestamp DESC 
    LIMIT ?
    """
    
    df = pd.read_sql_query(query, conn, params=(limit,))
    
    # Format the data
    df['timestamp'] = pd.to_datetime(df['timestamp'])
    df['timestamp'] = df['timestamp'].dt.strftime('%Y-%m-%d %H:%M:%S')
    df['cpu_usage'] = df['cpu_usage'].apply(format_percentage)
    df['cpu_temperature'] = df['cpu_temperature'].apply(lambda x: f"{x:.1f}°C" if x > 0 else "N/A")
    df['memory_usage'] = df['memory_usage'].apply(format_percentage)
    df['page_file_usage'] = df['page_file_usage'].apply(format_percentage)
    df['disk_read_bytes'] = df['disk_read_bytes'].apply(format_bytes)
    df['disk_write_bytes'] = df['disk_write_bytes'].apply(format_bytes)
    df['network_bytes_in'] = df['network_bytes_in'].apply(format_bytes)
    df['network_bytes_out'] = df['network_bytes_out'].apply(format_bytes)
    
    # Rename columns for display
    df.columns = [
        'Timestamp',
        'CPU Usage',
        'CPU Temp',
        'Memory Usage',
        'Page File',
        'Disk Read',
        'Disk Write',
        'Network In',
        'Network Out'
    ]
    
    print("\nSystem Resource Monitor - Basic Metrics")
    print("=" * 120)
    print(tabulate(df, headers='keys', tablefmt='pretty', showindex=False))

def display_advanced_metrics(limit=5):
    conn = sqlite3.connect('system_metrics.db')
    
    # Advanced metrics
    query = """
    SELECT 
        timestamp,
        disk_read_latency,
        disk_write_latency,
        disk_split_io,
        memory_fragmentation,
        memory_largest_free,
        memory_total_free,
        crash_count,
        error_count,
        warning_count
    FROM system_metrics 
    ORDER BY timestamp DESC 
    LIMIT ?
    """
    
    df = pd.read_sql_query(query, conn, params=(limit,))
    
    # Format the data
    df['timestamp'] = pd.to_datetime(df['timestamp'])
    df['timestamp'] = df['timestamp'].dt.strftime('%Y-%m-%d %H:%M:%S')
    df['disk_read_latency'] = df['disk_read_latency'].apply(format_latency)
    df['disk_write_latency'] = df['disk_write_latency'].apply(format_latency)
    df['memory_fragmentation'] = df['memory_fragmentation'].apply(format_percentage)
    df['memory_largest_free'] = df['memory_largest_free'].apply(format_bytes)
    df['memory_total_free'] = df['memory_total_free'].apply(format_bytes)
    
    # Rename columns for display
    df.columns = [
        'Timestamp',
        'Read Latency',
        'Write Latency',
        'Split I/Os',
        'Memory Frag',
        'Largest Free',
        'Total Free',
        'Crashes',
        'Errors',
        'Warnings'
    ]
    
    print("\nSystem Resource Monitor - Advanced Metrics")
    print("=" * 120)
    print(tabulate(df, headers='keys', tablefmt='pretty', showindex=False))

def display_system_health():
    conn = sqlite3.connect('system_metrics.db')
    
    # Get latest record
    latest = pd.read_sql_query("""
        SELECT 
            cpu_usage,
            memory_usage,
            disk_queue_length,
            memory_fragmentation,
            error_count + warning_count as issue_count
        FROM system_metrics 
        ORDER BY timestamp DESC 
        LIMIT 1
    """, conn)
    
    # Get averages
    averages = pd.read_sql_query("""
        SELECT 
            ROUND(AVG(cpu_usage), 2) as avg_cpu,
            ROUND(AVG(memory_usage), 2) as avg_mem,
            ROUND(AVG(disk_queue_length), 2) as avg_disk_queue,
            ROUND(AVG(memory_fragmentation), 2) as avg_frag,
            ROUND(AVG(error_count + warning_count), 2) as avg_issues
        FROM system_metrics
    """, conn)
    
    print("\nSystem Health Summary")
    print("=" * 40)
    print(f"CPU Usage: Current: {format_percentage(latest['cpu_usage'][0])} | Average: {format_percentage(averages['avg_cpu'][0])}")
    print(f"Memory Usage: Current: {format_percentage(latest['memory_usage'][0])} | Average: {format_percentage(averages['avg_mem'][0])}")
    print(f"Memory Fragmentation: Current: {format_percentage(latest['memory_fragmentation'][0])} | Average: {format_percentage(averages['avg_frag'][0])}")
    print(f"Disk Queue Length: Current: {latest['disk_queue_length'][0]:.1f} | Average: {averages['avg_disk_queue'][0]:.1f}")
    print(f"System Issues: Current: {int(latest['issue_count'][0])} | Average: {averages['avg_issues'][0]:.1f}")
    
    conn.close()

if __name__ == "__main__":
    try:
        display_basic_metrics()
        print("\n")
        display_advanced_metrics()
        print("\n")
        display_system_health()
    except Exception as e:
        print(f"Error: {e}")
        print("\nMake sure:")
        print("1. The system_metrics.db file exists in the current directory")
        print("2. Required packages are installed (pip install pandas tabulate)") 