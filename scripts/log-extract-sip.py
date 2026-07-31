#!/usr/bin/env python3
"""Extract SIP messages from GOnnect/pjsua logs into PCAP or plain ASCII.

Handles multiple log formats:
  - File dump: 2026-05-21 13:37:16.128 CEST DEBUG ...
  - Console:    [32m10:04:38.593[32m gonnect.pjsip: ...
  - Journalctl: Mai 26 16:00:26 host app[pid]: 16:00:26.071 gonnect.pjsip: ...
  - ISO date + local TZ: 2026-03-19 13:51:15.883 Mitteleuropäische Zeit ...

Supports IPv4 and IPv6 addresses.

Output formats (--format): pcap (default), ascii, or both.
With --anonymize, all accounts, IP addresses, DNS names, phone numbers and
auth secrets are consistently replaced with synthetic placeholders so the
result can be shared without leaking real data.
"""

import argparse
import datetime
import hashlib
import ipaddress
import os
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
    r"(\d{1,2}:\d{2}:\d{2})"      # time
    r"\.(\d+)"                    # fractional seconds
)

# pjsua log-line header
SIP_START_LOG_LINE_RE = re.compile(
    r".*?pjsua_core\.c\s+"
    r"\.{0,12}(TX|RX)\s+"
    r"\d+\s+bytes\s+"
    r"(?:Request\s+msg|Response\s+msg)\s+"
    r".+:\s*$"
)

