#!/usr/bin/env python3
"""
Antenna Subsystem Simulator (Multi-Protocol, N-TRM)
=====================================================

Simulates a full phased-array antenna subsystem consisting of N
Transmitter-Receiver Modules (TRMs) arranged in quadrants.

Each TRM is independently modelled with:
  - On/Off state
  - Trip status (Normal / Tripped)
  - BIT status (Normal / Fault)
  - Voltage and current readings
  - Derived health score (0-100)

Communication protocols supported:
  --protocol tcp      Line-delimited JSON over TCP (default, port 12345)
  --protocol udp      JSON datagrams over UDP (default port 12346)
  --protocol rs422    Simulated RS422 via TCP (for testing without hardware)
  --protocol rs232    Simulated RS232 via TCP (for testing without hardware)
  --protocol modbus   Simulated Modbus holding-register layout via TCP
  --protocol can      Simulated CAN frame stream via TCP wrapper

Message formats sent to UnifiedApp MessageServer:
  1. Component-level health:  { component_id, color, size }
  2. Subsystem health map:    { ..., subsystem_health: {...} }
  3. TRM grid data:           { ..., trm_data: [ {id, health, color, ...} ] }
  4. Full APCU telemetry:     { ..., apcu_telemetry: {...} }

Usage:
  python3 antenna_subsystem_simulator.py <component_id> [options]

Examples:
  python3 antenna_subsystem_simulator.py component_1
  python3 antenna_subsystem_simulator.py ant_left  --trm-count 271 --cols 16
  python3 antenna_subsystem_simulator.py ant_right --protocol udp --port 12346
  python3 antenna_subsystem_simulator.py ant_rear  --mode degraded --interval 1.0
"""

import socket
import json
import time
import random
import argparse
import sys
import math
import struct
import threading

# ──────────────────────────────────────────────────────────────────────────────
#  Configuration constants
# ──────────────────────────────────────────────────────────────────────────────

NUM_QUADRANTS = 4
DEFAULT_TRM_COUNT = 271
DEFAULT_TRM_COLUMNS = 16

QUADRANT_NAMES = {
    0: "Quadrant 0 (CB)",
    1: "Quadrant 1 (CF)",
    2: "Quadrant 2 (CD)",
    3: "Quadrant 3 (CE)",
}

NOMINAL_CHANNEL_VOLTAGE_MIN = 27.85
NOMINAL_CHANNEL_VOLTAGE_MAX = 28.15
NOMINAL_CHANNEL_CURRENT_MIN = 0.06
NOMINAL_CHANNEL_CURRENT_MAX = 0.70
NOMINAL_ARRAY_VOLTAGE = 27.87
NOMINAL_ARRAY_CURRENT = 22.35
NOMINAL_INPUT_VOLTAGE = 27.85
NOMINAL_INPUT_CURRENT = 5.33
NOMINAL_TEMP_A = 41.79
NOMINAL_TEMP_B = 49.10
NOMINAL_TEMP_C = 42.78

HEALTH_COLORS = {
    'operational': '#00FF00',
    'warning':     '#FFFF00',
    'degraded':    '#FFA500',
    'critical':    '#FF0000',
    'offline':     '#808080',
}


# ──────────────────────────────────────────────────────────────────────────────
#  TRM model
# ──────────────────────────────────────────────────────────────────────────────

