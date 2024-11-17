import sqlite3
import os
import requests
from datetime import datetime, timedelta
import matplotlib.pyplot as plt

API_URL = "https://api.bybit.com/v5/market/kline"

# Example usage:
# initialize_db("data/market_data.db")

def initialize_db(db_path):
    os.makedirs(os.path.dirname(db_path), exist_ok=True)
    conn = sqlite3.connect(db_path)
    cursor = conn.cursor()
    cursor.execute("""
        CREATE TABLE IF NOT EXISTS market_data (
            timestamp INTEGER PRIMARY KEY,
            symbol TEXT NOT NULL,
            interval TEXT NOT NULL,
            open REAL,
            high REAL,
            low REAL,
            close REAL,
            volume REAL
        )
    """)
    conn.commit()
    conn.close()
    print(f"Database initialized at {db_path}")


# Example usage:
# fetch_and_store_market_data(
#     "data/market_data.db", "BTCUSDT", "1m",
#     datetime(2024, 11, 1), datetime(2024, 11, 15)
# )

def fetch_and_store_market_data(db_path, symbol, interval, start_time, end_time):
    conn = sqlite3.connect(db_path)
    cursor = conn.cursor()
    
    # Fetch existing timestamps for the symbol and interval
    cursor.execute("""
        SELECT timestamp FROM market_data 
        WHERE symbol = ? AND interval = ? ORDER BY timestamp ASC
    """, (symbol, interval))
    existing_timestamps = {row[0] for row in cursor.fetchall()}
    
    # Calculate all expected timestamps in the range
    expected_timestamps = set(
        int((start_time + timedelta(minutes=i)).timestamp())
        for i in range(0, int((end_time - start_time).total_seconds() // 60), int(interval.strip("m")))
    )
    
    # Determine missing timestamps
    missing_timestamps = sorted(expected_timestamps - existing_timestamps)
    if not missing_timestamps:
        print("All data is already cached.")
        return

    # Group missing timestamps into contiguous intervals for batch requests
    batch_size = 200  # Max data points per API request
    batches = []
    current_batch = [missing_timestamps[0]]
    for ts in missing_timestamps[1:]:
        if ts - current_batch[-1] <= int(interval.strip("m")) * 60:
            current_batch.append(ts)
        else:
            batches.append((current_batch[0], current_batch[-1]))
            current_batch = [ts]
    if current_batch:
        batches.append((current_batch[0], current_batch[-1]))
    
    # Request missing data and store in the database
    for start_ts, end_ts in batches:
        params = {
            "category": "linear",
            "symbol": symbol,
            "interval": interval,
            "start": start_ts,
            "end": end_ts
        }
        response = requests.get(API_URL, params=params)
        response_data = response.json()
        if "result" not in response_data:
            print(f"Error fetching data for batch {start_ts}-{end_ts}: {response.text}")
            continue
        
        for entry in response_data["result"]["list"]:
            cursor.execute("""
                INSERT OR IGNORE INTO market_data 
                (timestamp, symbol, interval, open, high, low, close, volume)
                VALUES (?, ?, ?, ?, ?, ?, ?, ?)
            """, (
                entry["timestamp"], symbol, interval, 
                entry["open"], entry["high"], entry["low"], 
                entry["close"], entry["volume"]
            ))
        conn.commit()
        print(f"Data for batch {start_ts}-{end_ts} cached.")
    
    conn.close()
    print("Market data fetch and cache completed.")


# Example usage:
# plot_market_data("data/market_data.db", "BTCUSDT", "1m", datetime(2024, 11, 1), datetime(2024, 11, 5))

def plot_market_data(db_path, symbol, interval, start_time, end_time):
    conn = sqlite3.connect(db_path)
    cursor = conn.cursor()
    
    cursor.execute("""
        SELECT timestamp, open, high, low, close, volume 
        FROM market_data 
        WHERE symbol = ? AND interval = ? AND timestamp BETWEEN ? AND ?
        ORDER BY timestamp ASC
    """, (symbol, interval, int(start_time.timestamp()), int(end_time.timestamp())))
    
    data = cursor.fetchall()
    conn.close()
    
    if not data:
        raise ValueError("Insufficient data in the database for the requested range.")
    
    timestamps, opens, highs, lows, closes, volumes = zip(*data)
    plt.figure(figsize=(10, 5))
    plt.plot(timestamps, closes, label="Close Price")
    plt.title(f"{symbol} Price Data")
    plt.xlabel("Timestamp")
    plt.ylabel("Price")
    plt.legend()
    plt.grid()
    plt.show()

