#!/usr/bin/env bash
# tools/setup_venv.sh
#
# Creates a Python virtual environment used ONLY by the optional
# dataset generator (tools/dsa_dataset.py). The C solver itself
# (src/, Makefile) has no Python dependency at all -- this venv is
# purely a convenience for regenerating data/*.csv with different
# parameters (class sizes, honors probability, random seed, etc).
#
# Usage (from the project root):
#   bash tools/setup_venv.sh
#   source .venv/bin/activate
#   python tools/dsa_dataset.py
#   deactivate

set -euo pipefail

cd "$(dirname "$0")/.."   # move to project root regardless of cwd

if [ ! -d ".venv" ]; then
    echo "Creating virtual environment in .venv/ ..."
    python3 -m venv .venv
else
    echo ".venv already exists, reusing it."
fi

echo "Installing dependencies from tools/requirements.txt ..."
.venv/bin/pip install --upgrade pip
.venv/bin/pip install -r tools/requirements.txt

echo
echo "Done. Activate with:  source .venv/bin/activate"
echo "Then run:             python tools/dsa_dataset.py"
