#!/bin/bash
# NERD Installation Script
# Network-Flow Editor Production Installation

set -e

echo "============================================"
echo "NERD - Network-Flow Editor Installation"
echo "============================================"

# Check if running as root
if [ "$EUID" -ne 0 ]; then 
    echo "Error: This installation script must be run as root"
    echo "Please run: sudo ./install.sh"
    exit 1
fi

# Create necessary directories
echo "Creating directories..."
mkdir -p /etc/nerd
mkdir -p /var/lib/nerd
mkdir -p /var/log

# Build the project
echo "Building NERD..."
if [ ! -d "build" ]; then
    mkdir build
fi

cd build
cmake ..
make -j$(nproc)

# Install binary
echo "Installing binary..."
install -m 755 nerd /usr/local/bin/nerd

# Install configuration
echo "Installing configuration..."
cd ..
if [ ! -f "/etc/nerd/nerd.conf" ]; then
    install -m 644 config/nerd.conf /etc/nerd/nerd.conf
    echo "Installed default configuration to /etc/nerd/nerd.conf"
else
    echo "Configuration file already exists, skipping..."
fi

# Install systemd service
echo "Installing systemd service..."
install -m 644 config/nerd.service /etc/systemd/system/nerd.service
systemctl daemon-reload

# Set permissions
echo "Setting permissions..."
chown -R root:root /etc/nerd
chown -R root:root /var/lib/nerd
chmod 755 /var/lib/nerd

# Create log file
touch /var/log/nerd.log
chmod 644 /var/log/nerd.log

echo ""
echo "============================================"
echo "Installation Complete!"
echo "============================================"
echo ""
echo "NERD has been successfully installed."
echo ""
echo "To start NERD as a service:"
echo "  systemctl start nerd"
echo ""
echo "To enable NERD on boot:"
echo "  systemctl enable nerd"
echo ""
echo "To run NERD manually:"
echo "  sudo nerd [flow_name]"
echo ""
echo "Configuration file: /etc/nerd/nerd.conf"
echo "Log file: /var/log/nerd.log"
echo "Data directory: /var/lib/nerd"
echo ""
echo "For help:"
echo "  nerd --help"
echo ""