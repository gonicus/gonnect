#!/usr/bin/env python3
"""Extract SIP messages from GOnnect/pjsua logs into a PCAP file for Wireshark.

Handles multiple log formats:
  - File dump: 2026-05-21 13:37:16.128 CEST DEBUG ...
  - Console:    [32m10:04:38.593[32m gonnect.pjsip: ...
  - Journalctl: Mai 26 16:00:26 host app[pid]: 16:00:26.071 gonnect.pjsip: ...
  - ISO date + local TZ: 2026-03-19 13:51:15.883 Mitteleuropäische Zeit ...

Supports IPv4 and IPv6 addresses.
"""

import datetime
import ipaddress
import re
import struct
import sys

# ANSI escape codes
ANSI_RE = re.compile(r"\x1b\[[0-9;]*[a-zA-Z]")

# journalctl prefix: <Month> <Day> <Time> <Hostname> <Process>[<PID>]:
# Locale-agnostic: matches any word for month, any digits for day/time/etc.
JOURNAL_RE = re.compile(
    r"^.*\[\d+\]: "
)

# Embedded timestamp: HH:MM:SS.fff (always present in pjsua log lines)
TS_RE = re.compile(
    r"(\d{2}:\d{2}:\d{2})"      # time
    r"\.(\d+)"                    # fractional seconds
)

# pjsua log-line header
LOG_LINE_RE = re.compile(
    r".*?pjsua_core\.c\s+"
    r"\.{0,12}(TX|RX)\s+"
    r"\d+\s+bytes\s+"
    r"(?:Request\s+msg|Response\s+msg)\s+"
    r".+:\s*$"
)

# Full date + time at the start of a log line
# Format 1: 2026-05-21 13:37:16.128 CEST
# Format 2: 2026-03-19 13:51:15.883 Mitteleuropäische Zeit
FULL_TS_RE = re.compile(
    r"(\d{4}-\d{2}-\d{2})\s+"
    r"(\d{2}:\d{2}:\d{2})"
    r"\.(\d+)"
)

END_MARKER = "--end msg--"

# Via header: v: or Via:
VIA_HOST_RE = re.compile(
    r"(?i)^(?:v|via)\s*:\s*SIP/2\.0/\S+\s+"
    r"(?:\[([^\]]+)\]|(\d+\.\d+\.\d+\.\d+))"
    r":(\d+)"
)

# Peer IP from log header
DIR_IP_RE = re.compile(
    r"(TX|RX).*?(?:to|from)\s+(?:UDP|TCP|TLS)\s+"
    r"(?:\[([^\]]+)\]|(\d+\.\d+\.\d+\.\d+))"
    r":(\d+)"
)


def _clean_line(line: str) -> str:
    """Strip ANSI codes and journalctl prefix from a log line."""
    line = ANSI_RE.sub("", line)
    line = JOURNAL_RE.sub("", line)
    return line


def _parse_ts(line: str) -> tuple[int, int]:
    """Parse timestamp from a log header line.

    Returns (epoch_seconds, microseconds) or (0, 0) if no timestamp found.
    """
    # Try full date+time first (ISO format)
    m = FULL_TS_RE.search(line)
    if m:
        date_str = m.group(1)
        time_str = m.group(2)
        frac_str = m.group(3)
        frac_str = frac_str.ljust(6, "0")[:6]
        usec = int(frac_str)
        dt = datetime.datetime.strptime(f"{date_str} {time_str}", "%Y-%m-%d %H:%M:%S")
        return int(dt.timestamp()), usec

    # Fall back to embedded HH:MM:SS.fff only
    m = TS_RE.search(line)
    if m:
        time_str = m.group(1)
        frac_str = m.group(2)
        frac_str = frac_str.ljust(6, "0")[:6]
        usec = int(frac_str)
        parts = time_str.split(":")
        h, mi, s = int(parts[0]), int(parts[1]), int(parts[2])
        # Use current year since journalctl/time-only logs don't include a year
        dt = datetime.datetime(
            datetime.datetime.now().year, 1, 1, h, mi, s
        )
        return int(dt.timestamp()), usec

    return 0, 0


def _parse_via_host(lines: list[str]) -> tuple[str, int]:
    """Extract the local IP and port from the first Via header."""
    for line in lines:
        m = VIA_HOST_RE.match(line)
        if m:
            ip = m.group(1) or m.group(2)
            return ip, int(m.group(3))
    return "127.0.0.1", 5060


def _parse_dir_ip(line: str) -> tuple[str, str, int]:
    """Extract direction and peer IP:port from the log header line."""
    m = DIR_IP_RE.search(line)
    if m:
        direction = "TX" if m.group(1) == "TX" else "RX"
        ip = m.group(2) or m.group(3)
        return direction, ip, int(m.group(4))
    return "", "", 0


