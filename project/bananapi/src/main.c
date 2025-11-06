/**
 * @file      led_sequence.c
 * @brief     main source file
 * @version   1.0.0
 * @author    Hamza RAHAL
 * @date      13/04/2025
 *
 */

#include "driver_ws2812b_write_test.h"
#include "driver_ws2812b_basic.h"
#include <getopt.h>
#include <stdlib.h>
#include <unistd.h>

#include <stdio.h>
#include <stdint.h>
#include <sys/stat.h>
#include <sys/time.h>

#include "hubfiles.h"
#include "hubtypes.h"

#define NB_LEDS 112
#define TEMP_ARRAY_SIZE (NB_LEDS*64 + 1)

/**
* @brief global var definition
*/
static ws2812b_handle_t handle;        /**< ws2812b handle */
static ws2812b_info_t info;
static uint32_t color_array[NB_LEDS];                /**< rgb buffer */
static uint8_t temp[TEMP_ARRAY_SIZE];         // HMZ on ajoute un octets vide pour forcer un reset     /**< temp buffer*/

/**
 * @brief     main function
 * @param[in] argc is arg numbers
 * @param[in] **argv is the arg address
 * @return    status code
 *             - 0 success
 * @note      none
 */

char active[25];

int nb_leds;
int leds_order;		// Organisation RGB, RBG, etc.
int color, scheme, level_stop, level_start;
int state, blOn;	// États courants du chargeur et de son écran

int puissanceEV;			// Défini la vitesse de rotation du mode Chenille en fonction de la puissance appelée

static void initLeds() {
    printf("entering %s\n", __func__);

	// ledstring.freq = WS2811_TARGET_FREQ;
	// ledstring.dmanum = 10;

	// ledstring.channel[0].gpionum = 21;
	// ledstring.channel[0].invert = 0;
	// ledstring.channel[0].count = nb_leds;
	// ledstring.channel[0].strip_type = WS2811_STRIP_GBR;
	// ledstring.channel[0].brightness = 127;
	// //inutile mais on definit
	// ledstring.channel[1].gpionum = 0;
	// ledstring.channel[1].invert = 0;
	// ledstring.channel[1].count = 0;
	// ledstring.channel[1].brightness = 0;

    int ret = -1;

    /* link interface function */
    printf("DRIVER_WS2812B_LINK_INIT\n");
    DRIVER_WS2812B_LINK_INIT(&handle, ws2812b_handle_t);
    printf("DRIVER_WS2812B_LINK_SPI_10MHZ_INIT\n");
    DRIVER_WS2812B_LINK_SPI_10MHZ_INIT(&handle, ws2812b_interface_spi_10mhz_init);
    printf("DRIVER_WS2812B_LINK_SPI_DEINIT\n");
    DRIVER_WS2812B_LINK_SPI_DEINIT(&handle, ws2812b_interface_spi_deinit);
    printf("DRIVER_WS2812B_LINK_SPI_WRITE_COMMAND\n");
    DRIVER_WS2812B_LINK_SPI_WRITE_COMMAND(&handle, ws2812b_interface_spi_write_cmd);
    printf("DRIVER_WS2812B_LINK_DELAY_MS\n");
    DRIVER_WS2812B_LINK_DELAY_MS(&handle, ws2812b_interface_delay_ms);
    printf("DRIVER_WS2812B_LINK_DEBUG_PRINT\n");
    DRIVER_WS2812B_LINK_DEBUG_PRINT(&handle, ws2812b_interface_debug_print);;
    while(ret != 0){
        if((ret = ws2812b_init(&handle)) != 0){
            printf("ws2812b_init failed (%d)\n", ret);
        }
        else if((ret = ws2812b_info(&info)) != 0){
            printf("ws2812b_info failed (%d)\n", ret);
        }
        else{
            // pour sortir plus vite
            break;
        }
        usleep(1000000);
    }
}

static double getMaxValue(LedPower level) {
	switch (level) {
	case LEVEL_MAX :	// 100%
		return 255;
	case LEVEL_HIGH :	// 50%
		return 148;
	case LEVEL_NORMAL :	// 30%
		return 88;
	case LEVEL_MIN :	// 10%
		return 24;
	default :			// LEVEL_OFF
		return 0;
	}
}

static int getDelay(LedPower level) {
	switch (level) {
	case LEVEL_MAX :	// 100%
		return 3000;
	case LEVEL_HIGH :	// 50%
		return 6000;
	case LEVEL_NORMAL :	// 30%
		return 9000;
	case LEVEL_MIN :	// 10%
	default :			// LEVEL_OFF
		return 18000;
	}
}

