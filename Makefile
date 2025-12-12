CXX := g++
MKDIR := mkdir -p
RM := rm -rf

BUILD_DIR := build
DOCS_DIR := docs/html
SRC_DIR := src
INCLUDE_DIR := include

.PHONY: all install build run docs clean test

all: build

install:
	sudo apt update
	sudo apt install -y \
		build-essential \
		cmake \
		qt6-base-dev \
		qt6-httpserver-dev \
		qt6-tools-dev \
		qt6-l10n-tools \
		libgl1-mesa-dev \
		libglu1-mesa-dev \
		mesa-common-dev \
		freeglut3-dev \
		libopengl0 \
		python3 \
		python3-pip \
		doxygen \
		graphviz
	pip3 install flask

build:
	@$(MKDIR) $(BUILD_DIR)
	cd $(BUILD_DIR) && \
	cmake .. && \
	make

run: build
	./$(BUILD_DIR)/MINM

docs:
	doxygen Doxyfile

test:
	python3 test_server/main.py

clean:
	$(RM) $(BUILD_DIR)
	$(RM) $(DOCS_DIR)

rebuild: clean build

rerun: clean run

help:
	@echo "Available targets:"
	@echo "  install - Install dependencies (apt packages + flask)"
	@echo "  build   - Build project in build directory"
	@echo "  run     - Build and run project"
	@echo "  docs    - Generate documentation via Doxygen"
	@echo "  test    - Run test server (test_server.py)"
	@echo "  clean   - Remove build and docs/html directories"
	@echo "  rebuild - Full project rebuild"
	@echo "  rerun   - Full rebuild and run"
	@echo "  help    - Show this help"
