import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns
from preprocess import load_and_clean
import numpy as np

def analyze_preprocessed_data():
    # Load the preprocessed data
    print("Loading and preprocessing data...")
    df = load_and_clean()
    
    # Basic information about the dataset
    print("\n=== Dataset Overview ===")
    print(f"Number of samples: {len(df)}")
    print(f"Number of features: {len(df.columns)}")
    print("\n=== Features ===")
    for col in df.columns:
        print(f"{col}: {df[col].dtype}")
    
    # Summary statistics
    print("\n=== Summary Statistics ===")
    print(df.describe())
    
    # Create visualizations
    print("\n=== Creating visualizations... ===")
    
    # Set up the plotting style
    plt.style.use('default')  # Using default style instead of seaborn
    
    # 1. Time series plot of key metrics
    plt.figure(figsize=(15, 8))
    key_metrics = ['cpu_usage', 'memory_usage', 'memory_fragmentation']
    for metric in key_metrics:
        if metric in df.columns:
            plt.plot(df['timestamp'], df[metric], label=metric)
    plt.title('Key Metrics Over Time')
    plt.xlabel('Timestamp')
    plt.ylabel('Standardized Values')
    plt.legend()
    plt.xticks(rotation=45)
    plt.tight_layout()
    plt.savefig('key_metrics.png')
    plt.close()
    
    # 2. Correlation heatmap
    plt.figure(figsize=(12, 10))
    numeric_cols = df.select_dtypes(include=[np.number]).columns
    correlation = df[numeric_cols].corr()
    plt.imshow(correlation, cmap='coolwarm', aspect='auto')
    plt.colorbar()
    plt.xticks(range(len(numeric_cols)), numeric_cols, rotation=90)
    plt.yticks(range(len(numeric_cols)), numeric_cols)
    plt.title('Feature Correlation Heatmap')
    plt.tight_layout()
    plt.savefig('correlation_heatmap.png')
    plt.close()
    
    # 3. Distribution plots for main metrics
    main_metrics = ['cpu_usage', 'memory_usage', 'memory_fragmentation', 
                   'disk_read_latency', 'disk_write_latency', 'memory_virtual_usage']
    
    plt.figure(figsize=(15, 10))
    for i, metric in enumerate(main_metrics, 1):
        if metric in df.columns:
            plt.subplot(2, 3, i)
            plt.hist(df[metric], bins=20, density=True)
            plt.title(f'{metric} Distribution')
    plt.tight_layout()
    plt.savefig('distributions.png')
    plt.close()
    
    # Check for any remaining missing values
    missing_values = df.isnull().sum()
    if missing_values.any():
        print("\n=== Missing Values ===")
        print(missing_values[missing_values > 0])
    else:
        print("\nNo missing values found in the dataset.")
    
    # Value ranges for bounded metrics
    print("\n=== Bounded Metrics Range Check ===")
    bounded_metrics = ['memory_usage', 'memory_fragmentation', 'memory_virtual_usage']
    for metric in bounded_metrics:
        if metric in df.columns:
            print(f"{metric}: Min = {df[metric].min():.3f}, Max = {df[metric].max():.3f}")
    
    print("\nAnalysis complete! Visualizations have been saved as PNG files.")
    return df

if __name__ == "__main__":
    df = analyze_preprocessed_data() 