#!/usr/bin/env python3
import argparse
import struct
import sys
import time
from dataclasses import dataclass
from typing import Optional

import serial


DEFAULT_SERIAL_PATH = "/dev/serial0"
DEFAULT_BAUDRATE = 230400
DEFAULT_SERIAL_TIMEOUT_S = 0.2

MAVLINK_STX_V1 = 0xFE
MAVLINK_STX_V2 = 0xFD
MAVLINK_MAX_PAYLOAD_LEN = 255

MAVLINK_MSG_ID_HEARTBEAT = 0
MAVLINK_MSG_ID_GPS_RAW_INT = 24
MAVLINK_MSG_ID_ATTITUDE = 30
MAVLINK_MSG_ID_LOCAL_POSITION_NED = 32

MAVLINK_CRC_EXTRA = {
    MAVLINK_MSG_ID_HEARTBEAT: 50,
    MAVLINK_MSG_ID_GPS_RAW_INT: 24,
    MAVLINK_MSG_ID_ATTITUDE: 39,
    MAVLINK_MSG_ID_LOCAL_POSITION_NED: 185,
}


def crc_accumulate(data: int, crc: int) -> int:
    tmp = data ^ (crc & 0xFF)
    tmp ^= (tmp << 4) & 0xFF
    return ((crc >> 8) ^ (tmp << 8) ^ (tmp << 3) ^ (tmp >> 4)) & 0xFFFF


def compute_mavlink_v2_crc(frame_without_stx: bytes, crc_extra: int) -> int:
    crc = 0xFFFF
    for byte in frame_without_stx:
        crc = crc_accumulate(byte, crc)
    crc = crc_accumulate(crc_extra, crc)
    return crc


@dataclass
class ParsedFrame:
    msgid: int
    payload: bytes
    seq: int
    sysid: int
    compid: int


class MavlinkV2Parser:
    def __init__(self) -> None:
        self.buffer = bytearray()

    def feed(self, data: bytes) -> list[ParsedFrame]:
        frames: list[ParsedFrame] = []
        self.buffer.extend(data)

        while True:
            stx_index = self.buffer.find(bytes([MAVLINK_STX_V2]))
            if stx_index < 0:
                self.buffer.clear()
                break

            if stx_index > 0:
                del self.buffer[:stx_index]

            if len(self.buffer) < 12:
                break

            payload_len = self.buffer[1]
            frame_len = 12 + payload_len
            if len(self.buffer) < frame_len:
                break

            frame = bytes(self.buffer[:frame_len])
            frame_wo_stx = frame[1:-2]
            msgid = frame[7] | (frame[8] << 8) | (frame[9] << 16)
            recv_crc = frame[-2] | (frame[-1] << 8)
            crc_extra = MAVLINK_CRC_EXTRA.get(msgid)

            if crc_extra is not None:
                calc_crc = compute_mavlink_v2_crc(frame_wo_stx, crc_extra)
                if calc_crc == recv_crc:
                    frames.append(
                        ParsedFrame(
                            msgid=msgid,
                            payload=frame[10:-2],
                            seq=frame[4],
                            sysid=frame[5],
                            compid=frame[6],
                        )
                    )

            del self.buffer[:frame_len]

        return frames


def read_u32_le(data: bytes, offset: int) -> int:
    return struct.unpack_from("<I", data, offset)[0]


def read_i32_le(data: bytes, offset: int) -> int:
    return struct.unpack_from("<i", data, offset)[0]


def read_u64_le(data: bytes, offset: int) -> int:
    return struct.unpack_from("<Q", data, offset)[0]


def read_f32_le(data: bytes, offset: int) -> float:
    return struct.unpack_from("<f", data, offset)[0]


