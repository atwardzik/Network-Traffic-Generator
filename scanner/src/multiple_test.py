import socket
import threading

def handle_client(conn, addr, device_id):
    try:
        data = conn.recv(1024)
        if not data:
            return
        
        print(f"[{addr[0]}] -> Device {device_id} RECEIVED: {data.hex(' ')}")

        # Transaction ID is the first 2 bytes of the request
        trans_id = data[0:2]
        
        if device_id <= 7:
            # Response: MODBUS OK (Function 03)
            response = trans_id + bytes([0x00, 0x00, 0x00, 0x03, 0x01, 0x03, 0x01])
        elif device_id <= 9:
            # Response: MODBUS EX (Exception 83)
            response = trans_id + bytes([0x00, 0x00, 0x00, 0x03, 0x01, 0x83, 0x01])
        else:
            # Response: Garbage data (Not Modbus)
            response = b"NOT_MODBUS_DATA"

        conn.sendall(response)
    except Exception as e:
        print(f"Error on device {device_id}: {e}")
    finally:
        conn.close()

def start_device(ip, device_id):
    try:
        # Create socket and bind to Port 502
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
            s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            s.bind((ip, 502))
            s.listen()
            print(f"Device {device_id} active on {ip}:502")
            while True:
                conn, addr = s.accept()
                threading.Thread(target=handle_client, args=(conn, addr, device_id)).start()
    except PermissionError:
        print(f"Error: Run with sudo to bind to port 502 on {ip}")
    except Exception as e:
        print(f"Could not start {ip}: {e}")

if __name__ == "__main__":
    threads = []
    # Create 10 devices on the 127.0.0.x
    for i in range(1, 11):
        ip = f"127.0.0.{i}"
        t = threading.Thread(target=start_device, args=(ip, i))
        t.daemon = True
        t.start()
        threads.append(t)

    print("\nSimulator running.\n")
    try:
        for t in threads: t.join()
    except KeyboardInterrupt:
        print("\nStopping simulator.")