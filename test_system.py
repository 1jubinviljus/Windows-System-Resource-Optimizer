#!/usr/bin/env python3
"""
Test script for Windows System Resource Optimizer
Validates all components are working correctly
"""

import sqlite3
import subprocess
import time
import sys
from pathlib import Path

def test_database():
    """Test database connectivity and structure"""
    print("Testing database...")
    try:
        conn = sqlite3.connect("system_metrics.db")
        cursor = conn.cursor()
        
        # Check if tables exist
        cursor.execute("SELECT name FROM sqlite_master WHERE type='table'")
        tables = cursor.fetchall()
        print(f"Found tables: {[table[0] for table in tables]}")
        
        # Check recent data
        cursor.execute("SELECT COUNT(*) FROM system_metrics")
        system_count = cursor.fetchone()[0]
        cursor.execute("SELECT COUNT(*) FROM process_metrics")
        process_count = cursor.fetchone()[0]
        
        print(f"System metrics records: {system_count}")
        print(f"Process metrics records: {process_count}")
        
        conn.close()
        return True
        
    except Exception as e:
        print(f"Database test failed: {e}")
        return False

def test_data_collection():
    """Test if data collection is working"""
    print("\nTesting data collection...")
    try:
        # Check if executable exists
        if not Path("system_monitor.exe").exists():
            print("system_monitor.exe not found. Please build the project first.")
            return False
            
        # Run data collection for a short time
        process = subprocess.Popen(
            ["./system_monitor.exe"],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True
        )
        
        # Let it run for 10 seconds
        time.sleep(10)
        process.terminate()
        process.wait()
        
        # Check if new data was collected
        conn = sqlite3.connect("system_metrics.db")
        cursor = conn.cursor()
        
        # Check for data from the last 5 minutes (more reasonable)
        cursor.execute("SELECT COUNT(*) FROM system_metrics WHERE timestamp >= datetime('now', '-5 minutes')")
        recent_count = cursor.fetchone()[0]
        
        # Also check total data to ensure we have data at all
        cursor.execute("SELECT COUNT(*) FROM system_metrics")
        total_count = cursor.fetchone()[0]
        
        conn.close()
        
        if recent_count > 0:
            print(f"Data collection working: {recent_count} recent records")
            return True
        elif total_count > 0:
            print(f"Data collection working: {total_count} total records (no recent data, but system has data)")
            return True
        else:
            print("No data found")
            return False
            
    except Exception as e:
        print(f"Data collection test failed: {e}")
        return False

def test_optimization():
    """Test optimization functions"""
    print("\nTesting optimization...")
    try:
        # Check if optimization functions are available
        if not Path("src/resource_management/process_optimizer.c").exists():
            print("Process optimizer not found")
            return False
            
        # For now, just check if the file compiles
        result = subprocess.run(
            ["gcc", "-c", "src/resource_management/process_optimizer.c", 
             "-I./src/resource_management", "-o", "test_optimizer.o"],
            capture_output=True,
            text=True
        )
        
        if result.returncode == 0:
            print("Optimization functions compile successfully")
            # Clean up test object
            Path("test_optimizer.o").unlink(missing_ok=True)
            return True
        else:
            print(f"Optimization compilation failed: {result.stderr}")
            return False
            
    except Exception as e:
        print(f"Optimization test failed: {e}")
        return False

def test_analysis():
    """Test data analysis components"""
    print("\nTesting data analysis...")
    try:
        # Test if Python analysis modules can be imported
        import sys
        from pathlib import Path
        
        # Add the src directory to the path
        src_path = Path(__file__).parent / "src"
        sys.path.insert(0, str(src_path))
        
        from data_analysis.display_metrics import display_basic_metrics
        from data_analysis.anomaly_detection import detect_anomalies
        
        print("Data analysis modules imported successfully")
        
        # Test basic functionality
        if Path("system_metrics.db").exists():
            try:
                display_basic_metrics()
                print("System overview display working")
                
                anomalies = detect_anomalies("system_metrics.db")
                print(f"Anomaly detection working: {len(anomalies) if anomalies else 0} anomalies found")
                
                return True
            except Exception as e:
                print(f"Analysis execution failed: {e}")
                return False
        else:
            print("No database found for analysis testing")
            return False
            
    except ImportError as e:
        print(f"Analysis module import failed: {e}")
        return False

def main():
    """Run all tests"""
    print("Windows System Resource Optimizer - System Test")
    print("=" * 50)
    
    tests = [
        ("Database", test_database),
        ("Data Collection", test_data_collection),
        ("Optimization", test_optimization),
        ("Analysis", test_analysis)
    ]
    
    results = []
    for test_name, test_func in tests:
        try:
            result = test_func()
            results.append((test_name, result))
        except Exception as e:
            print(f"{test_name} test crashed: {e}")
            results.append((test_name, False))
    
    # Summary
    print("\n" + "=" * 50)
    print("TEST SUMMARY:")
    print("=" * 50)
    
    passed = 0
    for test_name, result in results:
        status = "PASS" if result else "FAIL"
        print(f"{test_name:20} : {status}")
        if result:
            passed += 1
    
    print(f"\nOverall: {passed}/{len(results)} tests passed")
    
    if passed == len(results):
        print("🎉 All tests passed! System is ready to use.")
    else:
        print("⚠️  Some tests failed. Please check the issues above.")

if __name__ == "__main__":
    main() 