static int getRedColorWithDecalage(int red, int green, int blue) {
	switch(leds_order) {
		case ORDER_GBR :
			return blue;
		case ORDER_BRG :
			return green;
		case ORDER_GRB :
			return green;
		case ORDER_RBG :
			return red;
		case ORDER_BGR :
			return blue;
		default: // RGB
			return red;
	}
}

static int getGreenColorWithDecalage(int red, int green, int blue) {
	switch(leds_order) {
		case ORDER_GBR :
			return red;
		case ORDER_BRG :
			return blue;
		case ORDER_GRB :
			return red;
		case ORDER_RBG :
			return blue;
		case ORDER_BGR :
			return green;
		default: // RGB
			return green;
	}
}

static int getBlueColorWithDecalage(int red, int green, int blue) {
	switch(leds_order) {
		case ORDER_GBR :
			return green;
		case ORDER_BRG :
			return red;
		case ORDER_GRB :
			return blue;
		case ORDER_RBG :
			return green;
		case ORDER_BGR :
			return red;
		default: // RGB
			return blue;
	}
}

static void launchChenille(int redConfig, int greenConfig, int blueConfig, LedPower level, LedPower level_alt) {
    printf("entering %s\n", __func__);

    int ret;
    int updateChenille = OFF;
    int oldLevel, blOn_NEW, puissanceEV_NEW;
    
    // LEVEL_OFF, on éteint tout
	if (level == LEVEL_OFF) {
		for (size_t i = 0; i < nb_leds; i++) {
			color_array[i] = 0x000000;
		}
		if ((ret = ws2812b_write(&handle, color_array, NB_LEDS, temp, TEMP_ARRAY_SIZE)) != 0)
        {
            printf("ws2812b write failed: %d (%s at line %d)\n", ret, __func__, __LINE__);
        }
		return;
	}

	// Délai pondéré par la vitesse de charge
	int delay = getDelay(level) * 33 / puissanceEV;

	// Arrangement et pondération des LEDs
	double redComp = getRedColorWithDecalage(redConfig, greenConfig, blueConfig) / 255.0;
	double greenComp = getGreenColorWithDecalage(redConfig, greenConfig, blueConfig) / 255.0;
	double blueComp = getBlueColorWithDecalage(redConfig, greenConfig, blueConfig) / 255.0;

	// valeurs max pondérées
	int mrv = redComp * getMaxValue(level);
	int mgv = greenComp * getMaxValue(level);
	int mbv = blueComp * getMaxValue(level);

	// valeurs des pixels latéraux
	int srv = mrv >> 2;
	int sgv = mgv >> 2;
	int sbv = mbv >> 2;

	for (size_t h = 0; h < nb_leds; h++) {
		for (size_t i = 0; i < nb_leds; i++) {
			if ((h == 0 && i == nb_leds - 1) || i == h-1) {
				color_array[i] = 0x000000 + srv + (sgv<<8) + (sbv<<16);
			}
			else if (i == h) {
				color_array[i] = 0x000000 + mrv + (mgv<<8) + (mbv<<16);
			}
			else if ((h == nb_leds - 1 && i == 0) || i == h+1) {
				color_array[i] = 0x000000 + srv + (sgv<<8) + (sbv<<16);
			}
			else {
				color_array[i] = 0x000000;
			}
		}
        if ((ret = ws2812b_write(&handle, color_array, NB_LEDS, temp, TEMP_ARRAY_SIZE)) != 0)
        {
            printf("ws2812b write failed: %d (%s at line %d)\n", ret, __func__, __LINE__);
            break;
        }

		// Si le status machine à changé, il faut stopper la séquence
		readNotifFile(NOTIF_STATE_EVSE, active);
		if (state != atoi(active)) return;

		// Si le baclight a changé, alors l'intensité et le delai doivent être mis à jour
		readNotifFile(NOTIF_BACKLIGHT, active);
		blOn_NEW = !strcmp(active, "On");
		if (blOn != blOn_NEW) {
			blOn = blOn_NEW;
			// Inversion des niveaux courant et alternatif
			oldLevel = level;
			level = level_alt;
			level_alt = oldLevel;
			updateChenille = ON;
		}
		// Si la vitesse de charge du véhicule a changé, alors l'intensité et le delai doivent être mis à jour
		readNotifFile(NOTIF_VALUE_POWER, active);
		puissanceEV_NEW = (atoi(active) / 1000) + 1;
		if (puissanceEV != puissanceEV_NEW) {
			puissanceEV = puissanceEV_NEW;
			updateChenille = ON;
		}
		
		if (updateChenille) {
			// Parametre de fréquence
			delay = getDelay(level) * 33 / puissanceEV;

			// valeurs max pondérées
			mrv = redComp * getMaxValue(level);
			mgv = greenComp * getMaxValue(level);
			mbv = blueComp * getMaxValue(level);

			// valeurs des pixels latéraux
			srv = mrv >> 2;
			sgv = mgv >> 2;
			sbv = mbv >> 2;

			updateChenille = OFF;
		}

		usleep(delay); // env. 10 msec
	}
}

