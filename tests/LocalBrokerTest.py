"""Run an MQTT test against a minimal MQTT 5 peer on an ephemeral loopback port."""
import socket
import subprocess
import sys
import threading


def read_exact(conn, size):
    data = bytearray()
    while len(data) < size:
        chunk = conn.recv(size - len(data))
        if not chunk:
            raise EOFError()
        data.extend(chunk)
    return data


def read_packet(conn):
    header = read_exact(conn, 1)[0]
    length = 0
    for shift in range(0, 28, 7):
        value = read_exact(conn, 1)[0]
        length |= (value & 127) << shift
        if not value & 128:
            return header >> 4, read_exact(conn, length)
    raise ValueError("Invalid MQTT remaining length")


errors = []
workers = []
stopping = threading.Event()


def serve_client(conn):
    with conn:
        conn.settimeout(8)
        try:
            packet, _ = read_packet(conn)
            if packet != 1:
                raise ValueError("Expected CONNECT")
            conn.sendall(bytes.fromhex("20 03 00 00 00"))  # Successful v5 CONNACK.
            while True:
                packet, _ = read_packet(conn)
                if packet == 14:  # DISCONNECT has no acknowledgement in MQTT.
                    return
                if packet == 12:
                    conn.sendall(bytes.fromhex("d0 00"))
                else:
                    raise ValueError(f"Unexpected MQTT packet {packet}")
        except (EOFError, ConnectionResetError):
            pass  # Immediate client teardown may close before DISCONNECT.
        except Exception as error:
            errors.append(repr(error))


with socket.socket() as server:
    server.bind(("127.0.0.1", 0))
    server.listen(16)
    server.settimeout(0.1)

    def accept_clients():
        while not stopping.is_set():
            try:
                conn, _ = server.accept()
            except socket.timeout:
                continue
            worker = threading.Thread(target=serve_client, args=(conn,))
            workers.append(worker)
            worker.start()

    acceptor = threading.Thread(target=accept_clients)
    acceptor.start()
    try:
        result = subprocess.run(
            [sys.argv[1], f"tcp://127.0.0.1:{server.getsockname()[1]}"], timeout=45
        )
    finally:
        stopping.set()
        acceptor.join()
        for worker in workers:
            worker.join()

if errors:
    raise RuntimeError(errors)
sys.exit(result.returncode)
