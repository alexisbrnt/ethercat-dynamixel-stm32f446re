"""
Motor IHM + EtherCAT Bridge (fused, no ROS2)
=============================================
Single-program version: the PyQt5 GUI communicates directly
with the EtherCAT master via Qt signals, no middleware needed.
"""

import sys
import time
import threading

import pysoem

from PyQt5 import QtCore, QtWidgets
from PyQt5.QtCore import Qt
from PyQt5.QtWidgets import (
    QApplication, QMainWindow, QWidget, QLabel, QPushButton,
    QGridLayout, QGroupBox, QVBoxLayout, QHBoxLayout,
    QLCDNumber, QLineEdit, QComboBox, QCheckBox, QSlider
)


# ===================== EtherCAT Thread =====================
class EthercatThread(QtCore.QThread):
    """
    Runs the EtherCAT cycle in a dedicated thread.
    Communicates with the GUI via Qt signals only.
    """
    status_received = QtCore.pyqtSignal(dict)
    error_occurred = QtCore.pyqtSignal(str)
    connected = QtCore.pyqtSignal()

    def __init__(self, ifname: str = "enx207bd2b452c6"):
        super().__init__()
        self.ifname = ifname
        self.running = True

        self._command_lock = threading.Lock()
        self._latest_command = {
            "id": 1,
            "control_mode": 3,
            "torque_enabled": 0,
            "LED_state": 0,
            "target_position": 0,
            "target_velocity": 0,
        }

        self.master = None
        self.slave = None

    # ---------- command interface (called from GUI thread) ----------
    def update_command(self, cmd: dict):
        with self._command_lock:
            self._latest_command = cmd

    # ---------- EtherCAT init ----------
    def _init_ethercat(self):
        self.master = pysoem.Master()
        self.master.open(self.ifname)

        if self.master.config_init() <= 0:
            raise RuntimeError("No EtherCAT slave found")

        self.slave = self.master.slaves[0]
        print(f"[EC] Slave 0: name={self.slave.name}, "
              f"man=0x{self.slave.man:08x}, rev=0x{self.slave.rev:08x}")

        # Let the STM32 reach PRE-OP
        time.sleep(2)

        iomap_size = self.master.config_map()
        print(f"[EC] IOMap size = {iomap_size}")

        has_dc = self.master.config_dc()
        print(f"[EC] DC found = {has_dc}")

        self.master.state = pysoem.SAFEOP_STATE
        self.master.write_state()
        self.master.state_check(pysoem.SAFEOP_STATE, 500_000)

        self.master.send_processdata()
        self.master.receive_processdata(5000)

        self.master.state = pysoem.OP_STATE
        self.master.write_state()
        state = self.master.state_check(pysoem.OP_STATE, 500_000)

        if state != pysoem.OP_STATE:
            self.master.read_state()
            raise RuntimeError(
                f"OP transition failed, current state = {self.master.state}")

        print("[EC] EtherCAT OPERATIONAL")

    # ---------- pack / unpack ----------
    @staticmethod
    def _pack_outputs(cmd: dict) -> bytes:
        buf = bytearray(12)
        buf[0] = cmd["id"] & 0xFF
        buf[1] = cmd["control_mode"] & 0xFF
        buf[2] = cmd["torque_enabled"] & 0xFF
        buf[3] = cmd["LED_state"] & 0xFF
        buf[4:8] = int(cmd["target_position"]).to_bytes(
            4, byteorder='little', signed=True)
        buf[8:12] = int(cmd["target_velocity"]).to_bytes(
            4, byteorder='little', signed=True)
        return bytes(buf)

    @staticmethod
    def _unpack_inputs(buf: bytes) -> dict:
        if len(buf) < 16:
            raise ValueError(f"Input buffer too short: {len(buf)}")
        return {
            "id": buf[0],
            "state": buf[1],
            "present_position": int.from_bytes(
                buf[2:6], byteorder='little', signed=True),
            "present_velocity": int.from_bytes(
                buf[6:10], byteorder='little', signed=True),
            "present_current": int.from_bytes(
                buf[10:12], byteorder='little', signed=True),
            "present_temperature": int.from_bytes(
                buf[12:14], byteorder='little', signed=True),
            "baudrate": buf[14],
            "firmware_version": buf[15],
        }

    # ---------- main loop ----------
    def run(self):
        try:
            self._init_ethercat()
            self.connected.emit()
        except Exception as e:
            self.error_occurred.emit(f"EtherCAT init failed: {e}")
            return

        cycle_s = 0.002
        publish_divider = 0
        while self.running:
            t0 = time.perf_counter()
            try:
                with self._command_lock:
                    cmd = dict(self._latest_command)

                self.slave.output = self._pack_outputs(cmd)
                self.master.send_processdata()
                self.master.receive_processdata(500)
                publish_divider += 1
                if publish_divider >= 25:
                    publish_divider = 0
                    inputs = bytes(self.slave.input)
                    status = self._unpack_inputs(inputs)
                    self.status_received.emit(status)

            except Exception as e:
                self.error_occurred.emit(str(e))

            dt = time.perf_counter() - t0
            sleep_time = cycle_s - dt
            if sleep_time > 0:
                time.sleep(sleep_time)

    def stop(self):
        self.running = False
        self.wait(2000)

        try:
            if self.master is not None:
                self.master.state = pysoem.INIT_STATE
                self.master.write_state()
                self.master.close()
        except Exception:
            pass


