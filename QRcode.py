import random
import string
import requests
import qrcode
import subprocess
import time
from datetime import datetime
import os

class QRCodeGenerator:
    def __init__(self):
        pass

    def generate_random_numbers(self, count: int) -> str:
        numbers = ''.join(random.choices(string.digits, k=count))
        return numbers

    def generate_random_letters(self, count: int) -> str:
        letters = ''.join(random.choices(string.ascii_uppercase, k=count))
        return letters

    def combine_num_letters(self, num_count: int, letter_count: int) -> str:
        numbers = self.generate_random_numbers(num_count)
        letters = self.generate_random_letters(letter_count)
        combined = numbers + letters
        combined_list = list(combined)
        random.shuffle(combined_list)
        return ''.join(combined_list)

    def get_device_serial(self) -> str:
        try:
            with open("/proc/cpuinfo", "r") as cpuinfo:
                for line in cpuinfo:
                    if line.startswith("Serial"):
                        serial = line.split(":")[1].strip()
                        return serial
        except FileNotFoundError:
            pass
        return "000000000"

    def get_current_time(self) -> str:
        now = datetime.now()
        return now.strftime("%Y-%m-%d-%H-%M-%S")

    def save_qr_png(self, filename: str, data: str) -> bool:
        try:
            qr = qrcode.QRCode(
                version=1,
                error_correction=qrcode.constants.ERROR_CORRECT_L,
                box_size=10,
                border=4,
            )
            qr.add_data(data)
            qr.make(fit=True)

            img = qr.make_image(fill_color="black", back_color="white")
            img.save(filename)
            return True
        except Exception as e:
            print(f"Error saving QR code PNG: {e}")
            return False

    def open_qr_image(self, filename: str):
        try:
            if os.name == 'nt':  # For Windows
                os.startfile(filename)
            elif os.name == 'posix':
                # Attempt to use 'feh', fallback to 'xdg-open'
                if subprocess.call(['which', 'feh'], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL) == 0:
                    subprocess.Popen(['feh', filename])
                else:
                    subprocess.Popen(['xdg-open', filename])
            else:
                print("Unsupported OS for opening images.")
        except Exception as e:
            print(f"Cannot open QR code image: {e}")

    def generate_and_send_user_message(self) -> str:
        current_time = self.get_current_time()
        device_serial = self.get_device_serial()
        numbers_and_letters = self.combine_num_letters(3, 3)
        #sample_id = f"{device_serial}-{current_time}-{numbers_and_letters}"
        sample_id = f"{device_serial}-2024-10-06-14-02-21-E6Z3I8"

        send = f"sample_id :{sample_id}"

        print(f"Generated sample_id: {sample_id}")

        qr = qrcode.QRCode(
        version=1,  
        error_correction=qrcode.constants.ERROR_CORRECT_L,  
        box_size=10,  
        border=4,  
    )

        qr.add_data(send)
        qr.make(fit=True)

        img = qr.make_image(fill='black', back_color='white')

        img.save("QRcode.png")

        os.system(f"feh QRcode.png")


        # Send sample_id to server
        base_url = "http://sp.grifcc.top:8080/collect/get_user"
        params = {'data': sample_id}



        try:
            response = requests.get(base_url, params=params)
            response.raise_for_status()
            read_buffer = response.text
            print("Success in sending message")
            print(f"服务器返回的数据: {read_buffer}")
        except requests.RequestException as e:
            print(f"Failed to send message: {e}")
            read_buffer = ""


        user_message = f"{read_buffer}-{sample_id}"
        print(user_message)

        try:
            with open("User_Message.txt", "w") as f:
                f.write(user_message)
        except IOError as e:
            print(f"Failed to write user message to file: {e}")

        return user_message

if __name__ == "__main__":
    generator = QRCodeGenerator()
    user_message = generator.generate_and_send_user_message()
    print(f"Final User Message: {user_message}")

