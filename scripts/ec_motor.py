import sys

from PyQt5.QtWidgets import QApplication

from main_window import MainWindow

iplink = "enxf8e43b4e91ea"


# ===================== Entry point =====================
def main():
    import argparse

    parser = argparse.ArgumentParser(description="Motor IHM + EtherCAT")
    parser.add_argument(
        "--ifname",
        default=iplink,
        help="Network interface for EtherCAT (default: iplink value)",
    )
    args = parser.parse_args()

    app = QApplication(sys.argv)
    window = MainWindow(ifname=args.ifname)
    window.show()
    sys.exit(app.exec_())


if __name__ == "__main__":
    main()
