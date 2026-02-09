#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/time.h>
#include <errno.h>
#include "../include/fingerprint.h"
#include <asm-generic/termbits-common.h>

static int serial_fd = -1;

int serial_open(const char* port) {
    // Open serial port
    serial_fd = open(port, O_RDWR | O_NOCTTY | O_SYNC);
    
    if (serial_fd < 0) {
        perror("Error opening serial port");
        return -1;
    }
    
    // Configure serial port
    struct termios tty;
    memset(&tty, 0, sizeof(tty));
    
    if (tcgetattr(serial_fd, &tty) != 0) {
        perror("Error from tcgetattr");
        close(serial_fd);
        serial_fd = -1;
        return -1;
    }
    
    // Set baud rate to 57600
    cfsetospeed(&tty, B57600);
    cfsetispeed(&tty, B57600);
    
    // 8N1 mode
    tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;     // 8-bit chars
    tty.c_iflag &= ~IGNBRK;                         // disable break processing
    tty.c_lflag = 0;                                // no signaling chars, no echo,
                                                    // no canonical processing
    tty.c_oflag = 0;                                // no remapping, no delays
    tty.c_cc[VMIN]  = 0;                            // read doesn't block
    tty.c_cc[VTIME] = 5;                            // 0.5 seconds read timeout
    
    tty.c_iflag &= ~(IXON | IXOFF | IXANY);         // shut off xon/xoff ctrl
    
    tty.c_cflag |= (CLOCAL | CREAD);                // ignore modem controls,
                                                    // enable reading
    tty.c_cflag &= ~(PARENB | PARODD);              // shut off parity
    tty.c_cflag &= ~CSTOPB;                         // one stop bit
    tty.c_cflag &= ~CRTSCTS;                        // no hardware flow control
    
    if (tcsetattr(serial_fd, TCSANOW, &tty) != 0) {
        perror("Error from tcsetattr");
        close(serial_fd);
        serial_fd = -1;
        return -1;
    }
    
    // Flush any old data
    tcflush(serial_fd, TCIOFLUSH);
    
    return 0;
}

void serial_close() {
    if (serial_fd >= 0) {
        close(serial_fd);
        serial_fd = -1;
    }
}

int serial_write(uint8_t* data, int len) {
    if (serial_fd < 0) {
        return -1;
    }
    
    int written = write(serial_fd, data, len);
    if (written < 0) {
        perror("Error writing to serial port");
        return -1;
    }
    
    return written;
}

int serial_read(uint8_t* buffer, int len, int timeout_ms) {
    if (serial_fd < 0) {
        return -1;
    }
    
    struct timeval start, now;
    gettimeofday(&start, NULL);
    
    int total = 0;
    
    while (1) {
        gettimeofday(&now, NULL);
        long elapsed = (now.tv_sec - start.tv_sec) * 1000 + 
                       (now.tv_usec - start.tv_usec) / 1000;
        
        if (elapsed >= timeout_ms) {
            break;
        }
        
        int n = read(serial_fd, buffer + total, len - total);
        
        if (n > 0) {
            total += n;
            if (total >= len) {
                break;
            }
        } else if (n < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
            perror("Error reading from serial port");
            return -1;
        }
        
        usleep(10000);  // 10ms delay
    }
    
    return total;
}