class TRM:
    """Transmitter-Receiver Module – a single element in the phased array."""

    def __init__(self, trm_id: int, quadrant: int, channel: int, degradation: float = 0.0):
        self.id = trm_id
        self.quadrant = quadrant
        self.channel = channel
        self.on = True
        self.trip = "Normal"
        self.bit = "Normal"
        self.voltage = random.uniform(NOMINAL_CHANNEL_VOLTAGE_MIN, NOMINAL_CHANNEL_VOLTAGE_MAX)
        self.current = random.uniform(NOMINAL_CHANNEL_CURRENT_MIN, NOMINAL_CHANNEL_CURRENT_MAX)
        self._degradation = degradation

    def step(self):
        """Advance one simulation tick."""
        d = self._degradation

        # Voltage / current drift
        self.voltage += random.uniform(-0.15, 0.10)
        self.voltage = max(26.0, min(29.0, self.voltage))
        self.current += random.uniform(-0.05, 0.05)
        self.current = max(0.0, min(1.0, self.current))

        # Trip events
        if random.random() < 0.002 + d * 0.02 and self.trip == "Normal":
            self.trip = "Tripped"
            self.on = False
        if self.trip == "Tripped" and random.random() < 0.05:
            self.trip = "Normal"
            self.on = True

        # BIT fault events
        if random.random() < 0.001 + d * 0.015 and self.bit == "Normal":
            self.bit = "Fault"
        if self.bit == "Fault" and random.random() < 0.03:
            self.bit = "Normal"

    def health(self) -> float:
        score = 100.0
        if not self.on:        score -= 50
        if self.trip != "Normal": score -= 30
        if self.bit != "Normal":  score -= 20
        if self.voltage < 27.5 or self.voltage > 28.5:
            score -= 10
        return max(0.0, score)

    def status_str(self, h: float) -> str:
        if h >= 90: return "HEALTHY"
        if h >= 70: return "WARNING"
        if h >= 40: return "DEGRADED"
        if h >= 10: return "CRITICAL"
        return "OFFLINE"

    def color(self) -> str:
        h = self.health()
        if h >= 90: return HEALTH_COLORS['operational']
        if h >= 70: return HEALTH_COLORS['warning']
        if h >= 40: return HEALTH_COLORS['degraded']
        if h >= 10: return HEALTH_COLORS['critical']
        return HEALTH_COLORS['offline']

    def to_dict(self) -> dict:
        h = self.health()
        return {
            "id":       self.id,
            "quadrant": self.quadrant,
            "channel":  self.channel,
            "health":   round(h, 1),
            "color":    self.color(),
            "status":   self.status_str(h),
            "on":       self.on,
            "trip":     self.trip,
            "bit":      self.bit,
            "voltage":  round(self.voltage, 2),
            "current":  round(self.current, 3),
        }


# ──────────────────────────────────────────────────────────────────────────────
#  AntennaSubsystem – top-level model
# ──────────────────────────────────────────────────────────────────────────────

