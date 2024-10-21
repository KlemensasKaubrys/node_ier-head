#!/bin/bash

# Color definitions
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# Function to print error and exit
function error_exit {
    echo -e "${RED}[ERROR] $1${NC}"
    exit 1
}

# Ensure the script is being run with bash
if [ -z "$BASH_VERSION" ]; then
    error_exit "Please run this script with bash."
fi

# Detect project path
PROJECT_PATH="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
echo -e "${YELLOW}Project path detected: ${PROJECT_PATH}${NC}"

# Ensure scripts have execute permissions
chmod +x "$PROJECT_PATH/install_and_run.sh"

# Function to stop running services
function stop_services {
    echo -e "${YELLOW}Stopping any running services...${NC}"
    # Stop backend if running
    if pgrep -f "$PROJECT_PATH/backend" > /dev/null; then
        pkill -f "$PROJECT_PATH/backend"
        echo -e "${GREEN}Stopped running backend server.${NC}"
    fi
    # Stop nginx if running
    if pidof nginx > /dev/null; then
        sudo nginx -s stop
        echo -e "${GREEN}Stopped running nginx server.${NC}"
    fi
}

# Function to clean previous installations
function clean_previous_installation {
    echo -e "${YELLOW}Cleaning previous installations...${NC}"
    # Remove compiled backend binary
    if [ -f "$PROJECT_PATH/backend" ]; then
        rm "$PROJECT_PATH/backend"
        echo -e "${GREEN}Removed old backend binary.${NC}"
    fi
    # Restore original nginx configuration if backup exists
    if [ -f "/etc/nginx/nginx.conf.backup" ]; then
        sudo mv /etc/nginx/nginx.conf.backup /etc/nginx/nginx.conf
        echo -e "${GREEN}Restored original nginx configuration.${NC}"
    fi
    # Remove temporary files
    if [ -f "$PROJECT_PATH/nginx.conf.tmp" ]; then
        rm "$PROJECT_PATH/nginx.conf.tmp"
        echo -e "${GREEN}Removed temporary files.${NC}"
    fi
    echo -e "${GREEN}Previous installations cleaned.${NC}"
}

# Function to install required packages
function install_packages {
    echo -e "${YELLOW}Updating package list...${NC}"
    sudo xbps-install -S || error_exit "Failed to update package list."

    echo -e "${YELLOW}Installing required packages...${NC}"
    sudo xbps-install -y nginx gcc make || error_exit "Failed to install packages."

    echo -e "${GREEN}All packages installed successfully.${NC}"
}

# Function to compile the backend
function compile_backend {
    echo -e "${YELLOW}Compiling C backend...${NC}"
    gcc -o "$PROJECT_PATH/backend" "$PROJECT_PATH/backend.c" -lpthread || error_exit "Failed to compile backend.c"

    echo -e "${GREEN}C backend compiled successfully.${NC}"
}

# Function to configure nginx
function configure_nginx {
    echo -e "${YELLOW}Configuring nginx...${NC}"

    # Replace placeholder in nginx.conf with actual project path
    sed "s|REPLACE_WITH_PROJECT_PATH|$PROJECT_PATH|g" "$PROJECT_PATH/nginx.conf" > "$PROJECT_PATH/nginx.conf.tmp"

    # Backup existing nginx.conf and replace it
    if [ -f "/etc/nginx/nginx.conf" ]; then
        sudo cp /etc/nginx/nginx.conf /etc/nginx/nginx.conf.backup || error_exit "Failed to backup nginx.conf"
    fi
    sudo cp "$PROJECT_PATH/nginx.conf.tmp" /etc/nginx/nginx.conf || error_exit "Failed to copy nginx.conf"

    rm "$PROJECT_PATH/nginx.conf.tmp"

    echo -e "${GREEN}nginx configured successfully.${NC}"
}

# Function to start services
function start_services {
    # Start backend server
    echo -e "${YELLOW}Starting backend server...${NC}"
    "$PROJECT_PATH/backend" &
    BACKEND_PID=$!

    sleep 1

    # Check if backend is running
    if ps -p $BACKEND_PID > /dev/null
    then
       echo -e "${GREEN}Backend server started successfully (PID: $BACKEND_PID).${NC}"
    else
       error_exit "Backend server failed to start."
    fi

    # Start nginx
    echo -e "${YELLOW}Starting nginx...${NC}"
    sudo nginx

    # Check if nginx is running
    sleep 1
    if pidof nginx > /dev/null
    then
        echo -e "${GREEN}nginx started successfully.${NC}"
    else
        error_exit "nginx failed to start."
    fi

    echo -e "${GREEN}Server is running. Press Ctrl+C to stop.${NC}"
}

# Function to handle graceful shutdown
function graceful_shutdown {
    echo -e "${YELLOW}\nStopping services...${NC}"
    echo -e "${YELLOW}Stopping nginx...${NC}"
    sudo nginx -s stop
    echo -e "${YELLOW}Stopping backend server...${NC}"
    kill $BACKEND_PID
    echo -e "${GREEN}Services stopped.${NC}"
    exit 0
}

# Main script execution

# Stop any running services
stop_services

# Clean previous installations
clean_previous_installation

# Install required packages
install_packages

# Compile the backend
compile_backend

# Configure nginx
configure_nginx

# Start services
start_services

# Trap Ctrl+C to stop services
trap graceful_shutdown INT

# Keep script running
while true
do
    sleep 1
done