def extract_sip_messages(log_path: str) -> list[tuple[str, int, int, str, str, int, str, int]]:
    """Parse the log file and return a list of
    (sip_message, ts_sec, ts_usec, direction, src_ip, src_port, dst_ip, dst_port)."""
    results: list[tuple[str, int, int, str, str, int, str, int]] = []
    current_lines: list[str] = []
    current_direction = ""
    current_peer_ip = ""
    current_peer_port = 0
    current_ts_sec = 0
    current_ts_usec = 0
    known_local_ip = "127.0.0.1"
    known_local_port = 5060
    in_message = False

    with open(log_path, "r", encoding="utf-8", errors="replace") as f:
        for raw_line in f:
            line = raw_line.rstrip("\r\n")
            line = _clean_line(line)

            if LOG_LINE_RE.match(line):
                in_message = True
                current_lines = []
                current_direction, current_peer_ip, current_peer_port = _parse_dir_ip(line)
                current_ts_sec, current_ts_usec = _parse_ts(line)
                continue

            if in_message:
                if line.strip() == END_MARKER:
                    in_message = False

                    via_ip, via_port = _parse_via_host(current_lines)

                    if via_ip != current_peer_ip:
                        local_ip, local_port = via_ip, via_port
                        known_local_ip, known_local_port = local_ip, local_port
                    else:
                        local_ip, local_port = known_local_ip, known_local_port

                    if current_direction == "TX":
                        src_ip, src_port, dst_ip, dst_port = (
                            local_ip, local_port, current_peer_ip, current_peer_port
                        )
                    else:
                        src_ip, src_port, dst_ip, dst_port = (
                            current_peer_ip, current_peer_port, local_ip, local_port
                        )

                    current_lines = [l for l in current_lines if l.strip()]
                    raw_sip = "\r\n".join(current_lines) + "\r\n\r\n"
                    results.append((raw_sip, current_ts_sec, current_ts_usec, current_direction, src_ip, src_port, dst_ip, dst_port))
                    current_lines = []
                else:
                    current_lines.append(line)

    return results


def _is_ipv6(ip_str: str) -> bool:
    """Quick check whether an address string is IPv6."""
    return ":" in ip_str


def _write_pcap_ipv4_packet(sip_data: bytes, src_ip: str, dst_ip: str, src_port: int, dst_port: int) -> bytes:
    """Build a complete IPv4/UDP/SIP packet."""
    udp_len = 8 + len(sip_data)
    ip_total_len = 20 + udp_len

    udp_header = struct.pack("!HHHH", src_port, dst_port, udp_len, 0)

    ip_header = struct.pack("!BBHHHBBH4s4s",
        0x45, 0, ip_total_len, 1024, 0x4000, 64, 17, 0,
        bytes(int(o) for o in src_ip.split(".")),
        bytes(int(o) for o in dst_ip.split("."))
    )

    return ip_header + udp_header + sip_data


def _write_pcap_ipv6_packet(sip_data: bytes, src_ip: str, dst_ip: str, src_port: int, dst_port: int) -> bytes:
    """Build a complete IPv6/UDP/SIP packet."""
    udp_len = 8 + len(sip_data)

    udp_header = struct.pack("!HHHH", src_port, dst_port, udp_len, 0)

    src_bytes = ipaddress.ip_address(src_ip).packed
    dst_bytes = ipaddress.ip_address(dst_ip).packed

    ip_header = struct.pack("!I", 0x60000000)
    ip_header += struct.pack("!H", udp_len)
    ip_header += struct.pack("!BB", 17, 64)
    ip_header += src_bytes + dst_bytes

    return ip_header + udp_header + sip_data


def write_pcap(messages: list[tuple[str, int, int, str, str, int, str, int]], out_path: str) -> None:
    """Write SIP messages as a PCAP file for Wireshark.

    Uses LINK_TYPE 228 (Raw IPv4/IPv6) which lets Wireshark auto-detect
    the IP version from the first byte of each packet.
    """
    with open(out_path, "wb") as f:
        f.write(struct.pack("<IHHiIII",
            0xA1B2C3D4, 2, 4, 0, 0, 65535, 228
        ))

        for msg, ts_sec, ts_usec, direction, src_ip, src_port, dst_ip, dst_port in messages:
            sip_data = msg.encode("utf-8")

            if _is_ipv6(src_ip) or _is_ipv6(dst_ip):
                packet = _write_pcap_ipv6_packet(sip_data, src_ip, dst_ip, src_port, dst_port)
            else:
                packet = _write_pcap_ipv4_packet(sip_data, src_ip, dst_ip, src_port, dst_port)

            cap_len = len(packet)
            f.write(struct.pack("<IIII", ts_sec, ts_usec, cap_len, cap_len))
            f.write(packet)


def main():
    if len(sys.argv) < 2:
        print(f"Usage: {sys.argv[0]} <log_file> [output_file]")
        print("  output_file defaults to sip_messages.pcap")
        sys.exit(1)

    log_file = sys.argv[1]
    out_file = sys.argv[2] if len(sys.argv) > 2 else "sip_messages.pcap"

    messages = extract_sip_messages(log_file)
    print(f"Extracted {len(messages)} SIP messages from {log_file}")

    write_pcap(messages, out_file)
    print(f"Wrote {out_file} — open in Wireshark to analyse")


if __name__ == "__main__":
    main()
