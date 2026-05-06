import sys

from PyQt5.QtWidgets import QApplication

from main_window import MainWindow


# ===================== Entry point =====================
def main():
    import argparse

    parser = argparse.ArgumentParser(description="Motor IHM + EtherCAT")
    parser.add_argument(
        "--ifname",
        default="enxa453eed090bc",
        help="Network interface for EtherCAT (default: enxa453eed090bc)",
    )
    args = parser.parse_args()

    app = QApplication(sys.argv)
    window = MainWindow(ifname=args.ifname)
    window.show()
    sys.exit(app.exec_())


if __name__ == "__main__":
    main()
