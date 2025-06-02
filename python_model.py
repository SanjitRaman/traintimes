from datetime import datetime
import os
import time
import requests
import json
import configparser

# === Configuration ===
CONFIG_PATH = os.path.join(os.path.dirname(__file__), "config")

def load_config():
    config = configparser.ConfigParser()
    try:
        config.read(CONFIG_PATH)
        section = config["DEFAULT"]
        return section["API_KEY"], section["STATION_CODE"], section["DEST_CRS"]
    except Exception as e:
        print(f"Error loading config: {e}")
        exit(1)

API_KEY, STATION_CODE, DEST_CRS = load_config()
TRAINS_PER_SCREEN = 2
TOTAL_TRAINS = 6
CALLING_AT_SCROLL_LEN = 40

FETCH_INTERVAL_SECONDS = 60
SCROLL_INTERVAL = 0.5
BLINK_COUNT = 6  # 3 visible + 3 off

BOARD_WIDTH = 60


def get_departure_data():
    url = f"https://api1.raildata.org.uk/1010-live-departure-board-dep1_2/LDBWS/api/20220120/GetDepBoardWithDetails/{STATION_CODE}"
    headers = {
        "x-apikey": API_KEY,
        "User-Agent": "TrainTimesApp/1.0"
    }
    params = {
        "numRows": 12,
        "timeOffset": 0,
        "timeWindow": 360
    }

    try:
        response = requests.get(url, headers=headers, params=params, timeout=10)
        response.raise_for_status()
    except Exception as e:
        print(f"Error fetching API: {e}")
        return []

    data = response.json()
    services = []
    for service in data.get("trainServices", []):
        destination = service.get("destination", [{}])[0]
        if destination.get("crs") != DEST_CRS:
            continue

        calling_at = []
        groups = service.get("subsequentCallingPoints", [])
        if groups and "callingPoint" in groups[0]:
            for point in groups[0]["callingPoint"]:
                calling_at.append(point.get("locationName", ""))

        services.append({
            "std": service.get("std", "??:??"),
            "etd": service.get("etd", "??:??"),
            "dest": destination.get("locationName", "Unknown"),
            "calling_at": calling_at,
            "isCancelled": service.get("isCancelled", False),
        })

        if len(services) == TOTAL_TRAINS:
            break

    return services


def print_board(services, offset, scroll_offset, mode, blink_on):
    os.system('cls' if os.name == 'nt' else 'clear')
    border = "=" * BOARD_WIDTH
    print(border)
    view = services[offset:offset + TRAINS_PER_SCREEN] if services else []

    for idx in range(TRAINS_PER_SCREEN):
        if idx < len(view):
            service = view[idx]
            etd = service["etd"]
            is_delayed = etd.lower() != "on time" and not service["isCancelled"]
            etd_display = f"Exp {etd}" if is_delayed else ("CANCELLED" if service["isCancelled"] else "On time")

            # Line 1: Time  Destination                   ETD (right aligned)
            left = f"{service['std']}  {service['dest']}"
            right = etd_display.rjust(BOARD_WIDTH - len(left) - 1)
            print(f"{left}{right}")

            # Line 2
            if idx == 0:
                if service["isCancelled"]:
                    cancel_msg = "⚠ CANCELLED" if blink_on else " " * 12
                    print(f"{cancel_msg:<{BOARD_WIDTH}}")
                elif is_delayed and mode == "delay":
                    delay_msg = f"⚠ Delayed – Expected at {etd}" if blink_on else ""
                    print(f"{delay_msg:<{BOARD_WIDTH}}")
                else:
                    calling = ", ".join(service["calling_at"])
                    padded = calling + "   "
                    scroll = (padded * 2)[scroll_offset:scroll_offset + CALLING_AT_SCROLL_LEN]
                    print(f"Calling at: {scroll:<{BOARD_WIDTH - 11}}")
            else:
                print("")

        else:
            print(" " * BOARD_WIDTH)
            print(" " * BOARD_WIDTH)

    now = datetime.now().strftime("%H:%M:%S")
    print(now.center(BOARD_WIDTH))
    print(border)


def main():
    services = get_departure_data()
    last_fetch = time.time()
    view_index = 0
    scroll_offset = 0
    scroll_steps_needed = 10
    mode = "delay"
    blink_counter = 0

    def reset_scroll():
        nonlocal scroll_offset, scroll_steps_needed, mode, blink_counter
        scroll_offset = 0
        mode = "delay"
        blink_counter = 0
        if services and view_index < len(services):
            calling = ", ".join(services[view_index]["calling_at"])
            scroll_len = len(calling + "   ")
            scroll_steps_needed = max(scroll_len - CALLING_AT_SCROLL_LEN + 1, 1)
        else:
            scroll_steps_needed = 10

    reset_scroll()

    while True:
        now = time.time()

        # Refresh data
        if now - last_fetch >= FETCH_INTERVAL_SECONDS:
            services = get_departure_data()
            last_fetch = now
            view_index = 0
            reset_scroll()

        # Handle delay blinking
        if mode == "delay":
            if blink_counter < BLINK_COUNT:
                blink_on = blink_counter % 2 == 0
                print_board(services, view_index, 0, "delay", blink_on)
                blink_counter += 1
                time.sleep(SCROLL_INTERVAL)
                continue
            else:
                mode = "scroll"
                scroll_offset = 0

        # Normal scroll mode
        blink_on = int(time.time() * 3) % 2 == 0
        print_board(services, view_index, scroll_offset, "scroll", blink_on)
        scroll_offset += 1

        if scroll_offset >= scroll_steps_needed:
            print_board([], 0, 0, "scroll", False)
            time.sleep(1)
            view_index += 1
            if view_index > TOTAL_TRAINS - TRAINS_PER_SCREEN:
                view_index = 0
            reset_scroll()

        time.sleep(SCROLL_INTERVAL)


main()
