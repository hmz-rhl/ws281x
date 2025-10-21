#include "driver_ws2812b_interface.h"
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>
#include <errno.h>
#include <stdarg.h>

/* SPI file descriptor */
static int spi_fd = -1;

/**
 * @brief  interface spi 10mhz bus init
 * @return status code
 *         - 0 success
 *         - 1 spi init 10mhz failed
 * @note   none
 */
uint8_t ws2812b_interface_spi_10mhz_init(void)
{
    uint8_t mode = SPI_MODE_0;
    uint8_t bits = 8;
    uint32_t speed = 10000000;  // 10 MHz

    // Ouvrir l'interface SPI
    spi_fd = open("/dev/spidev0.0", O_RDWR);
    if (spi_fd < 0) {
        perror("Unable to open SPI device");
        return 1;  // Échec
    }

    // Configurer le mode SPI, bits et la vitesse
    if (ioctl(spi_fd, SPI_IOC_WR_MODE, &mode) == -1) {
        perror("Failed to set SPI mode");
        close(spi_fd);
        return 1;
    }

    if (ioctl(spi_fd, SPI_IOC_WR_BITS_PER_WORD, &bits) == -1) {
        perror("Failed to set SPI bits per word");
        close(spi_fd);
        return 1;
    }

    if (ioctl(spi_fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed) == -1) {
        perror("Failed to set SPI speed");
        close(spi_fd);
        return 1;
    }

    return 0;  // Succès
}

/**
 * @brief  interface spi bus deinit
 * @return status code
 *         - 0 success
 *         - 1 spi deinit failed
 * @note   none
 */
uint8_t ws2812b_interface_spi_deinit(void)
{
    if (spi_fd >= 0) {
        close(spi_fd);
        spi_fd = -1;
        return 0;  // Succès
    } else {
        return 1;  // Échec (le périphérique SPI n'est pas initialisé)
    }
}

/**
 * @brief     interface spi bus write command
 * @param[in] *buf points to a data buffer
 * @param[in] len is the length of data buffer
 * @return    status code
 *            - 0 success
 *            - 1 write failed
 * @note      none
 */
uint8_t ws2812b_interface_spi_write_cmd(uint8_t *buf, uint16_t len)
{
    if (spi_fd < 0) {
        return 1;  // Échec (l'interface SPI n'est pas initialisée)
    }

    // Configurer la transmission SPI
    struct spi_ioc_transfer transfer = {
        .tx_buf = (unsigned long)buf,
        .len = len,
        .speed_hz = 10000000,  // 10 MHz (confirmé dans la configuration)
        .bits_per_word = 8,
    };

    if (ioctl(spi_fd, SPI_IOC_MESSAGE(1), &transfer) < 1) {
        perror("SPI write failed");
        return 1;  // Échec
    }

    return 0;  // Succès
}

/**
 * @brief     interface delay ms
 * @param[in] ms
 * @note      none
 */
void ws2812b_interface_delay_ms(uint32_t ms)
{
    usleep(ms * 1000);  // Conversion de ms à microsecondes
}

/**
 * @brief     interface print format data
 * @param[in] fmt is the format data
 * @note      none
 */
void ws2812b_interface_debug_print(const char *const fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
}
