#!/usr/bin/env python3
"""
APCU (Antenna Power Control Unit) Simulator
=============================================

Simulates the full APCU 4 Left [V1.0.0] antenna system, replicating
all subsystems visible in the APCU control interface:

  - APCU Controller    : Link status, Sequence On/Off, Array Voltage/Current
  - Temperature         : 3 Sensors (A, B, C) in Deg C
  - Board Data          : Input Voltage (V), Current (A)
  - Board Status        : CRC checks, CBIT, PBIT, Board Setting Check
  - Quadrant 0 (CB)     : 16 channels (Ch 0-15) with On/Off, Trip, Bit, Voltage, Current
  - Quadrant 1 (CF)     : 16 channels (Ch 0-15)
  - Quadrant 2 (CD)     : 16 channels (Ch 0-15)
  - Quadrant 3 (CE)     : 16 channels (Ch 0-15)
  - QTRMs               : Quad Transmit/Receive Module groupings
  - AQC                 : Antenna Quadrant Controllers (0,1,2,3)

Protocol:
  Sends JSON messages over TCP (default port 12345) with both
  component-level health and detailed subsystem telemetry.

Usage:
  python3 apcu_simulator.py [component_id] [options]

Examples:
  python3 apcu_simulator.py component_1
  python3 apcu_simulator.py antenna_1 --interval 1.5 --port 12345
  python3 apcu_simulator.py component_1 --mode degraded
"""

import socket
import json
import time
import random
import argparse
import sys
import math

# ────────────────────────────────────────────────────────────────
#  APCU Configuration Constants (from APCU 4 Left V1.0.0 spec)
# ────────────────────────────────────────────────────────────────

NUM_QUADRANTS = 4
CHANNELS_PER_QUADRANT = 16

QUADRANT_NAMES = {
    0: "Quadrant 0 (CB)",
    1: "Quadrant 1 (CF)",
    2: "Quadrant 2 (CD)",
    3: "Quadrant 3 (CE)",
}

QUADRANT_CODES = {0: "CB", 1: "CF", 2: "CD", 3: "CE"}

# QTRM groupings (from APCU display)
QTRM_GROUPS = [
    [5, 8, 15, 18, 25, 28],
    [33, 43, 53, 63, 73, 83],
    [3, 7, 13, 17, 23, 27],
    [37, 47, 57, 67, 77, 87],
    [39, 49, 59, 69, 79, 89],
    [4, 9, 14, 19, 24, 29],
    [34, 44, 54, 64, 74, 84],
    [0, 1, 10, 11, 20, 21],
    [31, 41, 51, 61, 71, 81],
    [30, 40, 50, 60, 70, 80],
    [2, 6, 12, 16, 22, 26],
    [36, 46, 56, 66, 76, 88],
]

AQC_CONTROLLERS = [0, 1, 2, 3]

# Nominal operating ranges
NOMINAL_ARRAY_VOLTAGE = 27.87       # V
NOMINAL_ARRAY_CURRENT = 22.35       # A
NOMINAL_INPUT_VOLTAGE = 27.85       # V
NOMINAL_INPUT_CURRENT = 5.33        # A
NOMINAL_CHANNEL_VOLTAGE_MIN = 27.85 # V
NOMINAL_CHANNEL_VOLTAGE_MAX = 28.15 # V
NOMINAL_CHANNEL_CURRENT_MIN = 0.06  # A
NOMINAL_CHANNEL_CURRENT_MAX = 0.70  # A
NOMINAL_TEMP_A = 41.79              # Deg C
NOMINAL_TEMP_B = 49.10              # Deg C
NOMINAL_TEMP_C = 42.78              # Deg C