# ===================== Main Window =====================
class MainWindow(QMainWindow):
    STATE_LABELS = {
        0: "IDLE",
        1: "INIT",
        2: "READY",
        3: "RUNNING",
        4: "ERROR",
        5: "OFF"
    }

    OPERATING_MODE_LABELS = {
        0: "Unknown",
        1: "Velocity",
        3: "Position",
    }

    BAUDRATE_LABELS = {
        0: "9600",
        1: "57600",
        2: "115200",
        3: "1M",
        4: "2M",
        5: "3M",
        6: "4M",
    }

    def __init__(self, ifname: str = "enx207bd2b452c6"):
        super().__init__()
        self.setWindowTitle("Motor Master IHM")
        self.setMinimumSize(900, 600)

        # EtherCAT thread (replaces ROS2)
        self.ec_thread = EthercatThread(ifname)
        self.ec_thread.status_received.connect(self.update_status)
        self.ec_thread.error_occurred.connect(self.on_ec_error)
        self.ec_thread.connected.connect(
            lambda: self.statusBar().showMessage("EtherCAT connected"))
        self.ec_thread.start()

        self.init_ui()

    # -------------------- UI setup --------------------
    def init_ui(self):
        central_widget = QWidget()
        self.setCentralWidget(central_widget)

        # --- Command group ---
        command_group = QGroupBox("Motor command")
        command_layout = QGridLayout()

        self.id_edit = QLineEdit("1")

        self.target_position_slider = QSlider(Qt.Horizontal)
        self.target_position_slider.setMinimum(0)
        self.target_position_slider.setMaximum(4095)
        self.position_label = QLabel("0")
        self.target_position_slider.valueChanged.connect(
            self.update_position_label)

        self.target_velocity_slider = QSlider(Qt.Horizontal)
        self.target_velocity_slider.setMinimum(-200)
        self.target_velocity_slider.setMaximum(200)
        self.velocity_label = QLabel("0")
        self.target_velocity_slider.valueChanged.connect(
            self.update_velocity_label)

        self.reset_velocity_button = QPushButton("Reset")
        self.reset_velocity_button.setFixedWidth(100)
        self.reset_velocity_button.clicked.connect(self.reset_velocity)

        self.control_mode_combo = QComboBox()
        self.control_mode_combo.addItem("Position", 3)
        self.control_mode_combo.addItem("Velocity", 1)

        self.torque_switch = self._create_switch("Torque")
        self.led_switch = self._create_switch("LED")

        command_layout.addWidget(QLabel("ID"), 0, 0)
        command_layout.addWidget(self.id_edit, 0, 1)

        command_layout.addWidget(QLabel("Target position"), 1, 0)
        command_layout.addWidget(self.target_position_slider, 1, 1)
        command_layout.addWidget(self.position_label, 1, 2)

        command_layout.addWidget(QLabel("Target velocity"), 2, 0)
        command_layout.addWidget(self.target_velocity_slider, 2, 1)
        command_layout.addWidget(self.velocity_label, 2, 2)
        command_layout.addWidget(self.reset_velocity_button, 2, 3)

        command_layout.addWidget(QLabel("Control mode"), 3, 0)
        command_layout.addWidget(self.control_mode_combo, 3, 1)

        command_layout.addWidget(self.torque_switch, 4, 0, 1, 2)
        command_layout.addWidget(self.led_switch, 5, 0, 1, 2)

        command_group.setLayout(command_layout)

        # --- Status group ---
        status_group = QGroupBox("Motor status")
        status_layout = QGridLayout()

        self.present_position_lcd = QLCDNumber()
        self.present_velocity_lcd = QLCDNumber()
        self.present_current_lcd = QLCDNumber()
        self.present_temperature_lcd = QLCDNumber()

        self.id_value = QLabel("0")
        self.state_value = QLabel("UNKNOWN")
        self.firmware_version = QLabel("0")
        self.baudrate_value = QLabel("0")

        self.raw_status_label = QLabel("Waiting for EtherCAT…")
        self.raw_status_label.setWordWrap(True)

        status_layout.addWidget(QLabel("ID"), 0, 0)
        status_layout.addWidget(self.id_value, 0, 1)
        status_layout.addWidget(QLabel("Present position"), 1, 0)
        status_layout.addWidget(self.present_position_lcd, 1, 1)
        status_layout.addWidget(QLabel("Present velocity"), 2, 0)
        status_layout.addWidget(self.present_velocity_lcd, 2, 1)
        status_layout.addWidget(QLabel("Present current"), 3, 0)
        status_layout.addWidget(self.present_current_lcd, 3, 1)
        status_layout.addWidget(QLabel("Present temperature"), 4, 0)
        status_layout.addWidget(self.present_temperature_lcd, 4, 1)
        status_layout.addWidget(QLabel("State"), 5, 0)
        status_layout.addWidget(self.state_value, 5, 1)
        status_layout.addWidget(QLabel("Operating Mode"), 6, 0)
        status_layout.addWidget(self.firmware_version, 6, 1)
        status_layout.addWidget(QLabel("Baudrate"), 8, 0)
        status_layout.addWidget(self.baudrate_value, 8, 1)
        status_layout.addWidget(QLabel("Raw status"), 9, 0)
        status_layout.addWidget(self.raw_status_label, 9, 1)

        status_group.setLayout(status_layout)

        # --- Main layout ---
        main_layout = QGridLayout(central_widget)
        main_layout.addWidget(command_group, 0, 0)
        main_layout.addWidget(status_group, 0, 1)

        # --- Connections ---
        self.control_mode_combo.currentIndexChanged.connect(
            self.update_control_mode_ui)
        self.target_position_slider.sliderReleased.connect(
            self.send_command)
        self.target_velocity_slider.sliderReleased.connect(
            self.send_command)
        self.torque_switch.stateChanged.connect(self.send_command)
        self.led_switch.stateChanged.connect(self.send_command)

        self.update_control_mode_ui()

    # -------------------- helpers --------------------
    @staticmethod
    def _create_switch(text):
        cb = QCheckBox(text)
        cb.setStyleSheet("""
            QCheckBox::indicator {
                width: 50px; height: 25px;
            }
            QCheckBox::indicator:unchecked {
                background-color: #ccc; border-radius: 12px;
            }
            QCheckBox::indicator:checked {
                background-color: #00ff00; border-radius: 12px;
            }
        """)
        return cb

    def update_position_label(self, value):
        self.position_label.setText(str(value))

    def update_velocity_label(self, value):
        self.velocity_label.setText(str(value))

    # -------------------- build & send command --------------------
    def _build_command(self, torque_override=None, led_override=None) -> dict:
        mode = self.control_mode_combo.currentData()

        if mode == 3:
            pos = self.target_position_slider.value()
            vel = 0
        elif mode == 1:
            pos = 0
            vel = self.target_velocity_slider.value()
        else:
            pos = 0
            vel = 0

        torque = (torque_override if torque_override is not None
                  else self.torque_switch.isChecked())
        led = (led_override if led_override is not None
               else self.led_switch.isChecked())

        return {
            "id": int(self.id_edit.text().strip() or "1"),
            "control_mode": mode,
            "torque_enabled": int(torque),
            "LED_state": int(led),
            "target_position": pos,
            "target_velocity": vel,
        }

    def send_command(self, _=None):
        """Build command dict and push it directly to the EC thread."""
        cmd = self._build_command()
        self.ec_thread.update_command(cmd)
        print(f"[SEND] {cmd}")

    def _send_with_overrides(self, **kw):
        cmd = self._build_command(**kw)
        self.ec_thread.update_command(cmd)
        print(f"[SEND] {cmd}")

    # -------------------- control mode switch --------------------
    def update_control_mode_ui(self):
        if not hasattr(self, 'torque_switch'):
            return

        mode = self.control_mode_combo.currentData()
        self.target_position_slider.setEnabled(mode == 3)
        self.target_velocity_slider.setEnabled(mode == 1)

        # Torque off → change mode → restore torque
        previous_torque = self.torque_switch.isChecked()
        self.torque_switch.setChecked(False)
        self._send_with_overrides(torque_override=False)

        self.send_command()

        if previous_torque:
            self.torque_switch.setChecked(True)
            self._send_with_overrides(torque_override=True)

    # -------------------- update status from EC thread --------------------
    def update_status(self, status: dict):
        raw_text = "; ".join(f"{k}={v}" for k, v in status.items())
        self.raw_status_label.setText(raw_text)

        try:
            self.id_value.setText(str(status.get("id", 0)))

            raw_pos = int(status.get("present_position", 0))
            self.present_position_lcd.display(raw_pos % 4096)
            self.present_velocity_lcd.display(
                int(status.get("present_velocity", 0)))
            self.present_current_lcd.display(
                int(status.get("present_current", 0)))
            self.present_temperature_lcd.display(
                int(status.get("present_temperature", 0)))

            raw_state = int(status.get("state", 0))
            self.state_value.setText(
                self.STATE_LABELS.get(raw_state, f"UNKNOWN ({raw_state})"))

            raw_mode = int(status.get("firmware_version", 0))
            self.firmware_version.setText(
                self.OPERATING_MODE_LABELS.get(
                    raw_mode, f"Unknown ({raw_mode})"))

            raw_baud = int(status.get("baudrate", 0))
            self.baudrate_value.setText(
                self.BAUDRATE_LABELS.get(raw_baud, f"Unknown ({raw_baud})"))

            # Disable controls on error
            in_error = (raw_state == 4)
            mode = self.control_mode_combo.currentData()
            self.target_position_slider.setEnabled(
                not in_error and mode == 3)
            self.target_velocity_slider.setEnabled(
                not in_error and mode == 1)
            self.reset_velocity_button.setEnabled(not in_error)
            self.control_mode_combo.setEnabled(not in_error)
            self.torque_switch.setEnabled(not in_error)
            self.led_switch.setEnabled(not in_error)

        except Exception as e:
            print(f"[ERROR update_status] {e}")

    def on_ec_error(self, msg: str):
        self.statusBar().showMessage(f"EC error: {msg}")
        print(f"[EC ERROR] {msg}")

    def reset_velocity(self):
        self.target_velocity_slider.setValue(0)
        self.send_command()

    # -------------------- cleanup --------------------
    def closeEvent(self, event):
        self.ec_thread.stop()
        event.accept()


# ===================== Entry point =====================
def main():
    import argparse
    parser = argparse.ArgumentParser(description="Motor IHM + EtherCAT")
    parser.add_argument(
        "--ifname", default="enx207bd2b452c6",
        help="Network interface for EtherCAT (default: enx207bd2b452c6)")
    args = parser.parse_args()

    app = QApplication(sys.argv)
    window = MainWindow(ifname=args.ifname)
    window.show()
    sys.exit(app.exec_())


if __name__ == "__main__":
    main()