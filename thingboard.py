print("Hello Core IOT")
import paho.mqtt.client as mqttclient
import time
import json
import socket

BROKER_ADDRESS = "app.coreiot.io"
BROKER_FALLBACK_IP = "213.199.63.66"
PORT = 1883
ACCESS_TOKEN = "IOT_DEVICE_A_"
ACCESS_USERNAME = "IOT_DEVICE_A_"

def subscribed(client, userdata, mid, granted_qos):
    print("Subscribed...")

def recv_message(client, userdata, message):
    print("Received: ", message.payload.decode("utf-8"))
    temp_data = {'value': True}
    try:
        jsonobj = json.loads(message.payload)
        if jsonobj['method'] == "setValue":
            temp_data['value'] = jsonobj['params']
            client.publish('v1/devices/me/attributes', json.dumps(temp_data), 1)
    except:
        pass

def connected(client, usedata, flags, rc):
    if rc == 0:
        print("Connected successfully!!")
        client.subscribe("v1/devices/me/rpc/request/+")
    else:
        print("Connection is failed, rc =", rc)

client = mqttclient.Client("IOT_DEVICE_A_")
client.username_pw_set(ACCESS_USERNAME, ACCESS_TOKEN)

client.on_connect = connected
client.on_subscribe = subscribed
client.on_message = recv_message

# --- Kết nối với cơ chế thử lại và fallback IP ---
def resolve_broker():
    # Trả về địa chỉ IP có thể kết nối được, ưu tiên tên miền, fallback sang IP cứng.
    try:
        infos = socket.getaddrinfo(BROKER_ADDRESS, PORT)
        return infos[0][4][0]  # lấy IP đầu tiên
    except socket.gaierror:
        print(f"Không giải quyết được tên miền {BROKER_ADDRESS}, dùng IP fallback {BROKER_FALLBACK_IP}")
        return BROKER_FALLBACK_IP

def connect_with_retry(max_retries=5, delay=5):
    for attempt in range(1, max_retries + 1):
        try:
            host = resolve_broker()
            print(f"Lần thử {attempt}: kết nối đến {host}:{PORT} ...")
            client.connect(host, PORT)
            client.loop_start()
            return True
        except Exception as e:
            print(f"Lần thử {attempt} thất bại: {e}")
            if attempt < max_retries:
                time.sleep(delay)
    return False

if not connect_with_retry():
    print("Không thể kết nối đến broker sau các lần thử. Vui lòng kiểm tra internet và DNS.")
    # Tùy chọn: thoát chương trình hoặc tiếp tục chạy ở chế độ offline
    # raise SystemExit(1)

temp = 30
humi = 50
light_intesity = 100
counter = 0
while True:
    collect_data = {'temperature': temp, 'humidity': humi, 'light': light_intesity}
    temp += 1
    humi += 1
    light_intesity += 1
    client.publish('v1/devices/me/telemetry', json.dumps(collect_data), 1)
    time.sleep(5)