class ChannelState:
    """Represents a single channel within a quadrant."""

    def __init__(self, ch_no, quadrant_idx):
        self.ch_no = ch_no
        self.quadrant_idx = quadrant_idx
        self.on_off = True                     # True = On
        self.trip_status = "Normal"            # "Normal" or "Tripped"
        self.bit_status = "Normal"             # "Normal" or "Fault"
        self.voltage = random.uniform(
            NOMINAL_CHANNEL_VOLTAGE_MIN, NOMINAL_CHANNEL_VOLTAGE_MAX
        )
        self.current = random.uniform(
            NOMINAL_CHANNEL_CURRENT_MIN, NOMINAL_CHANNEL_CURRENT_MAX
        )

    def simulate_step(self, degradation_factor=0.0):
        """Advance one simulation step."""
        # Voltage drift
        self.voltage += random.uniform(-0.15, 0.10)
        self.voltage = max(26.0, min(29.0, self.voltage))

        # Current drift
        self.current += random.uniform(-0.05, 0.05)
        self.current = max(0.0, min(1.0, self.current))

        # Trip / fault events (rare)
        trip_chance = 0.002 + degradation_factor * 0.02
        if random.random() < trip_chance and self.trip_status == "Normal":
            self.trip_status = "Tripped"
            self.on_off = False

        fault_chance = 0.001 + degradation_factor * 0.015
        if random.random() < fault_chance and self.bit_status == "Normal":
            self.bit_status = "Fault"

        # Recovery chance
        if self.trip_status == "Tripped" and random.random() < 0.05:
            self.trip_status = "Normal"
            self.on_off = True

        if self.bit_status == "Fault" and random.random() < 0.03:
            self.bit_status = "Normal"

    def health_score(self):
        """0-100 health for this channel."""
        score = 100.0
        if not self.on_off:
            score -= 50
        if self.trip_status == "Tripped":
            score -= 30
        if self.bit_status == "Fault":
            score -= 20
        # Voltage out of nominal range
        if self.voltage < 27.5 or self.voltage > 28.5:
            score -= 10
        return max(0.0, score)

    def to_dict(self):
        return {
            "ch": self.ch_no,
            "on": self.on_off,
            "trip": self.trip_status,
            "bit": self.bit_status,
            "voltage": round(self.voltage, 2),
            "current": round(self.current, 2),
        }


class QuadrantState:
    """Represents one of the 4 antenna quadrants."""

    def __init__(self, idx):
        self.idx = idx
        self.name = QUADRANT_NAMES[idx]
        self.code = QUADRANT_CODES[idx]
        self.channels = [ChannelState(ch, idx) for ch in range(CHANNELS_PER_QUADRANT)]

    def simulate_step(self, degradation_factor=0.0):
        for ch in self.channels:
            ch.simulate_step(degradation_factor)

    def health_score(self):
        if not self.channels:
            return 0.0
        return sum(ch.health_score() for ch in self.channels) / len(self.channels)

    def active_channels(self):
        return sum(1 for ch in self.channels if ch.on_off)

    def tripped_channels(self):
        return sum(1 for ch in self.channels if ch.trip_status == "Tripped")

    def faulted_channels(self):
        return sum(1 for ch in self.channels if ch.bit_status == "Fault")

    def to_dict(self):
        return {
            "name": self.name,
            "code": self.code,
            "health": round(self.health_score(), 1),
            "active": self.active_channels(),
            "tripped": self.tripped_channels(),
            "faulted": self.faulted_channels(),
            "channels": [ch.to_dict() for ch in self.channels],
        }


