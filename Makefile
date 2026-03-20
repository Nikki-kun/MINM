CXX := g++
MKDIR := mkdir -p
RM := rm -rf

BUILD_DIR := build
DOCS_DIR := docs/html
SRC_DIR := src
INCLUDE_DIR := include

MINM_DB_ENABLED ?= 1
MINM_DB_HOST ?= 127.0.0.1
MINM_DB_PORT ?= 3306
MINM_DB_NAME ?= minm_test
MINM_DB_USER ?= minm
MINM_DB_PASSWORD ?= minm_password

.PHONY: all install build run docs clean test test-server test-chats

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
		libqt6sql6-mysql \
		mysql-server \
		mysql-client \
		libgl1-mesa-dev \
		libglu1-mesa-dev \
		mesa-common-dev \
		freeglut3-dev \
		libopengl0 \
		python3 \
		python3-pip \
		doxygen \
		graphviz
	@echo "MySQL installation finished. Starting service (if supported)..."
	-(sudo service mysql start || true)
	-(sudo systemctl start mysql || true)

build:
	@$(MKDIR) $(BUILD_DIR)
	cd $(BUILD_DIR) && \
	cmake .. && \
	make

run: build
	MINM_DB_ENABLED=$(MINM_DB_ENABLED) \
	MINM_DB_HOST=$(MINM_DB_HOST) \
	MINM_DB_PORT=$(MINM_DB_PORT) \
	MINM_DB_NAME=$(MINM_DB_NAME) \
	MINM_DB_USER=$(MINM_DB_USER) \
	MINM_DB_PASSWORD=$(MINM_DB_PASSWORD) \
	./$(BUILD_DIR)/MINM

docs:
	doxygen Doxyfile

test-server: build
	./tests/test_server.sh

test: build
	./tests/test_chats_scenario.sh

clean:
	$(RM) $(BUILD_DIR)
	$(RM) $(DOCS_DIR)
	-(sudo mysql --protocol=socket -uroot -e "DROP DATABASE IF EXISTS \`$(MINM_DB_NAME)\`;")
	-(sudo mysql --protocol=socket -uroot -e "DROP USER IF EXISTS '$(MINM_DB_USER)'@'%'; FLUSH PRIVILEGES;")

rebuild: clean build

rerun: clean run

help:
	@echo "Available targets:"
	@echo "  install - Install dependencies and MySQL"
	@echo "  build   - Build project in build directory"
	@echo "  run     - Build and run project"
	@echo "           (by default connects to MySQL with MINM_DB_* vars)"
	@echo "  docs    - Generate documentation via Doxygen"
	@echo "  test    - Run test server (test_server.py)"
	@echo "  test-server - Run HTTP server integration tests (tests/test_server.sh)"
	@echo "  test-chats  - Run chats scenario tests (tests/test_chats_scenario.sh)"
	@echo "  clean   - Remove build/docs and drop MySQL test DB/user"
	@echo "  rebuild - Full project rebuild"
	@echo "  rerun   - Full rebuild and run"
	@echo "  help    - Show this help"
