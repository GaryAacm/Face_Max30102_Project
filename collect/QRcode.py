import qrcode
from datetime import datetime
from PIL import Image
import requests  

def get_device_serial():
    try:
        with open('/proc/cpuinfo','r') as f:
            for line in f:
                if line.startswith('Serial'):
                    return line.strip().split(':')[1]
    except Exception as e:
        return "000000000"

current_time = datetime.now().strftime("%Y-%m-%d %H:%M:%S")

device_serial = get_device_serial()

data = f"Device:{device_serial},Time:{current_time}"


server_url = 'http://yourserver.com/endpoint'
params = {'data': data}

#try:
response = requests.get(server_url, params=params)
    #if response.status_code == 200:
        #print("数据成功发送到服务器。")
    #else:
        #print(f"服务器返回状态码: {response.status_code}")
#except requests.exceptions.RequestException as e:
    #print(f"发送请求时出错: {e}")

qr = qrcode.QRCode(
    version=1,  
    error_correction=qrcode.constants.ERROR_CORRECT_L,  
    box_size=10,  
    border=4,  
)

qr.add_data(data)
qr.make(fit=True)

img = qr.make_image(fill='black', back_color='white')

img.save("QRcode.png")

img.show()
