# SPDX-License-Identifier: GPL-3.0-or-later

VENV := .venv
PYTHON := $(VENV)/bin/python
PIP := $(PYTHON) -m pip
ESPHOME := $(PYTHON) -m esphome
CONFIG := tests/esphome/geekmagic-smalltv-ultra.yaml
STAMP := $(VENV)/.installed

.PHONY: setup test native-test preview esphome-config esphome-compile clean

setup: $(STAMP)

$(STAMP): requirements.txt
	python3 -m venv $(VENV)
	$(PIP) install --upgrade pip
	$(PIP) install -r requirements.txt
	@touch $(STAMP)

test: $(STAMP)
	$(PYTHON) -m pytest

native-test: test

preview:
	python3 tools/render_previews.py

esphome-config: $(STAMP)
	$(ESPHOME) config $(CONFIG)

esphome-compile: $(STAMP)
	$(ESPHOME) compile $(CONFIG)

clean:
	rm -rf .pytest_cache .venv tests/esphome/.esphome
