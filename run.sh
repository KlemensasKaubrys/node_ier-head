#!/bin/bash

# Color definitions
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Detect project path
PROJECT_PATH="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

# Function to print error and exit
function error_exit {
    echo -e "${RED}[ERROR] $1${NC}"
    exit 1
}

# Function to compile the backend
function compile_backend {
    echo -e "${YELLOW}Compiling C backend...${NC}"
    gcc -o "$PROJECT_PATH/backend" "$PROJECT_PATH/backend.c" -lpthread || error_exit "Failed to compile backend.c"
    echo -e "${GREEN}C backend compiled successfully.${NC}"
}

# Function to start services
function start_services {
    echo -e "${YELLOW}Starting backend server...${NC}"
    "$PROJECT_PATH/backend" &
    BACKEND_PID=$!
    sleep 1
    if ps -p $BACKEND_PID > /dev/null; then
        echo -e "${GREEN}Backend server started successfully (PID: $BACKEND_PID).${NC}"
    else
        error_exit "Backend server failed to start."
    fi

    echo -e "${YELLOW}Starting nginx...${NC}"
    sudo nginx
    sleep 1
    if pidof nginx > /dev/null; then
        echo -e "${GREEN}nginx started successfully.${NC}"
    else
        error_exit "nginx failed to start."
    fi

    echo -e "${GREEN}Server is running. Press Ctrl+C to stop.${NC}"
}

# Function to stop services
function stop_services {
    echo -e "${YELLOW}\nStopping services...${NC}"
    echo -e "${YELLOW}Stopping nginx...${NC}"
    sudo nginx -s stop

    if ps -p $BACKEND_PID > /dev/null; then
        echo -e "${YELLOW}Stopping backend server...${NC}"
        kill $BACKEND_PID
        echo -e "${GREEN}Backend server stopped.${NC}"
    else
        echo -e "${YELLOW}Backend server is not running.${NC}"
    fi

    exit 0
}

# Main script execution
compile_backend
start_services

trap stop_services INT

while true; do
    sleep 1
done

