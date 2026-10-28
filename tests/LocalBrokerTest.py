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
            return header, read_exact(conn, length)
    raise ValueError("Invalid MQTT remaining length")


def send_packet(conn, header, body):
    length = len(body)
    encoded = bytearray([header])
    while True:
        value = length % 128
        length //= 128
        encoded.append(value | (128 if length else 0))
        if not length:
            break
    conn.sendall(encoded + body)


def skip_properties(body, offset):
    length = 0
    for shift in range(0, 28, 7):
        value = body[offset]
        offset += 1
        length |= (value & 127) << shift
        if not value & 128:
            return offset + length
    raise ValueError("Invalid property length")


def send_report(conn, topic, payload):
    # A QoS 0 message reports what actually arrived on the wire. The native API
    # test checks this through the production message callback/dispatcher.
    body = len(topic).to_bytes(2, "big") + topic + b"\x00" + payload
    send_packet(conn, 0x30, body)


errors = []
workers = []
stopping = threading.Event()


def serve_client(conn):
    with conn:
        conn.settimeout(8)
        try:
            header, _ = read_packet(conn)
            if header >> 4 != 1:
                raise ValueError("Expected CONNECT")
            conn.sendall(bytes.fromhex("20 03 00 00 00"))  # Successful v5 CONNACK.
            while True:
                header, body = read_packet(conn)
                packet = header >> 4
                if packet == 14:  # DISCONNECT has no acknowledgement in MQTT.
                    return
                if packet == 12:
                    conn.sendall(bytes.fromhex("d0 00"))
                elif packet == 8:  # SUBSCRIBE: one filter per native call.
                    offset = skip_properties(body, 2)
                    size = int.from_bytes(body[offset:offset + 2], "big")
                    topic = body[offset + 2:offset + 2 + size]
                    qos = body[offset + 2 + size] & 3
                    if qos > 2:
                        raise ValueError("Invalid subscription QoS")
                    send_packet(conn, 0x90, body[:2] + bytes([0, qos]))
                    send_report(conn, topic, f"subscribe:{qos}".encode())
                elif packet == 3:  # PUBLISH, including the full QoS 2 handshake.
                    size = int.from_bytes(body[:2], "big")
                    topic = body[2:2 + size]
                    qos = (header >> 1) & 3
                    offset = 2 + size
                    if qos:
                        packet_id = body[offset:offset + 2]
                        offset += 2
                        if qos not in (1, 2):
                            raise ValueError("Invalid publish QoS")
                        send_packet(conn, 0x40 if qos == 1 else 0x50, packet_id)
                    offset = skip_properties(body, offset)
                    report = f"publish:{qos}:{header & 1}:".encode() + body[offset:]
                    send_report(conn, topic, report)
                elif packet == 6:  # PUBREL -> PUBCOMP.
                    send_packet(conn, 0x70, body[:2])
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
