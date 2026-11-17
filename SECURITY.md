# Security Policy

## Supported versions

Security fixes are provided for the latest stable release. Pre-releases
(beta) receive fixes as part of the next pre-release.

| Version            | Supported |
| ------------------ | --------- |
| latest stable 2.x  | ✅        |
| older releases     | ❌        |

Please update to the latest release before reporting an issue.

## Reporting a vulnerability

**Please do not report security vulnerabilities through public GitHub issues,
discussions or pull requests.**

Report them privately using this channel:

- **GitHub:** [Report a vulnerability](https://github.com/gonicus/gonnect/security/advisories/new)
  (private security advisory)

Please include as much of the following as possible:

- affected GOnnect version and platform (Linux Flatpak, Windows installer, …)
- type of issue (e.g. credential leak, TLS/SRTP downgrade, remote code
  execution via SIP or chat messages)
- steps to reproduce, ideally with a minimal configuration
- proof of concept or logs, with secrets removed
- impact as you understand it

You can write in German or English.

## What to expect

- We acknowledge your report within five working days.
- We keep you informed about the progress of the fix.
- We coordinate the disclosure date with you and publish a GitHub security
  advisory once a fixed release is available.
- If you wish, we credit you in the advisory.

## Scope

In scope is the GOnnect application in this repository, including its
packaging (Flatpak, Windows installer) and the bundled plugins.

Vulnerabilities in third-party components (e.g. Qt, PJSIP, Jitsi Meet, a
Matrix homeserver or your PBX) should be reported to the respective project.
If such a vulnerability affects GOnnect, please let us know as well, so we can
update the dependency.
