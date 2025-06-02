import sqlite3
import pandas as pd
from tabulate import tabulate
from datetime import datetime

def format_bytes(bytes_value):
    try:
        bytes_value = float(bytes_value)
        if bytes_value < 0:  # Invalid negative values
            return "N/A"
            
        # Handle potential overflow values
        if bytes_value >= float(2**64 - 1):  # Max unsigned 64-bit value
            return "N/A"
            
        for unit in ['B', 'KB', 'MB', 'GB', 'TB']:
            if bytes_value < 1024.0 or unit == 'TB':
                return f"{bytes_value:.2f} {unit}"
            bytes_value /= 1024.0
    except (ValueError, TypeError, OverflowError):
        return "N/A"

def format_percentage(value):
    try:
        value = float(value)
        if value < 0 or value > 100 or value >= float(2**64 - 1):
            return "N/A"
        if value == 0 and "virtual" in str(value).lower():  # Special case for virtual memory
            return "N/A"
        return f"{value:.1f}%"
    except (ValueError, TypeError, OverflowError):
        return "N/A"

def format_temperature(value):
    try:
        value = float(value)
        if value <= 0 or value > 150:  # Changed to catch 0°C as invalid
            return "N/A"
        return f"{value:.1f}°C"
    except (ValueError, TypeError):
        return "N/A"

