#!/bin/bash

# =====================================================
# INTEGRATED SMART STORE - COMPLETE SETUP SCRIPT
# =====================================================

set -e  # Exit on error

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
NC='\033[0m'

# Header
echo -e "${BLUE}"
echo "╔═══════════════════════════════════════════════════════════╗"
echo "║     INTEGRATED SMART STORE - SYSTEM INSTALLATION         ║"
echo "╚═══════════════════════════════════════════════════════════╝"
echo -e "${NC}"
echo ""

# Function to print section headers
print_section() {
    echo -e "${GREEN}[$(date +'%H:%M:%S')] $1${NC}"
}

# Function to check if command exists
command_exists() {
    command -v "$1" &> /dev/null
}

# =====================================================
# STEP 1: System Update
# =====================================================

print_section "STEP 1: Updating system packages..."
sudo apt update
sudo apt upgrade -y

# =====================================================
# STEP 2: Install Build Tools
# =====================================================

print_section "STEP 2: Installing build tools..."

sudo apt install -y \
    build-essential \
    gcc \
    g++ \
    make \
    cmake \
    git \
    pkg-config \
    wget \
    unzip \
    curl

# =====================================================
# STEP 3: Install OpenCV and Dependencies
# =====================================================

print_section "STEP 3: Installing OpenCV and vision libraries..."

sudo apt install -y \
    libopencv-dev \
    libopencv-contrib-dev \
    python3-opencv

# =====================================================
# STEP 4: Install Python and Dependencies
# =====================================================

print_section "STEP 4: Installing Python environment..."

sudo apt install -y \
    python3 \
    python3-pip \
    python3-dev

print_section "Installing Python packages..."
pip3 install --upgrade pip
pip3 install \
    opencv-python \
    requests \
    mysql-connector-python \
    fpdf2 \
    numpy \
    flask \
    flask-cors

# =====================================================
# STEP 5: Install MySQL
# =====================================================

print_section "STEP 5: Installing MySQL database..."

sudo apt install -y \
    mysql-server \
    mysql-client \
    default-libmysqlclient-dev

# Start MySQL service
sudo systemctl enable mysql
sudo systemctl start mysql

# Wait for MySQL to start
sleep 3

# =====================================================
# STEP 6: Configure MySQL Database
# =====================================================

print_section "STEP 6: Configuring database..."

DB_NAME="smart_store_integrated"
DB_USER="smart_admin"
DB_PASS="MonMotDePasse123!"

# Create database and user
sudo mysql -u root <<EOF
CREATE DATABASE IF NOT EXISTS ${DB_NAME};
CREATE USER IF NOT EXISTS '${DB_USER}'@'localhost' IDENTIFIED BY '${DB_PASS}';
GRANT ALL PRIVILEGES ON ${DB_NAME}.* TO '${DB_USER}'@'localhost';
FLUSH PRIVILEGES;
EOF

echo -e "  ${GREEN}✓${NC} Database created: ${DB_NAME}"
echo -e "  ${GREEN}✓${NC} User created: ${DB_USER}"

# =====================================================
# STEP 7: Install libcurl (for HTTP requests)
# =====================================================

print_section "STEP 7: Installing HTTP client libraries..."

sudo apt install -y \
    libcurl4-openssl-dev \
    libssl-dev

# =====================================================
# STEP 8: Configure Serial Port Access
# =====================================================

print_section "STEP 8: Configuring serial port access..."

# Add user to dialout group for serial port access
if ! groups | grep -q dialout; then
    sudo usermod -aG dialout $USER
    echo -e "  ${YELLOW}⚠${NC} Added $USER to dialout group"
    echo -e "  ${YELLOW}⚠${NC} You need to LOG OUT and LOG BACK IN for this to take effect!"
else
    echo -e "  ${GREEN}✓${NC} User already in dialout group"
fi

# =====================================================
# STEP 9: Create Project Structure
# =====================================================

print_section "STEP 9: Creating project directories..."

mkdir -p database
mkdir -p obj bin external
mkdir -p ../images/{clients,produits,temp,temp_produit}
mkdir -p ../invoices
mkdir -p python

chmod -R 755 ../images ../invoices

echo -e "  ${GREEN}✓${NC} Project directories created"

# =====================================================
# STEP 10: Detect Arduino/Fingerprint Sensor
# =====================================================

print_section "STEP 10: Detecting Arduino/fingerprint sensor..."

if ls /dev/ttyUSB* /dev/ttyACM* &> /dev/null; then
    echo -e "  ${GREEN}✓${NC} Serial ports detected:"
    ls -l /dev/ttyUSB* /dev/ttyACM* 2>/dev/null | grep -v total
    echo ""
    echo -e "  ${BLUE}ℹ${NC} Your Arduino is likely on one of these ports"
else
    echo -e "  ${YELLOW}⚠${NC} No serial ports detected"
    echo -e "  ${YELLOW}⚠${NC} Please connect your Arduino via USB"
fi

# =====================================================
# STEP 11: Install Mongoose (HTTP Server Library)
# =====================================================

print_section "STEP 11: Installing Mongoose HTTP server..."

if [ ! -f "external/mongoose.h" ]; then
    cd external
    wget https://raw.githubusercontent.com/cesanta/mongoose/master/mongoose.h
    wget https://raw.githubusercontent.com/cesanta/mongoose/master/mongoose.c
    cd ..
    echo -e "  ${GREEN}✓${NC} Mongoose downloaded"
else
    echo -e "  ${GREEN}✓${NC} Mongoose already present"
fi

# =====================================================
# STEP 12: System Information
# =====================================================

print_section "STEP 12: Verifying installation..."

echo ""
echo "System Information:"
echo "  Python:    $(python3 --version 2>&1)"
echo "  GCC:       $(gcc --version | head -n1)"
echo "  G++:       $(g++ --version | head -n1)"
echo "  OpenCV:    $(pkg-config --modversion opencv4 2>/dev/null || echo 'Not found via pkg-config')"
echo "  MySQL:     $(mysql --version)"
echo ""

# =====================================================
# COMPLETION
# =====================================================

echo -e "${GREEN}"
echo "╔═══════════════════════════════════════════════════════════╗"
echo "║              INSTALLATION COMPLETED!                      ║"
echo "╚═══════════════════════════════════════════════════════════╝"
echo -e "${NC}"
echo ""
echo "Next Steps:"
echo ""
echo "  1. ${YELLOW}IMPORTANT:${NC} If you were just added to dialout group:"
echo "     ${BLUE}→${NC} Log out and log back in for serial port permissions"
echo ""
echo "  2. Initialize the database schema:"
echo "     ${BLUE}→${NC} make init_db"
echo ""
echo "  3. Build all system components:"
echo "     ${BLUE}→${NC} make all"
echo ""
echo "  4. Run the registration system:"
echo "     ${BLUE}→${NC} ./bin/registration_system"
echo ""
echo "  5. Start the vision recognition servers (in separate terminals):"
echo "     ${BLUE}→${NC} ./bin/face_recognition_server"
echo "     ${BLUE}→${NC} ./bin/product_recognition_server"
echo ""
echo "  6. Run the integrated Python system:"
echo "     ${BLUE}→${NC} python3 integrated_smart_store.py"
echo ""
echo "Documentation:"
echo "  • See README.md for detailed usage instructions"
echo "  • Check the examples/ directory for sample configurations"
echo ""
echo -e "${GREEN}Setup completed successfully!${NC}"
echo ""