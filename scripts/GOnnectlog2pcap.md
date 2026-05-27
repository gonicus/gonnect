# GOnnectlog2pcap.py

Extract SIP messages from GOnnect/pjsua log files and write them to a PCAP file for analysis in Wireshark or tshark.

## Features

- Handles multiple log formats:
  - File dump (ISO date + timezone)
  - Console output (ANSI escape codes)
  - journalctl dumps (locale-agnostic)
  - Mixed timezone formats
- Supports IPv4 and IPv6 addresses
- Real timestamps extracted from log entries
- PCAP output with LINK_TYPE 228 (Wireshark auto-detects IPv4/IPv6)

## Usage

```bash
python3 GOnnectlog2pcap.py <log_file> [output.pcap]
```

- `log_file` — path to the GOnnect/pjsua log file
- `output.pcap` — optional, defaults to `sip_messages.pcap`

## Examples

```bash
# Extract from a journalctl log
python3 GOnnectlog2pcap.py gonnect-journal.log

# Extract with custom output filename
python3 GOnnectlog2pcap.py gonnect-journal.log capture.pcap

# Inspect with tshark
tshark -r sip_messages.pcap -Y sip
```

## Supported Log Formats

| Format | Example |
|--------|---------|
| File dump | `2026-05-21 13:37:16.128 CEST DEBUG ...` |
| Console | `[32m10:04:38.593[32m gonnect.pjsip: ...` |
| journalctl | `Mai 26 16:00:26 host app[pid]: 16:00:26.071 ...` |
| ISO + TZ | `2026-03-19 13:51:15.883 Mitteleuropäische Zeit ...` |

## Requirements

- Python 3.10+
- No external dependencies (stdlib only)