class APCUSimulator:
    """
    Full APCU (Antenna Power Control Unit) simulator.
    Models all subsystems shown in the APCU 4 Left display.
    """

    def __init__(self, component_id, host='localhost', port=12345, mode='nominal'):
        self.component_id = component_id
        self.host = host
        self.port = port
        self.mode = mode  # 'nominal', 'degraded', 'critical'
        self.socket = None
        self.connected = False

        # APCU Controller state
        self.link_status = [True, True, True, True]  # Q0, Q1, Q2, Q3
        self.sequence_on = True
        self.array_voltage = NOMINAL_ARRAY_VOLTAGE
        self.array_current = NOMINAL_ARRAY_CURRENT

        # Temperature Monitoring
        self.temp_sensor_a = NOMINAL_TEMP_A
        self.temp_sensor_b = NOMINAL_TEMP_B
        self.temp_sensor_c = NOMINAL_TEMP_C
        self.temp_status = "Normal"

        # Board Data
        self.input_voltage = NOMINAL_INPUT_VOLTAGE
        self.input_current = NOMINAL_INPUT_CURRENT

        # Board Status
        self.crc_user_flash = "Pass"
        self.crc_factory_flash = "Pass"
        self.crc_program_flash = "Pass"
        self.cbit = "Pass"
        self.pbit = "Pass"
        self.board_setting_check = "Pass"

        # Mode Status
        self.operating_mode = "Operating"
        self.loss_of_input_voltage = "No Loss"
        self.reset_source = "Software Reset"

        # Quadrants
        self.quadrants = [QuadrantState(i) for i in range(NUM_QUADRANTS)]

        # QTRMs
        self.qtrm_groups = QTRM_GROUPS
        self.qtrm_health = 100.0

        # AQC
        self.aqc_controllers = AQC_CONTROLLERS
        self.aqc_health = 100.0

        # Degradation factor (0.0 = nominal, 1.0 = fully degraded)
        if mode == 'degraded':
            self.degradation = 0.3
        elif mode == 'critical':
            self.degradation = 0.7
        else:
            self.degradation = 0.0

        # Health color mapping
        self.health_colors = {
            'operational': '#00FF00',
            'warning':     '#FFFF00',
            'degraded':    '#FFA500',
            'critical':    '#FF0000',
            'offline':     '#808080',
        }

    def connect(self):
        """Connect to the UnifiedApp MessageServer."""
        try:
            self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.socket.settimeout(10)
            self.socket.connect((self.host, self.port))
            self.connected = True
            print(f"[APCU {self.component_id}] Connected to {self.host}:{self.port}")
            return True
        except Exception as e:
            print(f"[APCU {self.component_id}] Connection failed: {e}")
            self.connected = False
            return False

    def disconnect(self):
        if self.socket:
            try:
                self.socket.close()
            except Exception:
                pass
            self.connected = False
            print(f"[APCU {self.component_id}] Disconnected")

    # ────────────────────────────────────────────────────────
    #  Simulation Step
    # ────────────────────────────────────────────────────────

    def simulate_step(self):
        """Advance one simulation tick for all APCU subsystems."""
        # Natural degradation drift
        if random.random() < 0.05:
            self.degradation = max(0.0, min(1.0,
                self.degradation + random.uniform(-0.05, 0.03)))

        # Temperature fluctuations
        self.temp_sensor_a += random.uniform(-0.8, 0.6)
        self.temp_sensor_b += random.uniform(-0.8, 0.6)
        self.temp_sensor_c += random.uniform(-0.8, 0.6)
        self.temp_sensor_a = max(30.0, min(70.0, self.temp_sensor_a))
        self.temp_sensor_b = max(30.0, min(70.0, self.temp_sensor_b))
        self.temp_sensor_c = max(30.0, min(70.0, self.temp_sensor_c))

        # Temperature status
        max_temp = max(self.temp_sensor_a, self.temp_sensor_b, self.temp_sensor_c)
        if max_temp > 60:
            self.temp_status = "Warning"
        elif max_temp > 65:
            self.temp_status = "Critical"
        else:
            self.temp_status = "Normal"

        # Board Data fluctuations
        self.input_voltage += random.uniform(-0.15, 0.10)
        self.input_voltage = max(25.0, min(30.0, self.input_voltage))
        self.input_current += random.uniform(-0.10, 0.08)
        self.input_current = max(3.0, min(8.0, self.input_current))

        # Board status faults (rare)
        if random.random() < 0.003 * (1 + self.degradation):
            fault = random.choice(['crc_user_flash', 'crc_factory_flash',
                                   'crc_program_flash', 'cbit', 'pbit',
                                   'board_setting_check'])
            setattr(self, fault, "Fail")
        # Recovery
        for attr in ['crc_user_flash', 'crc_factory_flash', 'crc_program_flash',
                     'cbit', 'pbit', 'board_setting_check']:
            if getattr(self, attr) == "Fail" and random.random() < 0.1:
                setattr(self, attr, "Pass")

        # Link status
        for i in range(4):
            if random.random() < 0.005 * (1 + self.degradation):
                self.link_status[i] = not self.link_status[i]

        # Array voltage/current
        self.array_voltage += random.uniform(-0.20, 0.15)
        self.array_voltage = max(25.0, min(30.0, self.array_voltage))
        self.array_current += random.uniform(-0.30, 0.25)
        self.array_current = max(15.0, min(30.0, self.array_current))

        # Quadrant simulation
        for q in self.quadrants:
            q.simulate_step(self.degradation)

        # QTRMs health
        total_channels_healthy = sum(
            ch.health_score()
            for q in self.quadrants for ch in q.channels
        )
        max_possible = NUM_QUADRANTS * CHANNELS_PER_QUADRANT * 100.0
        self.qtrm_health = (total_channels_healthy / max_possible) * 100.0

        # AQC health (based on link status and quadrant health)
        links_up = sum(1 for l in self.link_status if l)
        quad_avg = sum(q.health_score() for q in self.quadrants) / NUM_QUADRANTS
        self.aqc_health = (links_up / 4.0 * 50) + (quad_avg / 100.0 * 50)

        # Operating mode
        if self.degradation > 0.5:
            self.loss_of_input_voltage = "Low Voltage"
            self.operating_mode = "Degraded"
        elif self.degradation > 0.3:
            self.operating_mode = "Warning"
            self.loss_of_input_voltage = "No Loss"
        else:
            self.operating_mode = "Operating"
            self.loss_of_input_voltage = "No Loss"

    # ────────────────────────────────────────────────────────
    #  Health Computation
    # ────────────────────────────────────────────────────────

    def overall_health(self):
        """Compute overall APCU health 0-100."""
        scores = []

        # APCU Controller (link status, sequence)
        link_health = sum(1 for l in self.link_status if l) / 4.0 * 100
        scores.append(('apcu', link_health, 0.10))

        # Temperature
        temp_health = 100.0
        for t in [self.temp_sensor_a, self.temp_sensor_b, self.temp_sensor_c]:
            if t > 60:
                temp_health -= 20
            elif t > 55:
                temp_health -= 10
        scores.append(('temp', max(0, temp_health), 0.05))

        # Board Data
        bd_health = 100.0
        if self.input_voltage < 26.5 or self.input_voltage > 29.5:
            bd_health -= 30
        scores.append(('board_data', bd_health, 0.05))

        # Board Status
        board_checks = [self.crc_user_flash, self.crc_factory_flash,
                        self.crc_program_flash, self.cbit, self.pbit,
                        self.board_setting_check]
        pass_count = sum(1 for c in board_checks if c == "Pass")
        bs_health = (pass_count / len(board_checks)) * 100
        scores.append(('board_status', bs_health, 0.10))

        # Quadrants (major contributor)
        for i, q in enumerate(self.quadrants):
            scores.append((f'q{i}', q.health_score(), 0.15))

        # QTRMs
        scores.append(('qtrm', self.qtrm_health, 0.05))

        # AQC
        scores.append(('aqc', self.aqc_health, 0.05))

        # Weighted average
        total_weight = sum(w for _, _, w in scores)
        overall = sum(h * w for _, h, w in scores) / total_weight
        return max(0.0, min(100.0, overall))

    def health_status(self, health):
        if health >= 90:
            return 'operational'
        elif health >= 70:
            return 'warning'
        elif health >= 40:
            return 'degraded'
        elif health >= 10:
            return 'critical'
        else:
            return 'offline'

    # ────────────────────────────────────────────────────────
    #  Subsystem Health Map
    # ────────────────────────────────────────────────────────

    def subsystem_health_map(self):
        """Return per-subsystem health for the Antenna component."""
        link_h = sum(1 for l in self.link_status if l) / 4.0 * 100

        temp_h = 100.0
        for t in [self.temp_sensor_a, self.temp_sensor_b, self.temp_sensor_c]:
            if t > 60:
                temp_h -= 20
            elif t > 55:
                temp_h -= 10
        temp_h = max(0, temp_h)

        bd_h = 100.0
        if self.input_voltage < 26.5 or self.input_voltage > 29.5:
            bd_h -= 30
        bd_h = max(0, bd_h)

        board_checks = [self.crc_user_flash, self.crc_factory_flash,
                        self.crc_program_flash, self.cbit, self.pbit,
                        self.board_setting_check]
        pass_count = sum(1 for c in board_checks if c == "Pass")
        bs_h = (pass_count / len(board_checks)) * 100

        return {
            "APCU Controller": round(link_h, 1),
            "Temperature Monitoring": round(temp_h, 1),
            "Board Data": round(bd_h, 1),
            "Board Status": round(bs_h, 1),
            "Quadrant 0 (CB)": round(self.quadrants[0].health_score(), 1),
            "Quadrant 1 (CF)": round(self.quadrants[1].health_score(), 1),
            "Quadrant 2 (CD)": round(self.quadrants[2].health_score(), 1),
            "Quadrant 3 (CE)": round(self.quadrants[3].health_score(), 1),
            "QTRMs": round(self.qtrm_health, 1),
            "AQC": round(self.aqc_health, 1),
        }

    # ────────────────────────────────────────────────────────
    #  Full Telemetry Payload
    # ────────────────────────────────────────────────────────

    def full_telemetry(self):
        """Build the complete APCU telemetry payload."""
        health = self.overall_health()
        status = self.health_status(health)
        color = self.health_colors[status]

        return {
            "component_id": self.component_id,
            "color": color,
            "size": round(health, 1),
            "subsystem_health": self.subsystem_health_map(),
            "apcu_telemetry": {
                "version": "1.0.0",
                "unit": "APCU 4 Left",
                "link_status": {
                    f"Q{i}": self.link_status[i] for i in range(4)
                },
                "sequence_on": self.sequence_on,
                "array_voltage": round(self.array_voltage, 2),
                "array_current": round(self.array_current, 2),
                "temperature": {
                    "sensor_a": round(self.temp_sensor_a, 2),
                    "sensor_b": round(self.temp_sensor_b, 2),
                    "sensor_c": round(self.temp_sensor_c, 2),
                    "status": self.temp_status,
                },
                "board_data": {
                    "input_voltage": round(self.input_voltage, 2),
                    "current": round(self.input_current, 2),
                },
                "mode_status": {
                    "operating": self.operating_mode,
                    "loss_of_input_voltage": self.loss_of_input_voltage,
                    "reset_source": self.reset_source,
                },
                "board_status": {
                    "crc_user_flash": self.crc_user_flash,
                    "crc_factory_flash": self.crc_factory_flash,
                    "crc_program_flash": self.crc_program_flash,
                    "cbit": self.cbit,
                    "pbit": self.pbit,
                    "board_setting_check": self.board_setting_check,
                },
                "quadrants": [q.to_dict() for q in self.quadrants],
                "qtrm_groups": self.qtrm_groups,
                "aqc_controllers": self.aqc_controllers,
            },
        }

    # ────────────────────────────────────────────────────────
    #  Send Messages
    # ────────────────────────────────────────────────────────

    def send_health_update(self):
        """Send a component-level health update (basic protocol)."""
        health = self.overall_health()
        status = self.health_status(health)
        color = self.health_colors[status]

        message = {
            "component_id": self.component_id,
            "color": color,
            "size": round(health, 1),
        }
        return self._send_json(message, "Health")

    def send_subsystem_update(self):
        """Send per-subsystem health updates."""
        sub_map = self.subsystem_health_map()
        for sub_name, sub_health in sub_map.items():
            status = self.health_status(sub_health)
            color = self.health_colors[status]
            message = {
                "component_id": self.component_id,
                "subsystem": sub_name,
                "color": color,
                "size": round(sub_health, 1),
            }
            if not self._send_json(message, f"Subsystem[{sub_name}]"):
                return False
        return True

    def send_full_telemetry(self):
        """Send the complete APCU telemetry payload."""
        payload = self.full_telemetry()
        return self._send_json(payload, "Full Telemetry")

    def _send_json(self, obj, label=""):
        if not self.connected:
            return False
        try:
            data = json.dumps(obj) + '\n'
            self.socket.sendall(data.encode('utf-8'))
            return True
        except Exception as e:
            print(f"[APCU {self.component_id}] Send failed ({label}): {e}")
            self.connected = False
            return False

    # ────────────────────────────────────────────────────────
    #  Console Display
    # ────────────────────────────────────────────────────────

    def print_status(self):
        """Print a console summary similar to the APCU display."""
        health = self.overall_health()
        status = self.health_status(health)

        print(f"\n{'='*72}")
        print(f"  APCU 4 Left [V1.0.0]   Component: {self.component_id}")
        print(f"{'='*72}")
        print(f"  Array Voltage: {self.array_voltage:6.2f} V   "
              f"Array Current: {self.array_current:5.2f} A   "
              f"Overall Health: {health:5.1f}% [{status.upper()}]")
        print(f"  Links: Q0={'UP' if self.link_status[0] else 'DN'}  "
              f"Q1={'UP' if self.link_status[1] else 'DN'}  "
              f"Q2={'UP' if self.link_status[2] else 'DN'}  "
              f"Q3={'UP' if self.link_status[3] else 'DN'}  "
              f"Seq: {'ON' if self.sequence_on else 'OFF'}")

        print(f"\n  Temperature:  A={self.temp_sensor_a:.1f}°C  "
              f"B={self.temp_sensor_b:.1f}°C  "
              f"C={self.temp_sensor_c:.1f}°C  [{self.temp_status}]")
        print(f"  Board Data:   Vin={self.input_voltage:.2f}V  "
              f"Iin={self.input_current:.2f}A")
        print(f"  Board Status: CRC_UF={self.crc_user_flash}  "
              f"CRC_FF={self.crc_factory_flash}  "
              f"CRC_PF={self.crc_program_flash}  "
              f"CBIT={self.cbit}  PBIT={self.pbit}  "
              f"BSC={self.board_setting_check}")
        print(f"  Mode: {self.operating_mode}  "
              f"InputV: {self.loss_of_input_voltage}  "
              f"Reset: {self.reset_source}")

        for q in self.quadrants:
            active = q.active_channels()
            tripped = q.tripped_channels()
            faulted = q.faulted_channels()
            print(f"\n  {q.name}  "
                  f"Active: {active}/{CHANNELS_PER_QUADRANT}  "
                  f"Tripped: {tripped}  Faulted: {faulted}  "
                  f"Health: {q.health_score():.1f}%")

        print(f"\n  QTRMs Health: {self.qtrm_health:.1f}%  "
              f"AQC Health: {self.aqc_health:.1f}%")
        print(f"  AQC Controllers: {', '.join(str(c) for c in self.aqc_controllers)}")
        print(f"{'─'*72}")

    # ────────────────────────────────────────────────────────
    #  Main Run Loop
    # ────────────────────────────────────────────────────────

    def run(self, interval=2.0, verbose=True, send_telemetry=True):
        """Main loop: simulate, send updates, repeat."""
        print(f"\n{'='*60}")
        print(f"  APCU ANTENNA SYSTEM SIMULATOR")
        print(f"{'='*60}")
        print(f"  Component ID : {self.component_id}")
        print(f"  Target       : {self.host}:{self.port}")
        print(f"  Mode         : {self.mode}")
        print(f"  Interval     : {interval}s")
        print(f"  Telemetry    : {'Full' if send_telemetry else 'Basic'}")
        print(f"{'='*60}\n")

        if not self.connect():
            print("  Failed to connect. Retrying in 5 seconds...")
            time.sleep(5)
            if not self.connect():
                print("  Could not connect. Exiting.")
                return

        try:
            tick = 0
            while True:
                self.simulate_step()

                # Always send component-level health
                if not self.send_health_update():
                    print(f"  Reconnecting...")
                    time.sleep(2)
                    if not self.connect():
                        break
                    continue

                # Send subsystem-level health
                self.send_subsystem_update()

                # Send full telemetry every 5th tick
                if send_telemetry and tick % 5 == 0:
                    self.send_full_telemetry()

                if verbose:
                    self.print_status()

                tick += 1
                time.sleep(interval)

        except KeyboardInterrupt:
            print(f"\n  Shutting down APCU simulator...")
        finally:
            self.disconnect()