class AntennaSubsystem:
    """
    Phased-array antenna with N TRMs distributed across 4 quadrants.
    Replicates the APCU 4 Left telemetry plus per-TRM data.
    """

    def __init__(self, component_id: str, trm_count: int = DEFAULT_TRM_COUNT,
                 mode: str = 'nominal'):
        self.component_id = component_id
        self.trm_count = trm_count
        self.mode = mode

        if mode == 'degraded':
            self.degradation = 0.3
        elif mode == 'critical':
            self.degradation = 0.7
        else:
            self.degradation = 0.0

        # Distribute TRMs across 4 quadrants
        per_q = trm_count // NUM_QUADRANTS
        extras = trm_count % NUM_QUADRANTS
        self.trms: list[TRM] = []
        trm_id = 0
        for q in range(NUM_QUADRANTS):
            count = per_q + (1 if q < extras else 0)
            for ch in range(count):
                self.trms.append(TRM(trm_id, q, ch, self.degradation))
                trm_id += 1

        # APCU controller state
        self.link_status = [True, True, True, True]
        self.sequence_on = True
        self.array_voltage = NOMINAL_ARRAY_VOLTAGE
        self.array_current = NOMINAL_ARRAY_CURRENT

        # Temperature monitoring
        self.temp_a = NOMINAL_TEMP_A
        self.temp_b = NOMINAL_TEMP_B
        self.temp_c = NOMINAL_TEMP_C
        self.temp_status = "Normal"

        # Board data / status
        self.input_voltage = NOMINAL_INPUT_VOLTAGE
        self.input_current = NOMINAL_INPUT_CURRENT
        self.crc_user_flash = "Pass"
        self.crc_factory_flash = "Pass"
        self.crc_program_flash = "Pass"
        self.cbit = "Pass"
        self.pbit = "Pass"
        self.board_setting_check = "Pass"
        self.operating_mode = "Operating"
        self.loss_of_input_voltage = "No Loss"
        self.reset_source = "Software Reset"

    def step(self):
        """Advance one simulation tick."""
        d = self.degradation

        # Degradation drift
        if random.random() < 0.05:
            self.degradation = max(0.0, min(1.0,
                self.degradation + random.uniform(-0.05, 0.03)))

        # Step all TRMs
        for trm in self.trms:
            trm._degradation = self.degradation
            trm.step()

        # Temperature
        self.temp_a = max(30.0, min(70.0, self.temp_a + random.uniform(-0.8, 0.6)))
        self.temp_b = max(30.0, min(70.0, self.temp_b + random.uniform(-0.8, 0.6)))
        self.temp_c = max(30.0, min(70.0, self.temp_c + random.uniform(-0.8, 0.6)))
        max_temp = max(self.temp_a, self.temp_b, self.temp_c)
        if max_temp > 65:   self.temp_status = "Critical"
        elif max_temp > 60: self.temp_status = "Warning"
        else:                self.temp_status = "Normal"

        # Board data
        self.input_voltage = max(25.0, min(30.0,
            self.input_voltage + random.uniform(-0.15, 0.10)))
        self.input_current = max(3.0, min(8.0,
            self.input_current + random.uniform(-0.10, 0.08)))

        # Board status faults (rare)
        attrs = ['crc_user_flash', 'crc_factory_flash', 'crc_program_flash',
                 'cbit', 'pbit', 'board_setting_check']
        if random.random() < 0.003 * (1 + d):
            setattr(self, random.choice(attrs), "Fail")
        for attr in attrs:
            if getattr(self, attr) == "Fail" and random.random() < 0.1:
                setattr(self, attr, "Pass")

        # Link status
        for i in range(4):
            if random.random() < 0.005 * (1 + d):
                self.link_status[i] = not self.link_status[i]

        # Array voltage/current
        self.array_voltage = max(25.0, min(30.0,
            self.array_voltage + random.uniform(-0.20, 0.15)))
        self.array_current = max(15.0, min(30.0,
            self.array_current + random.uniform(-0.30, 0.25)))

        # Operating mode
        if self.degradation > 0.5:
            self.operating_mode = "Degraded"
            self.loss_of_input_voltage = "Low Voltage"
        elif self.degradation > 0.3:
            self.operating_mode = "Warning"
            self.loss_of_input_voltage = "No Loss"
        else:
            self.operating_mode = "Operating"
            self.loss_of_input_voltage = "No Loss"

    # ── Health computation ────────────────────────────────────────────────────

    def overall_health(self) -> float:
        trm_avg = sum(t.health() for t in self.trms) / len(self.trms) if self.trms else 0
        link_h = sum(1 for l in self.link_status if l) / 4.0 * 100
        temp_h = max(0, 100.0 - max(0, (max(self.temp_a, self.temp_b, self.temp_c) - 55)) * 4)
        board_checks = [self.crc_user_flash, self.crc_factory_flash,
                        self.crc_program_flash, self.cbit, self.pbit,
                        self.board_setting_check]
        bs_h = sum(1 for c in board_checks if c == "Pass") / len(board_checks) * 100

        weights = [(trm_avg, 0.60), (link_h, 0.10), (temp_h, 0.10), (bs_h, 0.20)]
        return max(0.0, min(100.0, sum(h * w for h, w in weights)))

    def health_status(self, h: float) -> str:
        if h >= 90: return 'operational'
        if h >= 70: return 'warning'
        if h >= 40: return 'degraded'
        if h >= 10: return 'critical'
        return 'offline'

    def quadrant_health(self, q_idx: int) -> float:
        trms = [t for t in self.trms if t.quadrant == q_idx]
        if not trms: return 0.0
        return sum(t.health() for t in trms) / len(trms)

    def subsystem_health_map(self) -> dict:
        link_h = sum(1 for l in self.link_status if l) / 4.0 * 100
        temp_h = max(0, 100.0 - max(0, (max(self.temp_a, self.temp_b, self.temp_c) - 55)) * 4)
        bd_h = 100.0 if 26.5 <= self.input_voltage <= 29.5 else 70.0
        board_checks = [self.crc_user_flash, self.crc_factory_flash,
                        self.crc_program_flash, self.cbit, self.pbit,
                        self.board_setting_check]
        bs_h = sum(1 for c in board_checks if c == "Pass") / len(board_checks) * 100

        trm_avg = sum(t.health() for t in self.trms) / len(self.trms) if self.trms else 0
        aqc_links = sum(1 for l in self.link_status if l)
        aqc_h = (aqc_links / 4.0 * 50) + (self.quadrant_health(0) / 100.0 * 50)

        return {
            "APCU Controller":       round(link_h, 1),
            "Temperature Monitoring": round(temp_h, 1),
            "Board Data":            round(bd_h, 1),
            "Board Status":          round(bs_h, 1),
            "Quadrant 0 (CB)":       round(self.quadrant_health(0), 1),
            "Quadrant 1 (CF)":       round(self.quadrant_health(1), 1),
            "Quadrant 2 (CD)":       round(self.quadrant_health(2), 1),
            "Quadrant 3 (CE)":       round(self.quadrant_health(3), 1),
            "QTRMs":                 round(trm_avg, 1),
            "AQC":                   round(aqc_h, 1),
        }

    # ── Payload builders ─────────────────────────────────────────────────────

    def build_full_message(self) -> dict:
        """Build the complete JSON payload including TRM grid data."""
        health = self.overall_health()
        status = self.health_status(health)
        color = HEALTH_COLORS[status]

        return {
            "component_id":    self.component_id,
            "color":           color,
            "size":            round(health, 1),
            "subsystem_health": self.subsystem_health_map(),
            "trm_data":        [t.to_dict() for t in self.trms],
            "apcu_telemetry": {
                "version":         "2.0.0",
                "unit":            f"Antenna Subsystem ({self.trm_count} TRMs)",
                "trm_count":       self.trm_count,
                "link_status":     {f"Q{i}": self.link_status[i] for i in range(4)},
                "sequence_on":     self.sequence_on,
                "array_voltage":   round(self.array_voltage, 2),
                "array_current":   round(self.array_current, 2),
                "temperature": {
                    "sensor_a":    round(self.temp_a, 2),
                    "sensor_b":    round(self.temp_b, 2),
                    "sensor_c":    round(self.temp_c, 2),
                    "status":      self.temp_status,
                },
                "board_data": {
                    "input_voltage": round(self.input_voltage, 2),
                    "current":       round(self.input_current, 2),
                },
                "mode_status": {
                    "operating":             self.operating_mode,
                    "loss_of_input_voltage": self.loss_of_input_voltage,
                    "reset_source":          self.reset_source,
                },
                "board_status": {
                    "crc_user_flash":    self.crc_user_flash,
                    "crc_factory_flash": self.crc_factory_flash,
                    "crc_program_flash": self.crc_program_flash,
                    "cbit":              self.cbit,
                    "pbit":              self.pbit,
                    "board_setting_check": self.board_setting_check,
                },
            },
        }

    def build_health_message(self) -> dict:
        """Lightweight health + subsystem message (no TRM grid)."""
        health = self.overall_health()
        status = self.health_status(health)
        return {
            "component_id":    self.component_id,
            "color":           HEALTH_COLORS[status],
            "size":            round(health, 1),
            "subsystem_health": self.subsystem_health_map(),
        }

    def build_modbus_registers(self) -> list[int]:
        """
        Build a list of holding register values for Modbus simulation.
        Register layout:
          0   – TRM count (uint16)
          1   – overall health * 10 (uint16, range 0-1000)
          2   – status code (0=op, 1=warn, 2=deg, 3=crit, 4=off)
          3…N – per-subsystem health * 10 (uint16)
        """
        health = self.overall_health()
        status_map = {'operational': 0, 'warning': 1, 'degraded': 2,
                      'critical': 3, 'offline': 4}
        status_code = status_map.get(self.health_status(health), 4)
        regs = [self.trm_count, int(health * 10), status_code]
        for v in self.subsystem_health_map().values():
            regs.append(int(v * 10))
        return regs

    def print_status(self):
        health = self.overall_health()
        print(f"\n{'='*70}")
        print(f"  Antenna Subsystem – {self.trm_count} TRMs  [{self.component_id}]")
        print(f"{'='*70}")
        print(f"  Overall Health: {health:.1f}%  [{self.health_status(health).upper()}]")
        print(f"  Array V: {self.array_voltage:.2f}V  I: {self.array_current:.2f}A")
        print(f"  Temp: A={self.temp_a:.1f}°C  B={self.temp_b:.1f}°C  "
              f"C={self.temp_c:.1f}°C  [{self.temp_status}]")
        print(f"  Mode: {self.operating_mode}  Input: {self.loss_of_input_voltage}")

        for q in range(NUM_QUADRANTS):
            trms_q = [t for t in self.trms if t.quadrant == q]
            on_count = sum(1 for t in trms_q if t.on)
            tripped = sum(1 for t in trms_q if t.trip != "Normal")
            faulted = sum(1 for t in trms_q if t.bit != "Normal")
            qh = self.quadrant_health(q)
            print(f"  {QUADRANT_NAMES[q]}  TRMs={len(trms_q)}  "
                  f"On={on_count}  Tripped={tripped}  Faulted={faulted}  "
                  f"Health={qh:.1f}%")
        print(f"{'─'*70}")


