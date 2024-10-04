import qrcode
from datetime import datetime
from PIL import Image
import requests  
import os
import random
import string 
import urllib.parse

def generate_random_numbers(count):
    return ''.join(random.choice(string.digits) for _ in range(count))

def generate_random_letters(count):
    return ''.join(random.choice(string.ascii_letters)for _ in range(count))

def combine_num_letters(num_count,letter_count):
    numbers = generate_random_numbers(num_count)
    letters = generate_random_letters(letter_count)
    combine = numbers + letters
    combined_list = list(combine)
    
    return ''.join(combined_list)

numbers_and_letters = combine_num_letters(3,3)

def get_device_serial():
    try:
        with open('/proc/cpuinfo','r') as f:
            for line in f:
                if line.startswith('Serial'):
                    return line.strip().split(':')[1]
    except Exception as e:
        return "000000000"

current_time = datetime.now().strftime("%Y-%m-%d-%H-%M-%S")

device_serial = get_device_serial()

data = f"{device_serial}-{current_time}-{numbers_and_letters}"

encode_data = urllib.parse.quote(data)
 
param = {"sample_id":data.strip()}
# param = data.strip()

qr = qrcode.QRCode(
    version=1,  
    error_correction=qrcode.constants.ERROR_CORRECT_L,  
    box_size=10,  
    border=4,  
)

qr.add_data(param)
qr.make(fit=True)

img = qr.make_image(fill='black', back_color='white')

img.save("QRcode.png")

os.system(f"feh QRcode.png")

server_url = "http://sp.grifcc.top:8080/collect/register"


try:
    response = requests.get(server_url, params=param,timeout=5)
    if response.status_code == 200:
        print("Success connect!")
    else:
        print(f"Failed {response.status_code}")
except requests.exceptions.Timeout:
    print("Connect timeout!")
except requests.exceptions.ConnectionError:
    print("Please check out http")

print(param)
