from PyQt5 import QtCore, QtWidgets
import threading
import time

import pysoem


# ===================== EtherCAT Thread =====================
class EthercatThread(QtCore.QThread):
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
            "target_current": 0,
            "reboot": 0,
            "emergency_stop": 0,
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
        print(
            f"[EC] Slave 0: name={self.slave.name}, "
            f"man=0x{self.slave.man:08x}, rev=0x{self.slave.rev:08x}"
        )

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
                f"OP transition failed, current state = {self.master.state}"
            )

        print("[EC] EtherCAT OPERATIONAL")

    # ---------- pack / unpack ----------
    @staticmethod
    def _pack_outputs(cmd: dict) -> bytes:

        buf = bytearray(16)
        buf[0] = cmd["id"] & 0xFF
        buf[1] = cmd["control_mode"] & 0xFF
        buf[2] = cmd["torque_enabled"] & 0xFF
        buf[3] = cmd["LED_state"] & 0xFF
        buf[4:8] = int(cmd["target_position"]).to_bytes(
            4, byteorder="little", signed=True
        )
        buf[8:12] = int(cmd["target_velocity"]).to_bytes(
            4, byteorder="little", signed=True
        )
        buf[12:14] = int(cmd.get("target_current", 0)).to_bytes(
            2, byteorder="little", signed=True
        )
        buf[14] = cmd.get("reboot", 0) & 0xFF
        buf[15] = cmd.get("emergency_stop", 0) & 0xFF
        return bytes(buf)

    @staticmethod
    def _unpack_inputs(buf: bytes) -> dict:

        if len(buf) < 32:
            raise ValueError(f"Input buffer too short: {len(buf)}")
        return {
            "id": buf[0],
            "state": buf[1],
            "present_position": int.from_bytes(
                buf[2:6], byteorder="little", signed=True
            ),
            "present_velocity": int.from_bytes(
                buf[6:10], byteorder="little", signed=True
            ),
            "present_current": int.from_bytes(
                buf[10:12], byteorder="little", signed=True
            ),
            "present_temperature": int.from_bytes(
                buf[12:14], byteorder="little", signed=True
            ),
            "baudrate": buf[14],
            "operating_mode": buf[15],
            "max_pos_lim": int.from_bytes(buf[16:20], byteorder="little", signed=True),
            "min_pos_lim": int.from_bytes(buf[20:24], byteorder="little", signed=True),
            "velocity_lim": int.from_bytes(buf[24:28], byteorder="little", signed=True),
            "current_lim": int.from_bytes(buf[28:30], byteorder="little", signed=True),
            "hardware_error_status": buf[30],
            "moving": buf[31],
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
