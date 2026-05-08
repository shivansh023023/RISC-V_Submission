#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>
#include <sys/select.h>

// Define the default serial port (can be overridden via command line)
#define DEFAULT_PORT "/dev/ttyUSB0"
#define BAUD_RATE B115200

/**
 * @brief Configures the UART interface using the termios API.
 * * Sets the interface to 115200 baud, 8 data bits, no parity, 1 stop bit (8N1).
 * Disables canonical mode and local echo for raw data transmission.
 */
int configure_uart(int fd) {
    struct termios tty;

    // Retrieve current terminal parameters
    if (tcgetattr(fd, &tty) != 0) {
        fprintf(stderr, "Error %d from tcgetattr: %s\n", errno, strerror(errno));
        return -1;
    }

    // Set Baud Rate
    cfsetospeed(&tty, BAUD_RATE);
    cfsetispeed(&tty, BAUD_RATE);

    // 8N1 Mode (8 data bits, No parity, 1 stop bit)
    tty.c_cflag &= ~PARENB; // Clear parity bit, disabling parity
    tty.c_cflag &= ~CSTOPB; // Clear stop field, only one stop bit used
    tty.c_cflag &= ~CSIZE;  // Clear all bits that set the data size 
    tty.c_cflag |= CS8;     // 8 bits per byte

    // Disable hardware flow control (CRTSCTS may not be supported on all systems)
    tty.c_cflag &= ~CRTSCTS;

    // Turn on READ & ignore ctrl lines (CLOCAL = 1)
    tty.c_cflag |= CREAD | CLOCAL;

    // Set local modes: raw, non-canonical, no echo
    tty.c_lflag &= ~ICANON;
    tty.c_lflag &= ~ECHO;
    tty.c_lflag &= ~ECHOE;
    tty.c_lflag &= ~ECHONL;
    tty.c_lflag &= ~ISIG; // Disable interpretation of INTR, QUIT and SUSP

    // Set input modes: disable software flow control
    tty.c_iflag &= ~(IXON | IXOFF | IXANY);
    tty.c_iflag &= ~(IGNBRK|BRKINT|PARMRK|ISTRIP|INLCR|IGNCR|ICRNL);

    // Set output modes: raw output
    tty.c_oflag &= ~OPOST;
    tty.c_oflag &= ~ONLCR;

    // Apply the configuration immediately (TCSANOW)
    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
        fprintf(stderr, "Error %d from tcsetattr: %s\n", errno, strerror(errno));
        return -1;
    }

    return 0;
}

int main(int argc, char *argv[]) {
    const char *portname = (argc > 1) ? argv[1] : DEFAULT_PORT;

    printf("Attempting to open UART port: %s\n", portname);

    // Open the serial port
    // O_RDWR: Read/Write mode
    // O_NOCTTY: Don't make this device the controlling terminal
    // O_NDELAY: Non-blocking open
    int fd = open(portname, O_RDWR | O_NOCTTY | O_NDELAY);
    
    if (fd < 0) {
        fprintf(stderr, "Error %d opening %s: %s\n", errno, portname, strerror(errno));
        if (errno == EACCES) {
            fprintf(stderr, "Hint: You may need to run this with sudo or add your user to the 'dialout' group.\n");
        }
        return EXIT_FAILURE;
    }

    // Flush any stale data in the buffers before configuring
    tcflush(fd, TCIOFLUSH);

    if (configure_uart(fd) != 0) {
        close(fd);
        return EXIT_FAILURE;
    }

    printf("UART %s successfully configured (115200 8N1).\n", portname);

    // Transmit a test message
    const char *msg = "Hello RISC-V M-Mode from Shivansh!\r\n";
    int bytes_written = write(fd, msg, strlen(msg));
    
    if (bytes_written < 0) {
        fprintf(stderr, "Error %d writing to UART: %s\n", errno, strerror(errno));
        close(fd);
        return EXIT_FAILURE;
    }
    printf("Transmitted %d bytes: %s", bytes_written, msg);

    // Prepare for reading with a timeout using select()
    fd_set read_fds;
    struct timeval timeout;
    char read_buf[256];

    printf("Waiting for incoming data (5-second timeout)...\n");

    FD_ZERO(&read_fds);
    FD_SET(fd, &read_fds);

    // Set timeout to 5 seconds
    timeout.tv_sec = 5;
    timeout.tv_usec = 0;

    // Wait for data to be ready to read
    int rv = select(fd + 1, &read_fds, NULL, NULL, &timeout);

    if (rv == -1) {
        fprintf(stderr, "Error %d during select(): %s\n", errno, strerror(errno));
    } else if (rv == 0) {
        printf("Timeout reached! No data received within 5 seconds.\n");
    } else {
        // Data is available to read
        memset(&read_buf, '\0', sizeof(read_buf));
        int num_bytes = read(fd, &read_buf, sizeof(read_buf) - 1);
        
        if (num_bytes < 0) {
            fprintf(stderr, "Error %d reading from UART: %s\n", errno, strerror(errno));
        } else if (num_bytes > 0) {
            printf("Success! Received %d bytes: %s\n", num_bytes, read_buf);
        }
    }

    // Cleanup
    printf("Closing UART port.\n");
    close(fd);
    return EXIT_SUCCESS;
}