# ──────────────────────────────────────────────────────────────────────────────
#  Protocol transport helpers
# ──────────────────────────────────────────────────────────────────────────────

def _send_tcp(sock, data: dict) -> bool:
    """Send a JSON dict over a TCP socket (line-delimited)."""
    try:
        sock.sendall((json.dumps(data) + '\n').encode('utf-8'))
        return True
    except Exception as e:
        print(f"  [TCP] Send error: {e}")
        return False


def _send_udp(sock, host: str, port: int, data: dict) -> bool:
    """Send a JSON dict as a UDP datagram."""
    try:
        payload = (json.dumps(data) + '\n').encode('utf-8')
        sock.sendto(payload, (host, port))
        return True
    except Exception as e:
        print(f"  [UDP] Send error: {e}")
        return False


def _send_modbus_tcp(sock, registers: list[int]) -> bool:
    """
    Send Modbus TCP Read Holding Registers Response (Function 0x03).
    This is a simulation wrapper so the MessageServer's Modbus handler
    can be tested without a real Modbus server.
    """
    try:
        # Pack as a Modbus TCP PDU response (simplified, for testing)
        # Transaction ID=1, Protocol ID=0, Length=3+2*N, Unit ID=1, FC=3
        n = len(registers)
        byte_count = n * 2
        length = 3 + byte_count
        header = struct.pack('>HHHBB', 1, 0, length, 1, 3) + bytes([byte_count])
        reg_bytes = struct.pack(f'>{n}H', *registers)
        sock.sendall(header + reg_bytes)
        return True
    except Exception as e:
        print(f"  [Modbus] Send error: {e}")
        return False


