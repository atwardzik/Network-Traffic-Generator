import socket
from urllib import response


PORT = 502 

with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
    s.bind(('0.0.0.0', PORT))
    s.listen()
    print(f"Test Modbus Device listening on port {PORT}...")
    
    while True:
        conn, addr = s.accept()
        with conn:
            print(f"Connected by {addr}")
            data = conn.recv(1024)
         
            if data:
                
                #response = bytes([0x00, 0x01, 0x00, 0x00, 0x00, 0x03, 0x01, 0x03, 0x01])

                #exception response

                response = bytes([0x00, 0x01, 0x00, 0x00, 0x00, 0x03, 0x01, 0x83, 0x01])
                
                conn.sendall(response)