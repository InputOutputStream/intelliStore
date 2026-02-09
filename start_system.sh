#!/bin/bash

# =====================================================
# SMART STORE - System Startup Script
# Launches all required components in separate terminals
# =====================================================

GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
NC='\033[0m'

echo -e "${BLUE}"
echo "╔═══════════════════════════════════════════════════════════╗"
echo "║        SMART STORE - SYSTEM STARTUP                      ║"
echo "╚═══════════════════════════════════════════════════════════╝"
echo -e "${NC}"
echo ""

# Check if binaries exist
if [ ! -f "bin/face_recognition_server" ] || [ ! -f "bin/product_recognition_server" ]; then
    echo -e "${YELLOW}⚠ System not built. Building now...${NC}"
    make all
fi

# Function to check if a port is in use
port_in_use() {
    lsof -i :$1 &> /dev/null
}

# Check if servers are already running
if port_in_use 8000; then
    echo -e "${YELLOW}⚠ Port 8000 already in use (Face Recognition Server may be running)${NC}"
fi

if port_in_use 8080; then
    echo -e "${YELLOW}⚠ Port 8080 already in use (Product Recognition Server may be running)${NC}"
fi

# Detect terminal emulator
if command -v gnome-terminal &> /dev/null; then
    TERM_CMD="gnome-terminal --"
elif command -v xterm &> /dev/null; then
    TERM_CMD="xterm -e"
elif command -v konsole &> /dev/null; then
    TERM_CMD="konsole -e"
else
    echo -e "${YELLOW}⚠ No suitable terminal emulator found${NC}"
    echo "Please run the following commands in separate terminals:"
    echo ""
    echo "  Terminal 1: ./bin/face_recognition_server"
    echo "  Terminal 2: ./bin/product_recognition_server"
    echo "  Terminal 3: python3 integrated_smart_store.py"
    echo ""
    exit 1
fi

echo -e "${GREEN}Starting system components...${NC}"
echo ""

# Start Face Recognition Server
echo -e "  ${BLUE}→${NC} Launching Face Recognition Server (port 8000)..."
$TERM_CMD bash -c "cd $(pwd) && ./bin/face_recognition_server; exec bash" &
sleep 2

# Start Product Recognition Server
echo -e "  ${BLUE}→${NC} Launching Product Recognition Server (port 8080)..."
$TERM_CMD bash -c "cd $(pwd) && ./bin/product_recognition_server; exec bash" &
sleep 2

# Start Main System
echo -e "  ${BLUE}→${NC} Launching Integrated Shopping System..."
$TERM_CMD bash -c "cd $(pwd) && python3 integrated_smart_store.py; exec bash" &
sleep 1

echo ""
echo -e "${GREEN}✓ System startup complete!${NC}"
echo ""
echo "Components launched:"
echo "  • Face Recognition Server (localhost:8000)"
echo "  • Product Recognition Server (localhost:8080)"
echo "  • Integrated Shopping System"
echo ""
echo "To manage the system:"
echo "  • Use the camera interface for shopping"
echo "  • Press 'p' to process payment"
echo "  • Press 'q' to quit"
echo ""
echo "To run registration system separately:"
echo "  ./bin/registration_system"
echo ""