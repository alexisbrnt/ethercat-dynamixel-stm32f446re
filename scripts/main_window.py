from PyQt5.QtWidgets import (
    QApplication,
    QMainWindow,
    QWidget,
    QLabel,
    QPushButton,
    QGridLayout,
    QGroupBox,
    QVBoxLayout,
    QHBoxLayout,
    QLCDNumber,
    QLineEdit,
    QComboBox,
    QCheckBox,
    QSlider,
)

from PyQt5.QtCore import Qt

from ec_thread import EthercatThread


# ===================== Main Window =====================
class MainWindow(QMainWindow):
    STATE_LABELS = {
        0: "IDLE",
        1: "INIT",
        2: "READY",
        3: "RUNNING",
        4: "ERROR",
        5: "OFF",
        6: "SW_EMERGENCY_STOP",
        7: "GRIPPER_MODE",
    }

    OPERATING_MODE_LABELS = {
        0: "Current",
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
        self.setMinimumSize(1000, 650)

        self._emergency_active = False

        self.ec_thread = EthercatThread(ifname)
        self.ec_thread.status_received.connect(self.update_status)
        self.ec_thread.error_occurred.connect(self.on_ec_error)
        self.ec_thread.connected.connect(
            lambda: self.statusBar().showMessage("EtherCAT connected")
        )
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
        self.target_position_slider.setEnabled(False)  # disabled until limits known
        self.position_label = QLabel("– / –")  # shows  value [min, max]
        self.target_position_slider.valueChanged.connect(self.update_position_label)

        self.btn_ferme = QPushButton("Fermé")
        self.btn_ferme.setFixedWidth(100)
        self.btn_ouvert = QPushButton("Ouvert")
        self.btn_ouvert.setFixedWidth(100)
        self.btn_ferme.clicked.connect(self.go_to_min)
        self.btn_ouvert.clicked.connect(self.go_to_max)

        self.target_velocity_slider = QSlider(Qt.Horizontal)
        self.target_velocity_slider.setMinimum(-200)
        self.target_velocity_slider.setMaximum(200)
        self.velocity_label = QLabel("0")
        self.target_velocity_slider.valueChanged.connect(self.update_velocity_label)

        self.reset_velocity_button = QPushButton("Reset")
        self.reset_velocity_button.setFixedWidth(100)
        self.reset_velocity_button.clicked.connect(self.reset_velocity)

        self.target_current_slider = QSlider(Qt.Horizontal)
        self.target_current_slider.setMinimum(-1193)
        self.target_current_slider.setMaximum(1193)
        self.target_current_slider.setEnabled(False)
        self.current_label = QLabel("- / -")
        self.target_current_slider.valueChanged.connect(self.update_current_label)

        self.reset_current_button = QPushButton("Reset")
        self.reset_current_button.setFixedWidth(100)
        self.reset_current_button.clicked.connect(self.reset_current)

        self.control_mode_combo = QComboBox()
        self.control_mode_combo.addItem("Position", 3)
        self.control_mode_combo.addItem("Velocity", 1)
        self.control_mode_combo.addItem("Current", 0)

        self.torque_switch = self._create_switch("Torque")
        self.led_switch = self._create_switch("LED")

        self.emergency_stop_button = QPushButton("SW EMERGENCY STOP")
        self.emergency_stop_button.setFixedHeight(70)
        self.emergency_stop_button.setCheckable(True)
        self.emergency_stop_button.clicked.connect(self.toggle_emergency_stop)
        self._update_emergency_style(False)

        command_layout.addWidget(QLabel("ID"), 0, 0)
        command_layout.addWidget(self.id_edit, 0, 1)

        command_layout.addWidget(QLabel("Target position"), 1, 0)
        command_layout.addWidget(self.target_position_slider, 1, 1)
        command_layout.addWidget(self.position_label, 1, 2)
        command_layout.addWidget(self.btn_ferme, 1, 3)
        command_layout.addWidget(self.btn_ouvert, 1, 4)

        command_layout.addWidget(QLabel("Target velocity"), 2, 0)
        command_layout.addWidget(self.target_velocity_slider, 2, 1)
        command_layout.addWidget(self.velocity_label, 2, 2)
        command_layout.addWidget(self.reset_velocity_button, 2, 3)

        command_layout.addWidget(QLabel("Target current"), 3, 0)
        command_layout.addWidget(self.target_current_slider, 3, 1)
        command_layout.addWidget(self.current_label, 3, 2)
        command_layout.addWidget(self.reset_current_button, 3, 3)

        command_layout.addWidget(QLabel("Control mode"), 4, 0)
        command_layout.addWidget(self.control_mode_combo, 4, 1)

        command_layout.addWidget(self.torque_switch, 5, 0, 1, 2)
        command_layout.addWidget(self.led_switch, 6, 0, 1, 2)

        command_layout.addWidget(self.emergency_stop_button, 7, 0, 1, 5)

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
        self.operating_mode_value = QLabel("–")
        self.moving_value = QLabel("–")
        self.hw_error_value = QLabel("–")
        self.firmware_version = QLabel("0")
        self.baudrate_value = QLabel("0")

        self.max_pos_lim_value = QLabel("–")
        self.min_pos_lim_value = QLabel("–")
        self.velocity_lim_value = QLabel("–")
        self.current_lim_value = QLabel("–")

        self.raw_status_label = QLabel("Waiting for EtherCAT...")
        self.raw_status_label.setWordWrap(True)

        row = 0

        def _add(label, widget):
            nonlocal row
            status_layout.addWidget(QLabel(label), row, 0)
            status_layout.addWidget(widget, row, 1)
            row += 1

        _add("ID", self.id_value)
        _add("Present position", self.present_position_lcd)
        _add("Present velocity", self.present_velocity_lcd)
        _add("Present current", self.present_current_lcd)
        _add("Present temperature", self.present_temperature_lcd)
        _add("State", self.state_value)
        _add("Operating mode", self.operating_mode_value)
        _add("Baudrate", self.baudrate_value)
        _add("Moving", self.moving_value)
        _add("HW error status", self.hw_error_value)
        _add("Max pos limit", self.max_pos_lim_value)
        _add("Min pos limit", self.min_pos_lim_value)
        _add("Velocity limit", self.velocity_lim_value)
        _add("Current limit", self.current_lim_value)
        _add("Raw status", self.raw_status_label)

        status_group.setLayout(status_layout)

        # --- Main layout ---
        main_layout = QGridLayout(central_widget)
        main_layout.addWidget(command_group, 0, 0)
        main_layout.addWidget(status_group, 0, 1)

        # --- Connections ---
        self.control_mode_combo.currentIndexChanged.connect(self.update_control_mode_ui)
        self.target_position_slider.sliderReleased.connect(self.send_command)
        self.target_velocity_slider.sliderReleased.connect(self.send_command)
        self.target_current_slider.sliderReleased.connect(self.send_command)
        self.torque_switch.stateChanged.connect(self.send_command)
        self.led_switch.stateChanged.connect(self.send_command)

        self.update_control_mode_ui()

    # -------------------- helpers --------------------
    @staticmethod
    def _create_switch(text):
        cb = QCheckBox(text)
        cb.setStyleSheet(
            """
            QCheckBox::indicator {
                width: 50px; height: 25px;
            }
            QCheckBox::indicator:unchecked {
                background-color: #ccc; border-radius: 12px;
            }
            QCheckBox::indicator:checked {
                background-color: #00ff00; border-radius: 12px;
            }
        """
        )
        return cb

    def update_position_label(self, value):
        lo = self.target_position_slider.minimum()
        hi = self.target_position_slider.maximum()
        self.position_label.setText(str(value))

    def update_velocity_label(self, value):
        self.velocity_label.setText(str(value))

    def update_current_label(self, value):
        lo = self.target_current_slider.minimum()
        hi = self.target_current_slider.maximum()
        self.current_label.setText(str(value))

    # -------------------- build & send command --------------------
    def _build_command(self, torque_override=None, led_override=None) -> dict:
        mode = self.control_mode_combo.currentData()

        if mode == 3:
            pos = self.target_position_slider.value()
            vel = 0
            cur = 0
        elif mode == 1:
            pos = 0
            vel = self.target_velocity_slider.value()
            cur = 0
        elif mode == 0:
            pos = 0
            vel = 0
            cur = self.target_current_slider.value()
        else:
            pos = 0
            vel = 0
            cur = 0

        torque = (
            torque_override
            if torque_override is not None
            else self.torque_switch.isChecked()
        )
        led = led_override if led_override is not None else self.led_switch.isChecked()

        return {
            "id": int(self.id_edit.text().strip() or "1"),
            "control_mode": mode,
            "torque_enabled": int(torque),
            "LED_state": int(led),
            "target_position": pos,
            "target_velocity": vel,
            "target_current": cur,
            "reboot": 0,
            "sw_emergency_stop": 1 if self._emergency_active else 0,
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
        if not hasattr(self, "torque_switch"):
            return

        mode = self.control_mode_combo.currentData()
        self.target_position_slider.setEnabled(mode == 3)
        self.target_velocity_slider.setEnabled(mode == 1)
        self.target_current_slider.setEnabled(mode == 0)

        # Torque off → change mode → restore torque
        previous_torque = self.torque_switch.isChecked()
        self.torque_switch.setChecked(False)
        self._send_with_overrides(torque_override=False)

        self.send_command()

        if previous_torque:
            self.torque_switch.setChecked(True)
            self._send_with_overrides(torque_override=True)

    def toggle_emergency_stop(self):
        new_state = not self._emergency_active
        self._emergency_active = new_state
        self._update_emergency_style(new_state)
        if new_state:
            self.torque_switch.blockSignals(True)
            self.torque_switch.setChecked(False)
            self.torque_switch.blockSignals(False)

        cmd = self._build_command()
        cmd["emergency_stop"] = 1 if new_state else 0
        self.ec_thread.update_command(cmd)
        print(
            f"[SEND] SW EMERGENCY STOP = {new_state}, torque_enabled = {cmd['torque_enabled']}"
        )

    # -------------------- update status from EC thread --------------------
    def update_status(self, status: dict):
        raw_text = "; ".join(f"{k}={v}" for k, v in status.items())
        self.raw_status_label.setText(raw_text)

        try:
            self.id_value.setText(str(status.get("id", 0)))

            raw_pos = int(status.get("present_position", 0))
            self.present_position_lcd.display(raw_pos % 4096)
            self.present_velocity_lcd.display(int(status.get("present_velocity", 0)))
            self.present_current_lcd.display(int(status.get("present_current", 0)))
            self.present_temperature_lcd.display(
                int(status.get("present_temperature", 0))
            )

            raw_state = int(status.get("state", 0))
            self.state_value.setText(
                self.STATE_LABELS.get(raw_state, f"UNKNOWN ({raw_state})")
            )

            raw_mode = int(status.get("operating_mode", 0))
            self.operating_mode_value.setText(
                self.OPERATING_MODE_LABELS.get(raw_mode, f"Unknown ({raw_mode})")
            )

            raw_baud = int(status.get("baudrate", 0))
            self.baudrate_value.setText(
                self.BAUDRATE_LABELS.get(raw_baud, f"Unknown ({raw_baud})")
            )

            self.moving_value.setText("Yes" if status.get("moving") else "No")
            self.hw_error_value.setText(
                f"0b{status.get('hardware_error_status', 0):08b}"
            )

            max_lim = int(status.get("max_pos_lim", 0))
            min_lim = int(status.get("min_pos_lim", 0))
            vel_lim = int(status.get("velocity_lim", 0))
            cur_lim = int(status.get("current_lim", 0))

            self.max_pos_lim_value.setText(str(max_lim))
            self.min_pos_lim_value.setText(str(min_lim))
            self.velocity_lim_value.setText(str(vel_lim))
            self.current_lim_value.setText(str(cur_lim))

            if max_lim != min_lim and (
                max_lim != self.target_position_slider.maximum()
                or min_lim != self.target_position_slider.minimum()
            ):
                current_val = self.target_position_slider.value()
                self.target_position_slider.setMinimum(min_lim)
                self.target_position_slider.setMaximum(max_lim)
                # Clamp current value inside new range
                self.target_position_slider.setValue(
                    max(min_lim, min(max_lim, current_val))
                )
                self._LIMITS_INITIALIZED = True
                self.statusBar().showMessage(
                    f"EtherCAT connected — position limits set: [{min_lim}, {max_lim}]"
                )

            if cur_lim != self.target_current_slider.maximum():
                current_val = self.target_current_slider.maximum()
                self.target_current_slider.setMinimum(cur_lim * (-1))
                self.target_current_slider.setMaximum(cur_lim)
                self.target_current_slider.setValue(
                    max(cur_lim * (-1), min(cur_lim, current_val))
                )
                self._LIMITS_INITIALIZED = True

            # Disable controls on error
            in_error = raw_state == 4
            in_gripper = raw_state == 7
            in_emergency = raw_state == 6
            in_off = raw_state == 5
            can_control = not in_error and not in_emergency
            mode = self.control_mode_combo.currentData()
            self.target_position_slider.setEnabled(
                can_control and mode == 3 and not in_off
            )
            self.target_velocity_slider.setEnabled(
                can_control and mode == 1 and not in_gripper and not in_off
            )
            self.target_current_slider.setEnabled(
                can_control and mode == 0 and not in_gripper and not in_off
            )
            self.reset_current_button.setEnabled(
                can_control and not in_gripper and not in_off
            )
            self.reset_velocity_button.setEnabled(
                can_control and not in_gripper and not in_off
            )
            self.control_mode_combo.setEnabled(can_control and not in_gripper)
            self.torque_switch.setEnabled(can_control)
            self.led_switch.setEnabled(can_control)
            self.btn_ferme.setEnabled(can_control and not in_off)
            self.btn_ouvert.setEnabled(can_control and not in_off)
            self.emergency_stop_button.setEnabled(not in_error)

            if in_emergency != self._emergency_active:
                self._emergency_active = in_emergency
                self._update_emergency_style(in_emergency)

        except Exception as e:
            print(f"[ERROR update_status] {e}")

    def _update_emergency_style(self, active: bool):
        if active:
            self.emergency_stop_button.setText("EMERGENCY ACTIVE")
            self.emergency_stop_button.setStyleSheet(
                """
                QPushButton {
                    background-color: #aa0000; color: white;
                    font-weight: bold; font-size: 40px;
                    border-radius: 8px; border: 3px solid #ff0000;
                }
            """
            )

        else:
            self.emergency_stop_button.setText("SW EMERGENCY STOP")
            self.emergency_stop_button.setStyleSheet(
                """
                QPushButton {
                    background-color: #ff0000; color: white;
                    font-weight: bold; font-size: 40px;
                    border-radius: 8px;
                }
                QPushButton:pressed { background-color: #aa0000; }
            """
            )

    def on_ec_error(self, msg: str):
        self.statusBar().showMessage(f"EC error: {msg}")
        print(f"[EC ERROR] {msg}")

    def reset_velocity(self):
        self.target_velocity_slider.setValue(0)
        self.send_command()

    def reset_current(self):
        self.target_current_slider.setValue(0)
        self.send_command()

    def go_to_min(self):
        self.target_position_slider.setValue(self.target_position_slider.minimum())
        self.send_command()

    def go_to_max(self):
        self.target_position_slider.setValue(self.target_position_slider.maximum())
        self.send_command()

    # -------------------- cleanup --------------------
    def closeEvent(self, event):
        self.ec_thread.stop()
        event.accept()