def format_latency(value):
    try:
        value = float(value)
        if value < 0:  # Invalid negative values
            return "N/A"
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
        memory_total_free,
        page_file_usage,
        disk_read_bytes,
        disk_write_bytes
    FROM system_metrics 
    ORDER BY timestamp DESC 
    LIMIT ?
    """
    
    df = pd.read_sql_query(query, conn, params=(limit,))
    
    # Format the data
    df['timestamp'] = pd.to_datetime(df['timestamp'])
    df['timestamp'] = df['timestamp'].dt.strftime('%Y-%m-%d %H:%M:%S')
    df['cpu_usage'] = df['cpu_usage'].apply(format_percentage)
    df['cpu_temperature'] = df['cpu_temperature'].apply(format_temperature)
    df['memory_usage'] = df['memory_usage'].apply(format_percentage)
    df['memory_free'] = df['memory_total_free'].apply(format_bytes)
    df['page_file_usage'] = df['page_file_usage'].apply(format_bytes)
    df['disk_read_bytes'] = df['disk_read_bytes'].apply(format_bytes)
    df['disk_write_bytes'] = df['disk_write_bytes'].apply(format_bytes)
    
    # Drop the raw memory_total_free column
    df = df.drop(columns=['memory_total_free'])
    
    # Rename columns for display
    df.columns = [
        'Timestamp',
        'CPU Usage',
        'CPU Temp',
        'Memory Usage',
        'Memory Free',
        'Page File',
        'Disk Read',
        'Disk Write'
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
        memory_block_count,
        memory_avg_block_size,
        memory_small_blocks,
        memory_medium_blocks,
        memory_large_blocks,
        CASE 
            WHEN memory_virtual_total > 17592186044416 THEN NULL  -- Filter out values > 16TB
            ELSE memory_virtual_total 
        END as memory_virtual_total,
        CASE 
            WHEN memory_virtual_free > memory_virtual_total THEN NULL
            ELSE memory_virtual_free 
        END as memory_virtual_free,
        CASE 
            WHEN memory_virtual_usage <= 0 OR memory_virtual_usage > 100 THEN NULL
            ELSE memory_virtual_usage 
        END as memory_virtual_usage,
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
    df['memory_avg_block_size'] = df['memory_avg_block_size'].apply(format_bytes)
    df['memory_virtual_total'] = df['memory_virtual_total'].apply(format_bytes)
    df['memory_virtual_free'] = df['memory_virtual_free'].apply(format_bytes)
    df['memory_virtual_usage'] = df['memory_virtual_usage'].apply(format_percentage)
    
    # Split the display into three tables for better readability
    # Memory metrics
    memory_df = df[[
        'timestamp',
        'memory_fragmentation',
        'memory_largest_free',
        'memory_total_free',
        'memory_block_count',
        'memory_avg_block_size',
        'memory_small_blocks',
        'memory_medium_blocks',
        'memory_large_blocks'
    ]].copy()
    
    # Virtual memory metrics
    virtual_df = df[[
        'timestamp',
        'memory_virtual_usage',
        'memory_virtual_total',
        'memory_virtual_free'
    ]].copy()
    
    # System metrics
    system_df = df[[
        'timestamp',
        'disk_read_latency',
        'disk_write_latency',
        'disk_split_io',
        'crash_count',
        'error_count',
        'warning_count'
    ]].copy()
    
    # Rename columns for display
    memory_df.columns = [
        'Timestamp',
        'Memory Frag',
        'Largest Free',
        'Total Free',
        'Block Count',
        'Avg Block Size',
        'Blocks <1MB',
        'Blocks 1-16MB',
        'Blocks >16MB'
    ]
    
    virtual_df.columns = [
        'Timestamp',
        'Virtual Usage',
        'Virtual Total',
        'Virtual Free'
    ]
    
    system_df.columns = [
        'Timestamp',
        'Read Latency',
        'Write Latency',
        'Split I/Os',
        'Crashes',
        'Errors',
        'Warnings'
    ]
    
    print("\nSystem Resource Monitor - Memory Metrics")
    print("=" * 120)
    print(tabulate(memory_df, headers='keys', tablefmt='pretty', showindex=False))
    
    print("\nSystem Resource Monitor - Virtual Memory Metrics")
    print("=" * 120)
    print(tabulate(virtual_df, headers='keys', tablefmt='pretty', showindex=False))
    
    print("\nSystem Resource Monitor - System Metrics")
    print("=" * 120)
    print(tabulate(system_df, headers='keys', tablefmt='pretty', showindex=False))

def display_system_health():
    conn = sqlite3.connect('system_metrics.db')
    
    # Get latest record with additional memory metrics
    latest = pd.read_sql_query("""
        SELECT 
            cpu_usage,
            memory_usage,
            disk_queue_length,
            memory_fragmentation,
            memory_block_count,
            memory_avg_block_size,
            error_count + warning_count as issue_count
        FROM system_metrics 
        ORDER BY timestamp DESC 
        LIMIT 1
    """, conn)
    
    # Get averages with additional memory metrics
    averages = pd.read_sql_query("""
        SELECT 
            ROUND(AVG(cpu_usage), 2) as avg_cpu,
            ROUND(AVG(memory_usage), 2) as avg_mem,
            ROUND(AVG(disk_queue_length), 2) as avg_disk_queue,
            ROUND(AVG(memory_fragmentation), 2) as avg_frag,
            ROUND(AVG(memory_block_count), 2) as avg_blocks,
            ROUND(AVG(memory_avg_block_size), 2) as avg_block_size,
            ROUND(AVG(error_count + warning_count), 2) as avg_issues
        FROM system_metrics
    """, conn)
    
    print("\nSystem Health Summary")
    print("=" * 60)
    print(f"CPU Usage: Current: {format_percentage(latest['cpu_usage'][0])} | Average: {format_percentage(averages['avg_cpu'][0])}")
    print(f"Memory Usage: Current: {format_percentage(latest['memory_usage'][0])} | Average: {format_percentage(averages['avg_mem'][0])}")
    print(f"Memory Fragmentation: Current: {format_percentage(latest['memory_fragmentation'][0])} | Average: {format_percentage(averages['avg_frag'][0])}")
    print(f"Memory Blocks: Current: {latest['memory_block_count'][0]} | Average: {averages['avg_blocks'][0]:.0f}")
    print(f"Avg Block Size: Current: {format_bytes(latest['memory_avg_block_size'][0])} | Average: {format_bytes(averages['avg_block_size'][0])}")
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