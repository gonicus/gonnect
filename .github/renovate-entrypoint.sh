#!/bin/bash

apt update
apt install -y python3 python3-venv python3-pip

python3 -m venv .venv
source .venv/bin/activate
pip install -r /conan/requirements.txt

runuser -u ubuntu renovate