static void launchBlink(int redConfig, int greenConfig, int blueConfig, LedPower level) {
    printf("entering %s\n", __func__);

    int ret;
    
    // LEVEL_OFF, on éteint tout
	if (level == LEVEL_OFF) {
		for (size_t i = 0; i < nb_leds; i++) {
			color_array[i] = 0x000000;
		}
		if ((ret = ws2812b_write(&handle, color_array, NB_LEDS, temp, TEMP_ARRAY_SIZE)) != 0)
        {
            printf("ws2812b write failed: %d (%s at line %d)\n", ret, __func__, __LINE__);
        }
		return;
	}

    // Arrangement des LEDs
	int redComp = getRedColorWithDecalage(redConfig, greenConfig, blueConfig);
	int greenComp = getGreenColorWithDecalage(redConfig, greenConfig, blueConfig);
	int blueComp = getBlueColorWithDecalage(redConfig, greenConfig, blueConfig);

    // Parametres d'intensité et de délai de persistance
	double maxValue = getMaxValue(level) / 255.0;
	int delay = getDelay(level);

    // valeurs max pondérées
	int mrv = (int)((double)redComp * maxValue);
	int mgv = (int)((double)greenComp * maxValue);
	int mbv = (int)((double)blueComp * maxValue);

    // OFF period
	for (size_t i = 0; i < nb_leds; i++) {
		color_array[i] = 0x000000;
	}
    if ((ret = ws2812b_write(&handle, color_array, NB_LEDS, temp, TEMP_ARRAY_SIZE)) != 0)
    {
        printf("ws2812b write failed: %d (%s at line %d)\n", ret, __func__, __LINE__);
    }
	usleep(delay<<6); // env. 0,3 seconde

	// Si le status machine à changé, il faut stopper la séquence
	readNotifFile(NOTIF_STATE_EVSE, active);
	if (state != atoi(active)) return;
	// Si le baclight a changé, il faut stopper la séquence
	readNotifFile(NOTIF_BACKLIGHT, active);
	if (blOn == strcmp(active, "On")) return;

    // Fade IN
	for (int k = 3; k >= 0; k--) {
		int rv = mrv >> k;
		int gv = mgv >> k;
		int bv = mbv >> k;
		for (size_t i = 0; i < nb_leds; i++) {
			color_array[i] = 0x000000 + rv + (gv<<8) + (bv<<16);
		}

        if ((ret = ws2812b_write(&handle, color_array, NB_LEDS, temp, TEMP_ARRAY_SIZE)) != 0)
        {
            printf("ws2812b write failed: %d (%s at line %d)\n", ret, __func__, __LINE__);
            break;
        }
		usleep(delay<1);
	}
	
	usleep(delay);

    // FADE OUT
	for (int k = 0; k <= 3; k++) {
		int rv = mrv >> k;
		int gv = mgv >> k;
		int bv = mbv >> k;
		//fprintf(stderr, "FADEOUT - Render color: (%u, %u, %u)\n", rv, gv, bv);
		for (size_t i = 0; i < nb_leds; i++) {
			color_array[i] = 0x000000 + rv + (gv<<8) + (bv<<16);
		}
        if ((ret = ws2812b_write(&handle, color_array, NB_LEDS, temp, TEMP_ARRAY_SIZE)) != 0)
        {
            printf("ws2812b write failed: %d (%s at line %d)\n", ret, __func__, __LINE__);
            break;
        }
		usleep(delay<1);
	}

	// Si le status machine à changé, il faut stopper la séquence
	readNotifFile(NOTIF_STATE_EVSE, active);
	if (state != atoi(active)) return;
	// Si le baclight a changé, il faut stopper la séquence
	readNotifFile(NOTIF_BACKLIGHT, active);
	if (blOn == strcmp(active, "On")) return;

    // OFF period
	for (size_t i = 0; i < nb_leds; i++) {
		color_array[i] = 0x000000;
	}
    if ((ret = ws2812b_write(&handle, color_array, NB_LEDS, temp, TEMP_ARRAY_SIZE)) != 0)
    {
        printf("ws2812b write failed: %d (%s at line %d)\n", ret, __func__, __LINE__);
    }
	usleep(delay<<6); // env. 0,3 seconde
}

