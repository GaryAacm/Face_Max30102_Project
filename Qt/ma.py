import qrcode
from datetime import datetime
from PIL import Image

def get_device_serial():
    try:
        with open('/proc/cpuinfo','r') as f:
            for line in f:
                if line.startswith('Serial'):
                    return line.strip().split(':')[1]
    except Exception as e:
        return "000000000"

current_time = datetime.now().strftime("%Y-%m-%d %H:%M:%S")

device_serial = get_device_serial();

data = f"Device:{device_serial},Time:{current_time}"

# 创建QRCode对象
qr = qrcode.QRCode(
    version=1,  # 版本号
    error_correction=qrcode.constants.ERROR_CORRECT_L,  
    box_size=10,  
    border=4,  
)

# 向QRCode对象添加当前时间数据
qr.add_data(data)
qr.make(fit=True)

img = qr.make_image(fill='black', back_color='white')

img.save("Ma.png")

img.show()