def describe_frame(frame: ParsedFrame) -> str:
    if frame.msgid == MAVLINK_MSG_ID_HEARTBEAT and len(frame.payload) >= 9:
        custom_mode = read_u32_le(frame.payload, 0)
        base_mode = frame.payload[6]
        system_status = frame.payload[7]
        return (
            f"HEARTBEAT seq={frame.seq} sys={frame.sysid} comp={frame.compid} "
            f"custom_mode={custom_mode} base_mode={base_mode} system_status={system_status}"
        )

    if frame.msgid == MAVLINK_MSG_ID_ATTITUDE and len(frame.payload) >= 28:
        time_boot_ms = read_u32_le(frame.payload, 0)
        roll = read_f32_le(frame.payload, 4)
        pitch = read_f32_le(frame.payload, 8)
        yaw = read_f32_le(frame.payload, 12)
        return (
            f"ATTITUDE seq={frame.seq} boot_ms={time_boot_ms} "
            f"roll={roll:.3f} pitch={pitch:.3f} yaw={yaw:.3f}"
        )

    if frame.msgid == MAVLINK_MSG_ID_LOCAL_POSITION_NED and len(frame.payload) >= 28:
        time_boot_ms = read_u32_le(frame.payload, 0)
        x = read_f32_le(frame.payload, 4)
        y = read_f32_le(frame.payload, 8)
        z = read_f32_le(frame.payload, 12)
        vx = read_f32_le(frame.payload, 16)
        vy = read_f32_le(frame.payload, 20)
        vz = read_f32_le(frame.payload, 24)
        return (
            f"LOCAL_POSITION_NED seq={frame.seq} boot_ms={time_boot_ms} "
            f"x={x:.3f} y={y:.3f} z={z:.3f} vx={vx:.3f} vy={vy:.3f} vz={vz:.3f}"
        )

    if frame.msgid == MAVLINK_MSG_ID_GPS_RAW_INT and len(frame.payload) >= 30:
        time_usec = read_u64_le(frame.payload, 0)
        lat = read_i32_le(frame.payload, 8)
        lon = read_i32_le(frame.payload, 12)
        alt_mm = read_i32_le(frame.payload, 16)
        fix_type = frame.payload[28]
        sats = frame.payload[29]
        return (
            f"GPS_RAW_INT seq={frame.seq} time_usec={time_usec} "
            f"fix={fix_type} sats={sats} lat={lat} lon={lon} alt_mm={alt_mm}"
        )

    return f"MSG msgid={frame.msgid} seq={frame.seq} payload_len={len(frame.payload)}"


class Bridge:
    def __init__(self, serial_path: str, baudrate: int, timeout_s: float) -> None:
        self.serial_path = serial_path
        self.baudrate = baudrate
        self.timeout_s = timeout_s
        self.serial_port: Optional[serial.Serial] = None
        self.parser = MavlinkV2Parser()

    def open_serial(self) -> None:
        self.serial_port = serial.Serial(
            self.serial_path,
            self.baudrate,
            timeout=self.timeout_s,
            bytesize=serial.EIGHTBITS,
            parity=serial.PARITY_NONE,
            stopbits=serial.STOPBITS_ONE,
            xonxoff=False,
            rtscts=False,
            dsrdtr=False,
        )
        self.serial_port.reset_input_buffer()
        self.serial_port.reset_output_buffer()
        print(f"serial open: {self.serial_path} baud={self.baudrate}", flush=True)

    def ensure_serial(self) -> None:
        while self.serial_port is None:
            try:
                self.open_serial()
            except serial.SerialException as exc:
                print(f"serial open failed: {exc}", flush=True)
                time.sleep(1.0)

    def run(self) -> int:
        print(
            f"starting mavlink uart bridge: serial={self.serial_path} baud={self.baudrate}",
            flush=True,
        )

        while True:
            self.ensure_serial()

            try:
                assert self.serial_port is not None
                data = self.serial_port.read(512)
                if not data:
                    continue

                preview = " ".join(f"{byte:02X}" for byte in data[:32])
                print(f"read {len(data)} bytes data={preview}", flush=True)

                frames = self.parser.feed(data)
                for frame in frames:
                    print(describe_frame(frame), flush=True)
            except serial.SerialException as exc:
                print(f"serial read failed: {exc}", flush=True)
                try:
                    assert self.serial_port is not None
                    self.serial_port.close()
                except Exception:
                    pass
                self.serial_port = None
                time.sleep(1.0)


def parse_args(argv: list[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("--serial-path", default=DEFAULT_SERIAL_PATH, help="serial path")
    parser.add_argument("--baudrate", type=int, default=DEFAULT_BAUDRATE, help="serial baudrate")
    parser.add_argument("--serial-timeout", type=float, default=DEFAULT_SERIAL_TIMEOUT_S, help="serial timeout seconds")
    return parser.parse_args(argv)


def main(argv: list[str]) -> int:
    args = parse_args(argv)
    bridge = Bridge(
        serial_path=args.serial_path,
        baudrate=args.baudrate,
        timeout_s=args.serial_timeout,
    )
    return bridge.run()


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