def _encode_can_frames(antenna: AntennaSubsystem) -> bytes:
    """
    Encode antenna health as a sequence of CAN frames (using a simple
    ASCII wrapper for TCP simulation: each line is 'CAN <id> <hex_data>\\n').
    """
    health = antenna.overall_health()
    status_map = {'operational': 0, 'warning': 1, 'degraded': 2, 'critical': 3, 'offline': 4}
    status_code = status_map.get(antenna.health_status(health), 4)

    frames = []
    # Frame 0x100: overall health byte + status byte
    frames.append(f"CAN 100 {int(health):02X}{status_code:02X}\n")

    # Frames 0x101+: subsystem health (index, value)
    for i, v in enumerate(antenna.subsystem_health_map().values()):
        frames.append(f"CAN {0x101 + i:03X} {i:02X}{int(v):02X}\n")

    return ''.join(frames).encode('utf-8')


# ──────────────────────────────────────────────────────────────────────────────
#  Runner class
# ──────────────────────────────────────────────────────────────────────────────

class AntennaSimulator:
    """
    Main simulation runner. Connects to the UnifiedApp MessageServer using the
    selected protocol and sends periodic health updates.
    """

    def __init__(self, component_id: str, host: str = 'localhost', port: int = 12345,
                 protocol: str = 'tcp', trm_count: int = DEFAULT_TRM_COUNT,
                 trm_columns: int = DEFAULT_TRM_COLUMNS, mode: str = 'nominal',
                 interval: float = 2.0, verbose: bool = True,
                 full_trm_interval: int = 5):
        self.component_id = component_id
        self.host = host
        self.port = port
        self.protocol = protocol.lower()
        self.interval = interval
        self.verbose = verbose
        self.full_trm_interval = full_trm_interval

        self.antenna = AntennaSubsystem(component_id, trm_count, mode)

        self.sock = None
        self.connected = False

    def connect(self) -> bool:
        try:
            if self.protocol == 'udp':
                self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
                self.connected = True
                print(f"  [UDP] Socket ready -> {self.host}:{self.port}")
                return True
            else:
                # TCP for all other protocols (RS422/RS232/Modbus/CAN use TCP wrapper)
                self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                self.sock.settimeout(10)
                self.sock.connect((self.host, self.port))
                self.connected = True
                print(f"  [{self.protocol.upper()}] Connected to {self.host}:{self.port}")
                return True
        except Exception as e:
            print(f"  Connection failed: {e}")
            self.connected = False
            return False

    def disconnect(self):
        if self.sock:
            try: self.sock.close()
            except Exception: pass
        self.connected = False

    def send(self, data: dict) -> bool:
        if not self.connected:
            return False

        proto = self.protocol
        if proto == 'udp':
            return _send_udp(self.sock, self.host, self.port, data)

        elif proto == 'modbus':
            # Send Modbus response followed by the full JSON so the receiver
            # can also parse it directly (compatibility with JSON-mode server)
            regs = self.antenna.build_modbus_registers()
            ok = _send_modbus_tcp(self.sock, regs)
            # Also send the JSON for the fallback JSON parser
            return _send_tcp(self.sock, data) and ok

        elif proto in ('can',):
            # Send CAN-framed data then full JSON for compatibility
            try:
                self.sock.sendall(_encode_can_frames(self.antenna))
            except Exception as e:
                print(f"  [CAN] Frame send error: {e}")
                return False
            return _send_tcp(self.sock, data)

        else:
            # TCP / RS422 / RS232 – all use line-delimited JSON
            return _send_tcp(self.sock, data)

    def run(self):
        print(f"\n{'='*60}")
        print(f"  ANTENNA SUBSYSTEM SIMULATOR")
        print(f"{'='*60}")
        print(f"  Component ID : {self.component_id}")
        print(f"  TRM Count    : {self.antenna.trm_count}")
        print(f"  Protocol     : {self.protocol.upper()}")
        print(f"  Target       : {self.host}:{self.port}")
        print(f"  Mode         : {self.antenna.mode}")
        print(f"  Interval     : {self.interval}s")
        print(f"  TRM full msg : every {self.full_trm_interval} ticks")
        print(f"{'='*60}\n")

        if not self.connect():
            print("  Retrying in 5 seconds...")
            time.sleep(5)
            if not self.connect():
                print("  Cannot connect. Exiting.")
                return

        try:
            tick = 0
            while True:
                self.antenna.step()

                # Decide which payload to send
                if tick % self.full_trm_interval == 0:
                    payload = self.antenna.build_full_message()
                    label = "FULL (TRM grid + telemetry)"
                else:
                    payload = self.antenna.build_health_message()
                    label = "HEALTH + SUBSYSTEMS"

                if not self.send(payload):
                    print("  Send failed – reconnecting...")
                    self.disconnect()
                    time.sleep(2)
                    if not self.connect():
                        print("  Reconnect failed. Exiting.")
                        break
                    continue

                if self.verbose:
                    health = self.antenna.overall_health()
                    status = self.antenna.health_status(health)
                    print(f"  [{tick:04d}] {label}  "
                          f"Health={health:.1f}%  [{status.upper()}]  "
                          f"Proto={self.protocol.upper()}")
                    if tick % 10 == 0:
                        self.antenna.print_status()

                tick += 1
                time.sleep(self.interval)

        except KeyboardInterrupt:
            print("\n  Shutting down...")
        finally:
            self.disconnect()


