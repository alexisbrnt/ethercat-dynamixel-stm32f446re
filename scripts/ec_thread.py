from PyQt5 import QtCore, QtWidgets
import threading
import time

import pysoem

_SAFE_COMMAND = {
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
_WKC_ERROR_THRESHOLD = 10

_BACKOFF_INITIAL = 1.0
_BACKOFF_FACTOR = 2.0
_BACKOFF_MAX = 30.0


iplink = "enxf8e43b4e91ea"


# ===================== EtherCAT Thread =====================
class EthercatThread(QtCore.QThread):
    status_received = QtCore.pyqtSignal(dict)
    error_occurred = QtCore.pyqtSignal(str)
    connected = QtCore.pyqtSignal()
    comm_loss = QtCore.pyqtSignal()
    comm_restored = QtCore.pyqtSignal()
    reconnecting = QtCore.pyqtSignal(int)

    def __init__(self, ifname: str = iplink):
        super().__init__()
        self.ifname = ifname
        self.running = True

        self._command_lock = threading.Lock()
        self._latest_command = dict(_SAFE_COMMAND)
        self._comm_was_lost = False

        self.master = None
        self.slave = None

    # ---------- command interface (called from GUI thread) ----------
    def update_command(self, cmd: dict):
        with self._command_lock:
            self._latest_command = cmd

    def reset_to_safe(self):
        with self._command_lock:
            self._latest_command = dict(_SAFE_COMMAND)

    def _close_master(self):
        try:
            if self.master is not None:
                self.master.state = pysoem.INIT_STATE
                self.master.write_state()
                self.master.close()
        except Exception:
            pass
        finally:
            self.master = None
            self.slave = None

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

        # Pomper plusieurs cycles avant de demander OP
        default_cmd = dict(self._latest_command)
        self.slave.output = self._pack_outputs(default_cmd)
        for _ in range(100):  # ~100 ms à 1 ms/cycle
            self.master.send_processdata()
            self.master.receive_processdata(5000)
            time.sleep(0.001)

        self.master.state = pysoem.OP_STATE
        self.master.write_state()

        # Continuer à envoyer pendant la transition
        for _ in range(200):
            self.master.send_processdata()
            self.master.receive_processdata(5000)
            state = self.master.state_check(pysoem.OP_STATE, 10_000)
            if state == pysoem.OP_STATE:
                break
            time.sleep(0.001)

        else:
            self.master.read_state()
            sl = self.master.slaves[0]
            raise RuntimeError(
                f"OP transition failed — master_state={self.master.state}, "
                f"slave_state={sl.state}, al_status=0x{sl.al_status:04x}"
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

        if len(buf) < 33:
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
            "torque_status": buf[32],
        }

    # ---------- main loop ----------
    def run(self):
        backoff = _BACKOFF_INITIAL
        attempt = 0
        first_connect = True
        while self.running:
            self._close_master()
            self.reset_to_safe()

            attempt += 1
            if not first_connect:
                self.reconnecting.emit(attempt)
                print(
                    f"[EC] Reconnection attempt #{attempt}"
                    f"- next in {backoff:0f}s if failure"
                )
            try:
                self._init_ethercat()
            except Exception as e:
                msg = f"Connection failed (attempt #{attempt}):{e}"
                print(f"[EC] {msg}")
                self.error_occurred.emit(msg)
                self._sleep_interruptible(backoff)
                backoff = min(backoff * _BACKOFF_FACTOR, _BACKOFF_MAX)
                continue
            backoff = _BACKOFF_INITIAL
            attempt = 0

            if first_connect:
                first_connect = False
                self.connected.emit()
            else:
                self.comm_restored.emit()
                print("[EC] Reconnected - safe command active")

            consecutive_errors = 0
            publish_divider = 0

            while self.running:
                t0 = time.perf_counter()

                if self.slave is None:
                    break

                try:
                    with self._command_lock:
                        cmd = dict(self._latest_command)
                    self.slave.output = self._pack_outputs(cmd)
                    self.master.send_processdata()
                    wkc = self.master.receive_processdata(2000)

                    if wkc <= 0:
                        consecutive_errors += 1
                        if consecutive_errors == _WKC_ERROR_THRESHOLD:
                            print(f"[EC] WKC = {wkc}x{consecutive_errors} - link lost")
                            self.reset_to_safe()
                            self.comm_loss.emit()
                            break
                    else:
                        consecutive_errors = 0
                        publish_divider += 1
                        if publish_divider >= 25:
                            publish_divider = 0
                            try:
                                status = self._unpack_inputs(bytes(self.slave.input))

                                self.status_received.emit(status)
                            except Exception as e:
                                self.error_occurred.emit(f"Unpack:{e}")
                except Exception as e:
                    consecutive_errors += 1
                    self.error_occurred.emit(str(e))
                    if consecutive_errors >= _WKC_ERROR_THRESHOLD:
                        self.reset_to_safe()
                        self.comm_loss.emit()
                        break
                elapsed = time.perf_counter() - t0
                if (wait := 0.00097 - elapsed) > 0:
                    time.sleep(wait)
            if self.running:
                print(f"[EC] Waiting {backoff:.0f}s before reconnection...")
                self._sleep_interruptible(backoff)
                backoff = min(backoff * _BACKOFF_FACTOR, _BACKOFF_MAX)

        self._close_master()
        print("[EC] Thread stopped")

    def _sleep_interruptible(self, seconds: float):
        deadline = time.perf_counter() + seconds
        while self.running and time.perf_counter() < deadline:
            time.sleep(0.1)

    def stop(self):
        self.running = False
        self.wait(3000)
