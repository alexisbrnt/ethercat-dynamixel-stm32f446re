# 🔌 EtherCAT Communication for Dynamixel by STM32F446RE

> An embedded **EtherCAT slave node** built around the STM32F446RE, driving a Dynamixel XM430-W350 servomotor in real time.

## 📖 About

An embedded EtherCAT slave node based on the **STM32F446RE** microcontroller, designed to drive a **Dynamixel XM430-W350** servomotor through the Dynamixel Protocol 2.0. The STM32 interfaces with an EtherCAT master over SPI (via a LAN9252 slave controller) and translates incoming commands into UART/RS-485 frames sent to the motor.

This project is developed as part of an internship at **Heemskerk Innovative Technology (HIT)**, Delft, The Netherlands.

---

## 🎯 Goal

Design and implement a new real-time communication architecture allowing an EtherCAT master to command a Dynamixel servomotor (position, velocity, etc.) and read back its live telemetry (position, velocity, current, temperature) with deterministic timing.

**Data path:**

```
EtherCAT Master  ──►  LAN9252 (SPI slave)  ──►  STM32F446RE  ──►  MAX485  ──►  Dynamixel XM430-W350
```

---

## 🧩 Hardware

| Component | Reference |
|---|---|
| Microcontroller | STM32F446RE (ARM Cortex-M4 @ 180 MHz, 512 KB Flash, 128 KB SRAM) |
| Development board | Nucleo-F446RE (integrated ST-LINK) |
| EtherCAT slave controller | **EVB-LAN9252-SPI** (Microchip evaluation board) |
| Motor | Dynamixel XM430-W350 (Protocol 2.0) |
| UART ↔ RS-485 transceiver | MAX485 |
| Motor power supply | SMPS2Dynamixel |

---

## 📌 STM32F446RE Pinout

### Dynamixel communication (UART + RS-485)

| STM32 Pin | Function | Connected to |
|---|---|---|
| **PA9** | UART TX | MAX485 DI |
| **PA10** | UART RX | MAX485 RO |
| **PB3** | DE/RE (RS-485 direction control) | MAX485 DE + RE |

### EtherCAT communication (SPI to LAN9252)

| STM32 Pin | Function | Connected to |
|---|---|---|
| **PA5** | SPI SCK | LAN9252 SCK |
| **PA6** | SPI MISO | LAN9252 MISO |
| **PA7** | SPI MOSI | LAN9252 MOSI |
| **PB6** | SPI CS | LAN9252 CS |

---

## ⚙️ Configuration

| Parameter | Value |
|---|---|
| UART baudrate (STM32 ↔ Dynamixel) | **3,000,000 bauds** (3 Mbps) |
| Dynamixel protocol | **2.0** |
| Default motor ID | **1** |
| SPI frequency (STM32 ↔ LAN9252) | **11.25 MHz** |
| EtherCAT cycle time | **2 ms** |

---

## 🚀 Usage

### Build and flash the STM32 firmware

1. Open **STM32CubeIDE** 
2. `File → Open Projects from File System…` and select the `firmware/` folder.
3. Build the project: `Project → Build Project` (or `Ctrl+B`).
4. Connect the Nucleo-F446RE over USB and flash: `Run → Run` (or `F11` for debug mode).

### Run the EtherCAT master and GUI

The `ec_motor.py` script is a single-program tool that launches the **EtherCAT master** (using `pysoem`) and a **PyQt5 graphical interface** in the same process. The master runs its cyclic loop (2 ms period) in a dedicated thread and communicates with the GUI through Qt signals.

The GUI allows the user to:

- select the motor **ID** and **control mode** (Position or Velocity),
- set a **target position** (0–4095) or **target velocity** (−200 to +200) through sliders,
- toggle the **torque** and the **LED** on the motor,
- monitor in real time the **present position, velocity, current, temperature**, the motor **state** (`IDLE`, `INIT`, `READY`, `RUNNING`, `ERROR`, `OFF`), the current **operating mode** and the **baudrate**.

**Dependencies** (tested on Linux, Ubuntu/Debian):

```bash
pip install -r scripts/requirements.txt
```

**Launch** (from the root of the repository):

```bash
sudo python3 scripts/ec_motor.py --ifname <your_network_interface>
```

#### ⚠️ Network interface — must be adapted to your machine

The EtherCAT master binds to a specific network interface (the one physically connected to the EtherCAT slave, typically a USB-to-Ethernet adapter). The interface name depends on your hardware and operating system, so **it must be changed in two places**:

1. In the **launch command**, via the `--ifname` argument.
2. In the **default value** inside `scripts/ec_motor.py` (the current hardcoded default is the interface used during development and will not match your setup).

To find the name of your own interface on Linux:

```bash
ip link show
```

Look for an entry matching the adapter you plugged in (often starting with `enx...` for USB-to-Ethernet, or `eth...` / `enp...` for built-in Ethernet).

**Other notes:**

- `sudo` is required because the EtherCAT master sends raw Ethernet frames directly on the network interface.

---

## 📚 Resources & references

- [Dynamixel XM430-W350 e-Manual](https://emanual.robotis.com/docs/en/dxl/x/xm430-w350/)
- [Dynamixel Protocol 2.0](https://emanual.robotis.com/docs/en/dxl/protocol2/)
- [STM32F446RE datasheet](https://www.st.com/en/microcontrollers-microprocessors/stm32f446re.html)
- [Microchip LAN9252 product page](https://www.microchip.com/en-us/product/lan9252)
- [EtherCAT Technology Group (ETG)](https://www.ethercat.org/)
