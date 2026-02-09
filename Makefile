# Makefile for Integrated Smart Store System
# Combines fingerprint, face recognition, and product tracking

CC = gcc
CXX = g++
CFLAGS = -Wall -O2 -Iinclude -Iexternal
CXXFLAGS = -Wall -O2 -std=c++11 -Iinclude
LDFLAGS = -lm -lpthread -ldl -lcurl

# OpenCV flags
OPENCV_CFLAGS = $(shell pkg-config --cflags opencv4)
OPENCV_LIBS = $(shell pkg-config --libs opencv4)

# Directories
SRC_DIR = src
OBJ_DIR = obj
BIN_DIR = bin
EXTERNAL_DIR = external
PYTHON_DIR = python

# Source files
C_SOURCES = $(SRC_DIR)/serial.c $(SRC_DIR)/fingerprint.c $(SRC_DIR)/integrated_database.c
CPP_SOURCES_FACE = face_recognition_server.cpp
CPP_SOURCES_PRODUCT = product_recognition_server.cpp

# Object files
C_OBJECTS = $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(C_SOURCES))
SQLITE_OBJ = $(OBJ_DIR)/sqlite3.o

# Executables
REGISTRATION_BIN = $(BIN_DIR)/registration_system
FACE_SERVER_BIN = $(BIN_DIR)/face_recognition_server
PRODUCT_SERVER_BIN = $(BIN_DIR)/product_recognition_server

# Targets
.PHONY: all clean setup directories registration face_server product_server python_deps

all: setup directories registration face_server product_server

# Create necessary directories
directories:
	@mkdir -p $(OBJ_DIR) $(BIN_DIR) database ../images/clients ../images/produits ../images/temp ../images/temp_produit ../invoices

# Setup (download SQLite if needed)
setup:
	@if [ ! -f "$(EXTERNAL_DIR)/sqlite3.h" ]; then \
		echo "Downloading SQLite..."; \
		cd $(EXTERNAL_DIR) && \
		wget https://sqlite.org/2024/sqlite-amalgamation-3450100.zip -O sqlite.zip && \
		unzip -q sqlite.zip && \
		cp sqlite-amalgamation-3450100/*.h . && \
		cp sqlite-amalgamation-3450100/*.c sqlite3.c && \
		rm -rf sqlite.zip sqlite-amalgamation-3450100; \
	fi

# Compile SQLite
$(SQLITE_OBJ): $(EXTERNAL_DIR)/sqlite3.c
	@echo "Compiling SQLite..."
	@$(CC) -c -o $@ $< -DSQLITE_THREADSAFE=0 -DSQLITE_OMIT_LOAD_EXTENSION

# Compile C sources
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@echo "Compiling $<..."
	@$(CC) $(CFLAGS) -c -o $@ $<

# Compile main registration system
$(OBJ_DIR)/registration_system.o: registration_system.c
	@echo "Compiling registration system..."
	@$(CC) $(CFLAGS) -c -o $@ $<

# Link registration system
registration: $(C_OBJECTS) $(SQLITE_OBJ) $(OBJ_DIR)/registration_system.o
	@echo "Linking registration system..."
	@$(CC) -o $(REGISTRATION_BIN) $^ $(LDFLAGS)
	@echo "✓ Registration system built: $(REGISTRATION_BIN)"

# Face recognition server (C++ with OpenCV)
face_server: $(CPP_SOURCES_FACE)
	@echo "Compiling face recognition server..."
	@$(CXX) $(CXXFLAGS) $(OPENCV_CFLAGS) -o $(FACE_SERVER_BIN) $< $(OPENCV_LIBS) -lmongoose
	@echo "✓ Face recognition server built: $(FACE_SERVER_BIN)"

# Product recognition server (C++ with OpenCV)
product_server: $(CPP_SOURCES_PRODUCT)
	@echo "Compiling product recognition server..."
	@$(CXX) $(CXXFLAGS) $(OPENCV_CFLAGS) -o $(PRODUCT_SERVER_BIN) $< $(OPENCV_LIBS) -lmongoose
	@echo "✓ Product recognition server built: $(PRODUCT_SERVER_BIN)"

# Python dependencies
python_deps:
	@echo "Installing Python dependencies..."
	@pip3 install --user opencv-python requests mysql-connector-python fpdf2 numpy flask flask-cors
	@chmod +x $(PYTHON_DIR)/*.py
	@echo "✓ Python dependencies installed"

# Initialize database
init_db:
	@echo "Initializing database..."
	@mysql -u smart_admin -pMonMotDePasse123! < integrated_db_schema.sql
	@echo "✓ Database initialized"

# Clean build artifacts
clean:
	@echo "Cleaning build artifacts..."
	@rm -rf $(OBJ_DIR) $(BIN_DIR)
	@echo "✓ Clean complete"

# Clean everything including downloaded dependencies
distclean: clean
	@echo "Removing external dependencies..."
	@rm -rf $(EXTERNAL_DIR)/sqlite3.*
	@echo "✓ Distribution clean complete"

# Help
help:
	@echo "Integrated Smart Store System - Build System"
	@echo ""
	@echo "Targets:"
	@echo "  all           - Build everything"
	@echo "  registration  - Build registration system only"
	@echo "  face_server   - Build face recognition server"
	@echo "  product_server- Build product recognition server"
	@echo "  python_deps   - Install Python dependencies"
	@echo "  init_db       - Initialize MySQL database"
	@echo "  clean         - Remove build artifacts"
	@echo "  distclean     - Remove everything including dependencies"
	@echo "  help          - Show this help message"
	@echo ""
	@echo "Usage:"
	@echo "  make all           # Build all components"
	@echo "  make python_deps   # Install Python packages"
	@echo "  make init_db       # Setup database"