LOG_LINE_RE = re.compile(
    r"(\d{1,2}:\d{2}:\d{2})"      # time
    r"\.(\d+)\s"                    # fractional seconds
    r"gonnect\."
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


def _rebuild_message(lines: list[str]) -> str:
    """Reassemble a SIP message preserving the header/body separator.

    The first blank line separates headers from an optional body (RFC 3261).
    Leading/trailing blank lines (e.g. the ones before "--end msg--") are
    dropped, but the single separator and the body are kept verbatim so the
    body length stays consistent with the Content-Length header.
    """
    lines = list(lines)
    while lines and not lines[0].strip():
        lines.pop(0)
    while lines and not lines[-1].strip():
        lines.pop()

    sep_idx = next((i for i, l in enumerate(lines) if not l.strip()), None)
    if sep_idx is None:
        headers, body = lines, []
    else:
        headers = lines[:sep_idx]
        body = lines[sep_idx + 1:]
        while body and not body[0].strip():
            body.pop(0)

    header_block = "\r\n".join(headers) + "\r\n"
    body_block = ("\r\n".join(body) + "\r\n") if body else ""
    return header_block + "\r\n" + body_block


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

            if in_message:
                if line.strip() == END_MARKER or LOG_LINE_RE.match(line):
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

                    raw_sip = _rebuild_message(current_lines)
                    results.append((raw_sip, current_ts_sec, current_ts_usec, current_direction, src_ip, src_port, dst_ip, dst_port))
                    current_lines = []
                else:
                    current_lines.append(line)

            if SIP_START_LOG_LINE_RE.match(line):
                in_message = True
                current_lines = []
                current_direction, current_peer_ip, current_peer_port = _parse_dir_ip(line)
                current_ts_sec, current_ts_usec = _parse_ts(line)
                continue



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


def write_ascii(messages: list[tuple[str, int, int, str, str, int, str, int]], out_path: str) -> None:
    """Write SIP messages as a plain human-readable ASCII transcript.

    Each message is preceded by a header line with sequence number,
    timestamp and the resolved src/dst endpoints.
    """
    with open(out_path, "w", encoding="utf-8") as f:
        for i, (msg, ts_sec, ts_usec, direction, src_ip, src_port, dst_ip, dst_port) in enumerate(
            messages, 1
        ):
            if ts_sec:
                dt = datetime.datetime.fromtimestamp(ts_sec)
                tstr = dt.strftime("%Y-%m-%d %H:%M:%S") + f".{ts_usec:06d}"
            else:
                tstr = "????-??-?? ??:??:??.??????"

            src = f"[{src_ip}]" if _is_ipv6(src_ip) else src_ip
            dst = f"[{dst_ip}]" if _is_ipv6(dst_ip) else dst_ip

            f.write(f"===== #{i}  {tstr}  {direction or '??'}  "
                    f"{src}:{src_port} -> {dst}:{dst_port} =====\n")
            f.write(msg.replace("\r\n", "\n").rstrip("\n"))
            f.write("\n\n")


# Combined matcher for the anonymizer. Alternatives are tried left-to-right at
# each position, and re.sub never re-scans replaced text, so each token is
# rewritten exactly once (no double-mapping of IPs).
_ANON_RE = re.compile(
    r"""
    (?P<secret>\b(?:nonce|cnonce|response|opaque|nextnonce)\s*=\s*"[^"]*") |
    (?P<authuser>\busername\s*=\s*"(?P<authuserval>[^"]*)") |
    (?P<realm>\brealm\s*=\s*"(?P<realmval>[^"]*)") |
    (?P<uri>\bsips?:(?:(?P<uriuser>[^@\s>;:"]+)@)?
        (?P<urihost>\[[0-9A-Fa-f:]+\]|[^@\s>;:"/]+)(?::(?P<uriport>\d+))?) |
    (?P<tel>\btel:(?P<telnum>\+?[0-9().\-]+)) |
    (?P<ipv6>\[(?P<ipv6val>[0-9A-Fa-f:]+)\]) |
    (?P<ipv4>\b(?P<ipv4val>\d{1,3}(?:\.\d{1,3}){3})\b)
    """,
    re.VERBOSE | re.IGNORECASE,
)


def _is_ip(s: str) -> bool:
    try:
        ipaddress.ip_address(s)
        return True
    except ValueError:
        return False


def _is_exempt_ip(s: str, keep_private: bool = False) -> bool:
    """Special-use addresses left untouched by the anonymizer.

    Loopback, unspecified (0.0.0.0 / ::), link-local and multicast carry no
    identity and/or have protocol meaning (e.g. "c=IN IP4 0.0.0.0" signals
    hold), so rewriting them would only obscure the analysis. RFC 1918 private
    ranges are mapped by default (in these logs both endpoints are private, so
    exempting them would leak the real internal addresses), unless keep_private
    is set.
    """
    try:
        obj = ipaddress.ip_address(s)
    except ValueError:
        return False
    if obj.is_loopback or obj.is_unspecified or obj.is_link_local or obj.is_multicast:
        return True
    return keep_private and obj.is_private


class Anonymizer:
    """Consistently replaces real PII in SIP messages with stable placeholders.

    The same real value always maps to the same synthetic one, and the IPs used
    in PCAP packet headers are mapped through the very same tables so they stay
    consistent with the IPs appearing inside the SIP payload.
    """

    def __init__(self, keep_private: bool = False) -> None:
        self._keep_private = keep_private
        self._ip: dict[str, str] = {}
        self._host: dict[str, str] = {}
        self._user: dict[str, str] = {}
        self._realm: dict[str, str] = {}

    @staticmethod
    def _scramble(value: str, salt: str) -> str:
        """Deterministic, length-preserving char-wise scramble.

        Letters map to random letters (case kept), digits to random digits,
        every other character (".", "+", "-", "@" ...) is left in place. The
        same input always yields the same output, so replacements stay
        consistent across messages while the length never changes.
        """
        rnd = hashlib.shake_256(f"{salt}\x00{value}".encode()).digest(len(value) or 1)
        out = []
        for ch, b in zip(value, rnd):
            if ch.islower():
                out.append(chr(ord("a") + b % 26))
            elif ch.isupper():
                out.append(chr(ord("A") + b % 26))
            elif ch.isdigit():
                out.append(chr(ord("0") + b % 10))
            else:
                out.append(ch)
        return "".join(out)

    def _fake_ipv4(self, ip: str) -> str:
        """Valid IPv4 of identical string length (digit count per octet kept)."""
        octets = ip.split(".")
        rnd = hashlib.shake_256(f"ip4\x00{ip}".encode()).digest(len(octets))
        out = []
        for octet, b in zip(octets, rnd):
            if len(octet) >= 3:
                out.append(str(100 + b % (256 - 100)))   # 100..255 -> 3 digits
            elif len(octet) == 2:
                out.append(str(10 + b % 90))              # 10..99  -> 2 digits
            else:
                out.append(str(b % 10))                   # 0..9    -> 1 digit
        return ".".join(out)

    def _fake_ipv6(self, ip: str) -> str:
        """IPv6 of identical length: hex digits scrambled, ':'/'::' kept."""
        rnd = hashlib.shake_256(f"ip6\x00{ip}".encode()).digest(len(ip) or 1)
        fake = "".join(
            "0123456789abcdef"[b % 16] if ch in "0123456789abcdefABCDEF" else ch
            for ch, b in zip(ip, rnd)
        )
        if _is_ip(fake):
            return fake
        # Rare (e.g. IPv4-mapped IPv6): fall back to a valid, non-leaking
        # address derived from the same seed. Length is not preserved here.
        packed = hashlib.shake_256(f"ip6f\x00{ip}".encode()).digest(16)
        return str(ipaddress.IPv6Address(int.from_bytes(packed, "big")))

    def map_ip(self, ip: str) -> str:
        if not ip or _is_exempt_ip(ip, self._keep_private):
            return ip
        if ip not in self._ip:
            self._ip[ip] = self._fake_ipv6(ip) if _is_ipv6(ip) else self._fake_ipv4(ip)
        return self._ip[ip]

    def map_host(self, host: str) -> str:
        if host not in self._host:
            self._host[host] = self._scramble(host, "host")
        return self._host[host]

    def map_user(self, user: str) -> str:
        if user not in self._user:
            self._user[user] = self._scramble(user, "user")
        return self._user[user]

    def map_realm(self, realm: str) -> str:
        if realm not in self._realm:
            self._realm[realm] = self._scramble(realm, "realm")
        return self._realm[realm]

    def _map_anyhost(self, host: str) -> str:
        """Map a host token that may be an IP (bracketed or bare) or a DNS name."""
        if host.startswith("[") and host.endswith("]"):
            inner = host[1:-1]
            return f"[{self.map_ip(inner)}]" if _is_ip(inner) else host
        if _is_ip(host):
            return self.map_ip(host)
        return self.map_host(host)

    def _replace(self, m: re.Match) -> str:
        if m.group("secret") is not None:
            return re.sub(
                r'"([^"]*)"',
                lambda mm: '"' + self._scramble(mm.group(1), "secret") + '"',
                m.group("secret"),
            )
        if m.group("authuser") is not None:
            val = m.group("authuserval")
            return m.group("authuser").replace(f'"{val}"', f'"{self.map_user(val)}"', 1)
        if m.group("realm") is not None:
            val = m.group("realmval")
            return m.group("realm").replace(f'"{val}"', f'"{self.map_realm(val)}"', 1)
        if m.group("uri") is not None:
            scheme = m.group("uri").split(":", 1)[0]
            user = m.group("uriuser")
            host = self._map_anyhost(m.group("urihost"))
            port = m.group("uriport")
            out = f"{scheme}:"
            if user is not None:
                out += f"{self.map_user(user)}@"
            out += host
            if port is not None:
                out += f":{port}"
            return out
        if m.group("tel") is not None:
            return f"tel:{self.map_user(m.group('telnum'))}"
        if m.group("ipv6") is not None:
            val = m.group("ipv6val")
            return f"[{self.map_ip(val)}]" if _is_ip(val) else m.group(0)
        if m.group("ipv4") is not None:
            val = m.group("ipv4val")
            return self.map_ip(val) if _is_ip(val) else m.group(0)
        return m.group(0)

    def anonymize_text(self, text: str) -> str:
        text = _ANON_RE.sub(self._replace, text)
        # Catch real DNS names that appeared inside a URI but recur elsewhere
        # (e.g. in Call-ID "...@host"), replacing longest names first.
        for real in sorted(self._host, key=len, reverse=True):
            text = text.replace(real, self._host[real])
        return text

    def anonymize_messages(
        self, messages: list[tuple[str, int, int, str, str, int, str, int]]
    ) -> list[tuple[str, int, int, str, str, int, str, int]]:
        out = []
        for msg, ts_sec, ts_usec, direction, src_ip, src_port, dst_ip, dst_port in messages:
            out.append((
                self.anonymize_text(msg), ts_sec, ts_usec, direction,
                self.map_ip(src_ip), src_port, self.map_ip(dst_ip), dst_port,
            ))
        return out


def main():
    parser = argparse.ArgumentParser(
        description="Extract SIP messages from GOnnect/pjsua logs into PCAP or ASCII."
    )
    parser.add_argument("log_file", help="GOnnect/pjsua log file to read")
    parser.add_argument(
        "output", nargs="?",
        help="output path (for 'both' used as base name); "
             "defaults to sip_messages.pcap/.txt",
    )
    parser.add_argument(
        "-f", "--format", choices=["pcap", "ascii", "both"], default="pcap",
        help="output format (default: pcap)",
    )
    parser.add_argument(
        "-a", "--anonymize", action="store_true",
        help="replace real accounts, IPs, DNS names, phone numbers and secrets",
    )
    parser.add_argument(
        "--keep-private", action="store_true",
        help="when anonymizing, leave RFC 1918 private addresses untouched "
             "(loopback/link-local/multicast/unspecified are always kept)",
    )
    args = parser.parse_args()

    if args.keep_private and not args.anonymize:
        parser.error("--keep-private has no effect without --anonymize")

    messages = extract_sip_messages(args.log_file)
    print(f"Extracted {len(messages)} SIP messages from {args.log_file}")

    if args.anonymize:
        messages = Anonymizer(keep_private=args.keep_private).anonymize_messages(messages)
        kept = "public IPs only" if args.keep_private else "all non-special IPs"
        print(f"Anonymized accounts, {kept}, DNS names, phone numbers and auth secrets")

    if args.format == "both":
        base = os.path.splitext(args.output)[0] if args.output else "sip_messages"
        pcap_out, ascii_out = base + ".pcap", base + ".txt"
    elif args.format == "pcap":
        pcap_out, ascii_out = args.output or "sip_messages.pcap", None
    else:
        pcap_out, ascii_out = None, args.output or "sip_messages.txt"

    if pcap_out:
        write_pcap(messages, pcap_out)
        print(f"Wrote {pcap_out} — open in Wireshark to analyse")
    if ascii_out:
        write_ascii(messages, ascii_out)
        print(f"Wrote {ascii_out} — plain ASCII transcript")


if __name__ == "__main__":
    main()
