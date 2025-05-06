# Project settings
PROJECT_NAME = LoraReceiver
SKETCH = $(PROJECT_NAME).ino

# Arduino CLI settings
ARDUINO_CLI = arduino-cli
FQBN = esp32:esp32:esp32doit-devkit-v1 # fully qualified board name
PORT = COM3

# Directories
SRC_DIR = .
BUILD_DIR = build

# Targets
all: compile

compile:
	$(ARDUINO_CLI) compile --fqbn $(FQBN) --build-path $(BUILD_DIR) $(SRC_DIR)/$(SKETCH)

upload: compile
	$(ARDUINO_CLI) upload --fqbn $(FQBN) --port $(PORT) --input-dir $(BUILD_DIR)

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all compile upload clean