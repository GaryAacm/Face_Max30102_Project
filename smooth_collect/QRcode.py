import qrcode
from datetime import datetime
import requests
import os
import random
import string
import urllib.parse

def generate_random_numbers(count):
    return ''.join(random.choice(string.digits) for _ in range(count))

def generate_random_letters(count):
    return ''.join(random.choice(string.ascii_letters)for _ in range(count))


def combine_num_letters(num_count, letter_count):
    numbers = generate_random_numbers(num_count)
    letters = generate_random_letters(letter_count)
    combine = numbers + letters
    combined_list = list(combine)
    return ''.join(combined_list)

def get_device_serial():
    try:
        with open('/proc/cpuinfo', 'r') as f:
            for line in f:
                if line.startswith('Serial'):
                    return line.strip().split(':')[1]
    except Exception as e:
        return "000000000"

user_uuid = " "
current_time = datetime.now().strftime("%Y-%m-%d-%H-%M-%S")
device_serial = get_device_serial()

# 生成 sample_id
numbers_and_letters = combine_num_letters(3, 3)
sample_id = f"{device_serial}-{current_time}-{numbers_and_letters}"

param = {"sample_id":sample_id.strip()}

qr = qrcode.QRCode(
    version=1,
    error_correction=qrcode.constants.ERROR_CORRECT_L,
    box_size=10,
    border=4,
)
qr.add_data(sample_id)
qr.make(fit=True)

img = qr.make_image(fill='black', back_color='white')

img.save("QRcode.png")
os.system(f"feh QRcode.png")

server_url = "http://sp.grifcc.top:8080/collect/get_user"



try:
    # 发起 GET 请求
    response = requests.get(server_url, params=param, timeout=5)

    # 检查响应状态码
    if response.status_code == 200:
        result = response.json()  # 将响应转为 JSON 格式
        user_uuid = result.get("user_uuid", None)  # 从响应中提取 user_uuid
        if user_uuid:
            print(f"Success! Retrieved user_uuid: {user_uuid}")
        else:
            print("Failed to retrieve user_uuid from response.")
    else:
        print(f"Request failed with status code {response.status_code}, Response: {response.text}")

# 异常处理
except requests.exceptions.Timeout:
    print("Connection timeout!")
except requests.exceptions.ConnectionError:
    print("Failed to connect to the server. Please check your network or server status.")

# 打印 sample_id 以及获取到的 user_uuid（如果有）
print(f"Generated sample_id: {sample_id}")
if 'user_uuid' in locals():
    print(f"Retrieved user_uuid: {user_uuid}")



user_message = f"sample_id:{sample_id.strip()},user_uuid:{user_uuid}"

try:
    with open("User_Message.txt", "w") as f:
        f.write(user_message)
except IOError as e:
    print(f"Failed to write user message to file: {e}")
