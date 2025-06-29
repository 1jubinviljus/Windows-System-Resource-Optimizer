#!/usr/bin/env python3
"""
Main Controller for Windows System Resource Optimizer
Coordinates data collection, optimization, and analysis
"""

import subprocess
import time
import sqlite3
import threading
import signal
import sys
import os
from pathlib import Path
from datetime import datetime

# Add the data_analysis directory to the path
sys.path.append(str(Path(__file__).parent / 'data_analysis'))

from data_analysis.display_metrics import display_basic_metrics
from data_analysis.anomaly_detection import detect_anomalies

class SystemOptimizer:
    def __init__(self):
        self.running = True
        self.db_path = "system_metrics.db"
        self.optimization_interval = 30  # seconds
        self.analysis_interval = 60      # seconds
        
    def signal_handler(self, signum, frame):
        """Handle shutdown signals gracefully"""
        print("\nShutting down System Optimizer...")
        self.running = False
        
    def run_data_collection(self):
        """Run the C data collection system"""
        try:
            # Run the compiled C executable
            process = subprocess.Popen(
                ["./system_monitor.exe"],
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True
            )
            
            print("Data collection started...")
            return process
            
        except FileNotFoundError:
            print("Error: system_monitor.exe not found. Please build the project first.")
            return None
            
    def run_optimization_cycle(self):
        """Run periodic optimization checks"""
        while self.running:
            try:
                # Log when check occurs
                print(f"[{datetime.now().strftime('%H:%M:%S')}] Running optimization check...")
                
                # Connect to database
                conn = sqlite3.connect(self.db_path)
                cursor = conn.cursor()
                
                # Check for engineering software and optimize
                cursor.execute("""
                    SELECT DISTINCT process_name, process_id 
                    FROM process_metrics 
                    WHERE timestamp >= datetime('now', '-5 minutes')
                    AND (LOWER(process_name) LIKE '%solidworks%' 
                         OR LOWER(process_name) LIKE '%autocad%' 
                         OR LOWER(process_name) LIKE '%matlab%'
                         OR LOWER(process_name) LIKE '%ansys%')
                """)
                
                engineering_processes = cursor.fetchall()
                
                if engineering_processes:
                    print(f"Found {len(engineering_processes)} engineering processes")
                    # Here you would call your C optimization functions
                    # For now, just log the detection
                    for process_name, process_id in engineering_processes:
                        print(f"Engineering process detected: {process_name} (PID: {process_id})")
                else:
                    print("No engineering processes found")
                
                conn.close()
                
            except Exception as e:
                print(f"Optimization cycle error: {e}")
                
            time.sleep(self.optimization_interval)
            
    def run_analysis_cycle(self):
        """Run periodic data analysis"""
        while self.running:
            try:
                # Log when check occurs
                print(f"[{datetime.now().strftime('%H:%M:%S')}] Running analysis check...")
                
                # Run anomaly detection
                anomalies = detect_anomalies(self.db_path)
                if anomalies:
                    print(f"Anomalies detected: {len(anomalies)}")
                    
                # Display system overview
                display_basic_metrics()
                
            except Exception as e:
                print(f"Analysis cycle error: {e}")
                
            time.sleep(self.analysis_interval)
            
    def start(self):
        """Start the complete system optimizer"""
        print("Starting Windows System Resource Optimizer...")
        print("=" * 50)
        
        # Set up signal handling
        signal.signal(signal.SIGINT, self.signal_handler)
        signal.signal(signal.SIGTERM, self.signal_handler)
        
        # Start data collection
        data_collection_process = self.run_data_collection()
        if not data_collection_process:
            return
            
        # Start optimization thread
        optimization_thread = threading.Thread(target=self.run_optimization_cycle)
        optimization_thread.daemon = True
        optimization_thread.start()
        
        # Start analysis thread
        analysis_thread = threading.Thread(target=self.run_analysis_cycle)
        analysis_thread.daemon = True
        analysis_thread.start()
        
        print("System Optimizer is running...")
        print("Press Ctrl+C to stop")
        
        try:
            # Keep main thread alive
            while self.running:
                time.sleep(1)
                
        except KeyboardInterrupt:
            print("\nReceived interrupt signal")
            
        finally:
            # Cleanup
            self.running = False
            if data_collection_process:
                data_collection_process.terminate()
                data_collection_process.wait()
            print("System Optimizer stopped.")

def main():
    """Main entry point"""
    optimizer = SystemOptimizer()
    optimizer.start()

if __name__ == "__main__":
    main() 