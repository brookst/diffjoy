from struct import unpack
import os
import keyboard


def main():
    dev_path = get_dev_path()
    if not dev_path:
        print("No recognised device detected")
        exit(1)
    try:
        event_loop(dev_path)
    except KeyboardInterrupt:
        pass


def event_loop(dev_path):
    steps = 9 / 1024
    last_step = 4
    pause = False
    with open(dev_path, "rb") as handle:
        for value in iter_values(handle):
            step = int(value * steps)
            if step != last_step:
                diff = step - last_step
                if step == 0 or last_step == 0:
                    keyboard.press_and_release(" ")
                elif diff > 0:
                    keyboard.press_and_release("shift+.")
                elif diff < 0:
                    keyboard.press_and_release("shift+<")
                last_step = step


def iter_values(handle):
    try:
        while True:
            word = unpack("h", handle.read(2))[0]
            yield word
    except OSError:
        pass


def get_dev_path():
    with os.scandir("/sys/class/hidraw") as dirs:
        for entry in dirs:
            path = os.path.join(entry.path, "device/uevent")
            if is_device(path):
                return os.path.join("/dev/", entry.name)


def is_device(path):
    with open(path, "r") as handle:
        for line in handle.readlines():
            if line.strip() == "HID_NAME=Skoorb Diffjoy":
                return True
        else:
            return False


def read():
    with open("/dev/hidraw0", "rb") as handle:
        while True:
            word = unpack("h", handle.read(2))
            print(f"{word}")


if __name__ == "__main__":
    main()
