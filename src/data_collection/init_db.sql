-- Create system metrics table
CREATE TABLE IF NOT EXISTS system_metrics (
    timestamp TEXT,
    cpu_usage REAL,
    cpu_temperature REAL,
    memory_usage REAL,
    page_file_usage REAL,
    disk_read_bytes REAL,
    disk_write_bytes REAL,
    disk_queue_length INTEGER,
    network_bytes_in REAL,
    network_bytes_out REAL,
    crash_count INTEGER,
    error_count INTEGER,
    warning_count INTEGER,
    disk_read_latency REAL,
    disk_write_latency REAL,
    disk_split_io INTEGER,
    memory_fragmentation REAL,
    memory_largest_free INTEGER,
    memory_total_free INTEGER,
    memory_block_count INTEGER,      -- Number of free memory blocks
    memory_avg_block_size REAL,      -- Average size of free blocks
    memory_small_blocks INTEGER,     -- Blocks < 1MB
    memory_medium_blocks INTEGER,    -- Blocks 1MB-16MB
    memory_large_blocks INTEGER,     -- Blocks > 16MB
    memory_virtual_total INTEGER,    -- Total virtual memory
    memory_virtual_free INTEGER,     -- Total free virtual memory
    memory_virtual_usage REAL,       -- Virtual memory usage percentage
    PRIMARY KEY (timestamp)
);

-- Create process metrics table
CREATE TABLE IF NOT EXISTS process_metrics (
    timestamp TEXT,
    process_id INTEGER,
    parent_process_id INTEGER,
    process_name TEXT,
    cpu_usage REAL,
    working_set_bytes INTEGER,
    private_bytes INTEGER,
    handle_count INTEGER,
    thread_count INTEGER,
    io_read_bytes INTEGER,
    io_write_bytes INTEGER,
    page_faults INTEGER,
    PRIMARY KEY (timestamp, process_id)
); 