# ────────────────────────────────────────────────────────────────
#  CLI Entry Point
# ────────────────────────────────────────────────────────────────

def main():
    parser = argparse.ArgumentParser(
        description='APCU Antenna System Simulator',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  # Simulate antenna with default settings
  python3 apcu_simulator.py component_1

  # Simulate with degraded conditions
  python3 apcu_simulator.py antenna_1 --mode degraded

  # Fast updates, minimal output
  python3 apcu_simulator.py component_1 --interval 0.5 --quiet

  # Full telemetry mode
  python3 apcu_simulator.py antenna_1 --telemetry
        """
    )
    parser.add_argument('component_id',
                        help='Component ID (must match canvas ID, e.g. component_1)')
    parser.add_argument('--host', default='localhost',
                        help='Server host (default: localhost)')
    parser.add_argument('--port', type=int, default=12345,
                        help='Server port (default: 12345)')
    parser.add_argument('--interval', type=float, default=2.0,
                        help='Update interval in seconds (default: 2.0)')
    parser.add_argument('--mode', choices=['nominal', 'degraded', 'critical'],
                        default='nominal',
                        help='Simulation mode (default: nominal)')
    parser.add_argument('--quiet', action='store_true',
                        help='Suppress verbose console output')
    parser.add_argument('--telemetry', action='store_true', default=True,
                        help='Send full APCU telemetry (default: enabled)')
    parser.add_argument('--no-telemetry', action='store_true',
                        help='Disable full telemetry, send basic health only')

    args = parser.parse_args()

    send_telem = not args.no_telemetry

    sim = APCUSimulator(args.component_id, args.host, args.port, args.mode)
    sim.run(interval=args.interval, verbose=not args.quiet, send_telemetry=send_telem)


if __name__ == '__main__':
    main()