static void launchPulse(int redConfig, int greenConfig, int blueConfig, LedPower level) {
    printf("entering %s\n", __func__);

    int ret;
    
    // LEVEL_OFF, on éteint tout
	if (level == LEVEL_OFF) {
		for (size_t i = 0; i < nb_leds; i++) {
			color_array[i] = 0x000000;
		}
		if ((ret = ws2812b_write(&handle, color_array, NB_LEDS, temp, TEMP_ARRAY_SIZE)) != 0)
        {
            printf("ws2812b write failed: %d (%s at line %d)\n", ret, __func__, __LINE__);
        }
		return;
	}

    // Arrangement des LEDs
	int redComp = getRedColorWithDecalage(redConfig, greenConfig, blueConfig);
	int greenComp = getGreenColorWithDecalage(redConfig, greenConfig, blueConfig);
	int blueComp = getBlueColorWithDecalage(redConfig, greenConfig, blueConfig);

    // Parametres d'intensité et de délai de persistance
	double maxValue = getMaxValue(level) / 255.0;
	int delay = getDelay(level);

    // valeurs d'incréments pondérés
	double mrv = (((double)redComp * maxValue) / 64.0);
	double mgv = (((double)greenComp * maxValue) / 64.0);
	double mbv = (((double)blueComp * maxValue) / 64.0);

    // OFF period
	for (size_t i = 0; i < nb_leds; i++) {
		color_array[i] = 0x000000;
	}
    if ((ret = ws2812b_write(&handle, color_array, NB_LEDS, temp, TEMP_ARRAY_SIZE)) != 0)
    {
        printf("ws2812b write failed: %d (%s at line %d)\n", ret, __func__, __LINE__);
    }

	usleep(delay<<1);

    // Fade IN
	for (int k = 1; k <= 64; k++) {
		int rv = (int)(mrv * k);
		int gv = (int)(mgv * k);
		int bv = (int)(mbv * k);
		for (size_t i = 0; i < nb_leds; i++) {
			color_array[i] = 0x000000 + rv + (gv<<8) + (bv<<16);
		}

        if ((ret = ws2812b_write(&handle, color_array, NB_LEDS, temp, TEMP_ARRAY_SIZE)) != 0)
        {
            printf("ws2812b write failed: %d (%s at line %d)\n", ret, __func__, __LINE__);
            break;
        }
		usleep(delay<<1);
		// Si le status machine à changé, il faut stopper la séquence
		readNotifFile(NOTIF_STATE_EVSE, active);
		if (state != atoi(active)) return;
		// Si le baclight a changé, il faut stopper la séquence
		readNotifFile(NOTIF_BACKLIGHT, active);
		if (blOn == strcmp(active, "On")) return;

	}

	usleep(delay<<2);

    // FADE OUT
	for (int k = 64; k >= 1; k--) {
		int rv = (int)(mrv * k);
		int gv = (int)(mgv * k);
		int bv = (int)(mbv * k);
		//fprintf(stderr, "FADEOUT - Render color: (%u, %u, %u)\n", rv, gv, bv);
		for (size_t i = 0; i < nb_leds; i++) {
			color_array[i] = 0x000000 + rv + (gv<<8) + (bv<<16);
		}
        if ((ret = ws2812b_write(&handle, color_array, NB_LEDS, temp, TEMP_ARRAY_SIZE)) != 0)
        {
            printf("ws2812b write failed: %d (%s at line %d)\n", ret, __func__, __LINE__);
            break;
        }
		usleep(delay<<1);
		// Si le status machine à changé, il faut stopper la séquence
		readNotifFile(NOTIF_STATE_EVSE, active);
		if (state != atoi(active)) return;
		// Si le baclight a changé, il faut stopper la séquence
		readNotifFile(NOTIF_BACKLIGHT, active);
		if (blOn == strcmp(active, "On")) return;

	}
}