# ──────────────────────────────────────────────────────────────────────────────
#  CLI
# ──────────────────────────────────────────────────────────────────────────────

def main():
    parser = argparse.ArgumentParser(
        description='Antenna Subsystem Simulator (Multi-Protocol, N-TRM)',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  # Default TCP, 271 TRMs
  python3 antenna_subsystem_simulator.py component_1

  # UDP protocol
  python3 antenna_subsystem_simulator.py ant_left --protocol udp --port 12346

  # RS422 (simulated via TCP)
  python3 antenna_subsystem_simulator.py ant_rear --protocol rs422

  # Modbus TCP
  python3 antenna_subsystem_simulator.py ant_top --protocol modbus --port 502

  # Degraded mode, 512 TRMs
  python3 antenna_subsystem_simulator.py ant_full --trm-count 512 --mode degraded

  # Fast updates, quiet
  python3 antenna_subsystem_simulator.py component_1 --interval 0.5 --quiet
        """
    )
    parser.add_argument('component_id',
                        help='Component ID matching the canvas component (e.g. component_1)')
    parser.add_argument('--host', default='localhost',
                        help='Server host (default: localhost)')
    parser.add_argument('--port', type=int, default=None,
                        help='Server port (default: 12345 for TCP/RS422/RS232/CAN, '
                             '12346 for UDP, 502 for Modbus)')
    parser.add_argument('--protocol',
                        choices=['tcp', 'udp', 'rs422', 'rs232', 'modbus', 'can'],
                        default='tcp',
                        help='Communication protocol (default: tcp)')
    parser.add_argument('--trm-count', type=int, default=DEFAULT_TRM_COUNT,
                        help=f'Number of TRMs (default: {DEFAULT_TRM_COUNT})')
    parser.add_argument('--cols', type=int, default=DEFAULT_TRM_COLUMNS,
                        help=f'Columns in TRM grid (default: {DEFAULT_TRM_COLUMNS})')
    parser.add_argument('--mode',
                        choices=['nominal', 'degraded', 'critical'],
                        default='nominal',
                        help='Simulation mode (default: nominal)')
    parser.add_argument('--interval', type=float, default=2.0,
                        help='Update interval in seconds (default: 2.0)')
    parser.add_argument('--trm-full-interval', type=int, default=5,
                        help='Send full TRM grid every N ticks (default: 5)')
    parser.add_argument('--quiet', action='store_true',
                        help='Suppress verbose console output')

    args = parser.parse_args()

    # Default port per protocol
    if args.port is None:
        defaults = {'tcp': 12345, 'udp': 12346, 'rs422': 12345, 'rs232': 12345,
                    'modbus': 12345, 'can': 12345}
        args.port = defaults.get(args.protocol, 12345)

    sim = AntennaSimulator(
        component_id=args.component_id,
        host=args.host,
        port=args.port,
        protocol=args.protocol,
        trm_count=args.trm_count,
        trm_columns=args.cols,
        mode=args.mode,
        interval=args.interval,
        verbose=not args.quiet,
        full_trm_interval=args.trm_full_interval,
    )
    sim.run()


if __name__ == '__main__':
    main()
