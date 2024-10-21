#!/bin/bash

# Color definitions
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Ensure the script is being run with sudo
if [ "$EUID" -ne 0 ]; then
    echo -e "${RED}[ERROR] Please run this script with sudo or as root.${NC}"
    exit 1
fi

# Detect project path
PROJECT_PATH="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
echo -e "${YELLOW}Project path detected: ${PROJECT_PATH}${NC}"

# Function to print error and exit
function error_exit {
    echo -e "${RED}[ERROR] $1${NC}"
    exit 1
}

# Function to stop running services
function stop_services {
    echo -e "${YELLOW}Stopping any running services...${NC}"
    pkill -f "$PROJECT_PATH/backend" && echo -e "${GREEN}Stopped running backend server.${NC}"
    nginx_pid=$(pidof nginx)
    if [ -n "$nginx_pid" ]; then
        nginx -s stop && echo -e "${GREEN}Stopped running nginx server.${NC}"
    fi
}

# Function to clean previous installations
function clean_previous_installation {
    echo -e "${YELLOW}Cleaning previous installations...${NC}"
    if [ -f "/etc/nginx/nginx.conf.backup" ]; then
        mv /etc/nginx/nginx.conf.backup /etc/nginx/nginx.conf
        echo -e "${GREEN}Restored original nginx configuration.${NC}"
    fi
    echo -e "${GREEN}Previous installations cleaned.${NC}"
}

# Function to install required packages
function install_packages {
    echo -e "${YELLOW}Updating package list...${NC}"
    xbps-install -S || error_exit "Failed to update package list."

    REQUIRED_PACKAGES=(nginx gcc make)

    MISSING_PACKAGES=()
    for pkg in "${REQUIRED_PACKAGES[@]}"; do
        if ! xbps-query -R "$pkg" >/dev/null 2>&1; then
            MISSING_PACKAGES+=("$pkg")
        fi
    done

    if [ ${#MISSING_PACKAGES[@]} -ne 0 ]; then
        echo -e "${YELLOW}Installing required packages: ${MISSING_PACKAGES[*]}${NC}"
        xbps-install -y "${MISSING_PACKAGES[@]}" || error_exit "Failed to install packages."
    else
        echo -e "${GREEN}All required packages are already installed.${NC}"
    fi
}

# Function to configure nginx
function configure_nginx {
    echo -e "${YELLOW}Configuring nginx...${NC}"
    if [ -f "/etc/nginx/nginx.conf" ]; then
        cp /etc/nginx/nginx.conf /etc/nginx/nginx.conf.backup
    fi
    sed "s|REPLACE_WITH_PROJECT_PATH|$PROJECT_PATH|g" "$PROJECT_PATH/nginx.conf.template" > /etc/nginx/nginx.conf
    echo -e "${GREEN}nginx configured successfully.${NC}"
}

# Main script execution
stop_services
clean_previous_installation
install_packages
configure_nginx
echo -e "${GREEN}Installation completed successfully.${NC}"