// 
static void launchFixed(int redConfig, int greenConfig, int blueConfig, LedPower level_end, LedPower level_start) {
    printf("entering %s\n", __func__);

    int ret;
    
	// Arrangement des LEDs
	int redComp = getRedColorWithDecalage(redConfig, greenConfig, blueConfig);
	int greenComp = getGreenColorWithDecalage(redConfig, greenConfig, blueConfig);
	int blueComp = getBlueColorWithDecalage(redConfig, greenConfig, blueConfig);

	// Parametres d'intensité et de délai de persistance
	double startValue = getMaxValue(level_start) / 255.0;
	double stepValue = (getMaxValue(level_end) - getMaxValue(level_start)) / 255.0;
	int delay = getDelay(level_end);

	// valeurs de départ pondérées
	double drv = ((double)redComp * startValue);
	double dgv = ((double)greenComp * startValue);
	double dbv = ((double)blueComp * startValue);

	// valeurs d'incréments pondérés
	double srv = (((double)redComp * stepValue) / 64.0);
	double sgv = (((double)greenComp * stepValue) / 64.0);
	double sbv = (((double)blueComp * stepValue) / 64.0);

    
	// Start period
	for (size_t i = 0; i < nb_leds; i++) {
        color_array[i] = 0x000000 + (int)drv + ((int)dgv<<8) + ((int)dbv<<16);
	}
    printf("end of start period (%s)\n", __func__);
    if ((ret = ws2812b_write(&handle, color_array, NB_LEDS, temp, TEMP_ARRAY_SIZE)) != 0)
    {
        printf("ws2812b write failed: %d (%s at line %d)\n", ret, __func__, __LINE__);
    }
	usleep(delay<<1); // env. 1 msec

	// Fade START -> END
	for (int k = 1; k <= 64; k++) {
		int rv = (int)(drv + (srv * k));
		int gv = (int)(dgv + (sgv * k));
		int bv = (int)(dbv + (sbv * k));
		for (size_t i = 0; i < nb_leds; i++) {
			color_array[i] = 0x000000 + rv + (gv<<8) + (bv<<16);
		}

        if ((ret = ws2812b_write(&handle, color_array, NB_LEDS, temp, TEMP_ARRAY_SIZE)) != 0)
        {
            printf("ws2812b write failed: %d (%s at line %d)\n", ret, __func__, __LINE__);
            break;
        }
		usleep(delay<<1);
		// Si le status machine à changé, il faut stopper la séquence
		readNotifFile(NOTIF_STATE_EVSE, active);
		if (state != atoi(active)) return;
		// Si le baclight a changé, il faut stopper la séquence
		readNotifFile(NOTIF_BACKLIGHT, active);
		if (blOn == strcmp(active, "On")) return;

	}
}

// Commande des LEDs
static void launchSequence(LedColor color, LedScheme scheme, LedPower level_end, LedPower level_start) {
    printf("entering %s\n", __func__);

    int red, green, blue;
    printf("entering %s\n", __func__);
    // Définition des couleurs RGB à utiliser
	switch(color) {
	case BLUE :
		red = 0;
		green = 0;
		blue = 255;
		break;
	case GREEN :
		red = 0;
		green = 255;
		blue = 0;
		break;
	case RED :
		red = 255;
		green = 0;
		blue = 0;
		break;
	case ORANGE :
		red = 255;
		green = 95;
		blue = 0;
		break;
	case YELLOW :
		red = 255;
		green = 255;
		blue = 0;
		break;
	default: // WHITE
		red = 255;
		green = 255;
		blue = 255;
	}

    // Définition du schéma de comportement des LEDs
	switch(scheme) {
	case PULSE :
		launchPulse(red, green, blue, level_end);
		break;
	case BLINK :
		launchBlink(red, green, blue, level_end);
		break;
	case CHENILLE :
		launchChenille(red, green, blue, level_end, level_start);
		break;
	case CONSTANT :
	default :
		launchFixed(red, green, blue, level_end, level_start);
	}
}


//__________MAIN__________//

int main(uint8_t argc, char **argv)
{
    if (argc < 6 || (!strcmp(argv[1],"-h") || !strcmp(argv[1],"--help"))) {
        printf("Usage: ./led_sequence -h, --help \taffiche ce message d'aide \n");
        printf("       ./led_sequence <nb_leds> <leds_order> <color> <sequence> <end_power> [<start_power>] [<puissanceEV>]\n");
        printf("exemple: ./led_sequence 112 3 1 2 255 [127] [32]\n");
        exit(EXIT_SUCCESS);
    } else {
	    nb_leds = atoi(argv[1]);
    	leds_order = atoi(argv[2]);
 		color = atoi(argv[3]);
 		scheme = atoi(argv[4]);
 		level_stop = atoi(argv[5]);
		if (argc < 7)	level_start = level_stop;
		else			level_start = atoi(argv[6]);
		if (argc < 8)	puissanceEV = 7;
		else			puissanceEV = atoi(argv[7]);
	}

	initLeds();

	// État du BL en début de cycle
	readNotifFile(NOTIF_BACKLIGHT, active);
	blOn = !strcmp(active, "On");
	// État courant de la borne en début de cycle
	readNotifFile(NOTIF_STATE_EVSE, active);
	state = atoi(active);

	launchSequence(color, scheme, level_stop, level_start);

}