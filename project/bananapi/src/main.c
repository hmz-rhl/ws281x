/**
 * Copyright (c) 2015 - present LibDriver All rights reserved
 * 
 * The MIT License (MIT)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE. 
 *
 * @file      main.c
 * @brief     main source file
 * @version   1.0.0
 * @author    Shifeng Li
 * @date      2021-11-13
 *
 * <h3>history</h3>
 * <table>
 * <tr><th>Date        <th>Version  <th>Author      <th>Description
 * <tr><td>2021/11/13  <td>1.0      <td>Shifeng Li  <td>first upload
 * </table>
 */

#include "driver_ws2812b_write_test.h"
#include "driver_ws2812b_basic.h"
#include <getopt.h>
#include <stdlib.h>
#include <unistd.h>

#define NB_LEDS 112
#define TEMP_ARRAY_SIZE (NB_LEDS*64 + 1)

/**
 * @brief global var definition
 */
static ws2812b_handle_t handle;        /**< ws2812b handle */
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
int main(uint8_t argc, char **argv)
{
    int ret;
    ws2812b_info_t info;



    /* link interface function */
    DRIVER_WS2812B_LINK_INIT(&handle, ws2812b_handle_t);
    DRIVER_WS2812B_LINK_SPI_10MHZ_INIT(&handle, ws2812b_interface_spi_10mhz_init);
    DRIVER_WS2812B_LINK_SPI_DEINIT(&handle, ws2812b_interface_spi_deinit);
    DRIVER_WS2812B_LINK_SPI_WRITE_COMMAND(&handle, ws2812b_interface_spi_write_cmd);
    DRIVER_WS2812B_LINK_DELAY_MS(&handle, ws2812b_interface_delay_ms);
    DRIVER_WS2812B_LINK_DEBUG_PRINT(&handle, ws2812b_interface_debug_print);;
    
    if((ret = ws2812b_init(&handle)) != 0){
        printf("ws2812b_init failed (%d)\n", ret);
        return 1;
    }
    if((ret = ws2812b_info(&info)) != 0){
        printf("ws2812b_info failed (%d)\n", ret);
        return 1;
    }
    
    while(1){

        if(argc > 1){
            if(atoi(argv[1]) == 1){

                for(int j = 0; j < 3; j++ ) {
                // Fade IN
                    for(int k = 0; k < 256; k++) {

                        
                        for (size_t i = 0; i < NB_LEDS; i++)
                        {
                            /* code */
                            color_array[i] = 0x000000 + (k<<(16-8*j));
                        }
                        

                        if ((ret = ws2812b_write(&handle, color_array, NB_LEDS, temp, TEMP_ARRAY_SIZE)) != 0)
                        {
                            printf("ws2812b write failed: %d\n", ret);
                            break;
                        }
                        //delay(3);
                        usleep(3000);
                    }	
                    // Fade OUT
                    for(int k = 255; k >= 0; k--) {
                    
                        for (size_t i = 0; i < NB_LEDS; i++)
                        {
                            /* code */
                            color_array[i] = 0x000000 + (k<<(16-8*j));
                        }
                        

                        if ((ret = ws2812b_write(&handle, color_array, NB_LEDS, temp, TEMP_ARRAY_SIZE)) != 0)
                        {
                            printf("ws2812b write failed: %d\n", ret);
                            break;
                        }
                        //delay(3);
                        usleep(3000);
                    }
                }	
            }

            else if(atoi(argv[1]) == 2){

                uint8_t blue, green, red;
                unsigned char WheelPos;
                uint16_t i, j;

                for(j=0; j<256*5; j++) { // 5 cycles of all colors on wheel

                    for(i=0; i< NB_LEDS; i++) {
                        

                        WheelPos = (((i * 256 / NB_LEDS) + j) & 255);
                        if(WheelPos < 85) {
                            blue = WheelPos * 3;
                            green = 255 - WheelPos * 3;
                            red = 0;
                        } else if(WheelPos < 170) {
                            WheelPos -= 85;
                            blue = 255 - WheelPos * 3;
                            green = 0;
                            red = WheelPos * 3;
                        } else {
                            WheelPos -= 170;
                            blue = 0;
                            green = WheelPos * 3;
                            red = 255 - WheelPos * 3;
                        }
                        //                  blue            green     red
                        color_array[i] = (blue << 16) + (green << 8) + red ; 
                    }
                    if ((ret = ws2812b_write(&handle, color_array, NB_LEDS, temp, TEMP_ARRAY_SIZE)) != 0)
                    {
                        printf("ws2812b write failed: %d\n", ret);
                        break;
                    }
                    

                    usleep(10000);
                    
                }
                        

            }

            else if(atoi(argv[1]) == 3){
                int j = 0;
                
                // Fade IN
                for(int k = 0; k < 256; k++) {

                    
                    for (size_t i = 0; i < NB_LEDS; i++)
                    {
                        /* code */
                        if(j == 0){
                            color_array[i] = (k)<<16       + (255-k)<<8 + 0;
                        }
                        if(j == 1){
                            color_array[i] = (255-k)<<16   + 0          + k;
                        }
                        if(j == 2){
                            color_array[i] = 0             +       k<<8 + (255-k);
                        }
                        if(j == 3){
                            
                        }
                        if(j == 4){
                            
                        }
                        if(j == 5){
                            
                        }

                        //color_array[i] = 0x000000 + j == 0(k<<8) + (255-k);
                    }
                    

                    if ((ret = ws2812b_write(&handle, color_array, NB_LEDS, temp, TEMP_ARRAY_SIZE)) != 0)
                    {
                        printf("ws2812b write failed: %d\n", ret);
                        break;
                    }
                    //delay(3);
                    usleep(3000);
                }	
                // Fade OUT
                for(int k = 255; 0; k--) {
                
                    for (size_t i = 0; i < NB_LEDS; i++)
                    {
                        /* code */
                                                /* code */
                        if(j == 0){
                            color_array[i] = (k)<<16       + (255-k)<<8 + 0;
                        }
                        if(j == 1){
                            color_array[i] = (255-k)<<16   + 0          + k;
                        }
                        if(j == 2){
                            color_array[i] = 0             +       k<<8 + (255-k);
                        }
                        //color_array[i] = 0x000000 + (k<<8);
                    }
                    

                    if ((ret = ws2812b_write(&handle, color_array, NB_LEDS, temp, TEMP_ARRAY_SIZE)) != 0)
                    {
                        printf("ws2812b write failed: %d\n", ret);
                        break;
                    }
                    //delay(3);
                    usleep(3000);
                }
                j = j+1 > 3 ? 0 : j+1;
                    
            }

            else if(atoi(argv[1]) == 4){
                for(int k = 0; k < 256; k++) {

                    
                    for (size_t i = 0; i < NB_LEDS; i++)
                    {
                        /* code */
                        color_array[i] = 0x000000 + (k<<16);
                    }
                    

                    if ((ret = ws2812b_write(&handle, color_array, NB_LEDS, temp, TEMP_ARRAY_SIZE)) != 0)
                    {
                        printf("ws2812b write failed: %d\n", ret);
                        break;
                    }
                    //delay(3);
                    usleep(3000);
                }	
                // Fade OUT
                for(int k = 255; k >= 0; k--) {
                
                    for (size_t i = 0; i < NB_LEDS; i++)
                    {
                        /* code */
                        color_array[i] = 0x000000 + (k<<16);
                    }
                    

                    if ((ret = ws2812b_write(&handle, color_array, NB_LEDS, temp, TEMP_ARRAY_SIZE)) != 0)
                    {
                        printf("ws2812b write failed: %d\n", ret);
                        break;
                    }
                    //delay(3);
                    usleep(3000);
                }
            }

            else if(atoi(argv[1]) == 5){
                            for(int k = 0; k < 256; k++) {

                    
                    for (size_t i = 0; i < NB_LEDS; i++)
                    {
                        /* code */
                        color_array[i] = 0x000000 + (k);
                    }
                    

                    if ((ret = ws2812b_write(&handle, color_array, NB_LEDS, temp, TEMP_ARRAY_SIZE)) != 0)
                    {
                        printf("ws2812b write failed: %d\n", ret);
                        break;
                    }
                    //delay(3);
                    usleep(3000);
                }	
                // Fade OUT
                for(int k = 255; k >= 0; k--) {
                
                    for (size_t i = 0; i < NB_LEDS; i++)
                    {
                        /* code */
                        color_array[i] = 0x000000 + (k);
                    }
                    

                    if ((ret = ws2812b_write(&handle, color_array, NB_LEDS, temp, TEMP_ARRAY_SIZE)) != 0)
                    {
                        printf("ws2812b write failed: %d\n", ret);
                        break;
                    }
                    //delay(3);
                    usleep(3000);
                }
            }

            else if(atoi(argv[1]) == 6){

                for(int i = 0; i < NB_LEDS-1-2; i++) {
                    for (size_t i = 0; i < NB_LEDS; i++)
                    {
                        /* code */
                        color_array[i] = 0;
                    }
                    color_array[i] = 0x20;
                    for(int j = 1; j <= 1; j++) {
                        color_array[i+j] = 0xFF;
                    }
                    color_array[i+1+1] = 0x20;
                    if ((ret = ws2812b_write(&handle, color_array, NB_LEDS, temp, TEMP_ARRAY_SIZE)) != 0)
                    {
                        printf("ws2812b write failed: %d\n", ret);
                        break;
                    }
                    usleep(30000);
                }
            }

            else if(atoi(argv[1]) == 7){

                for(int i = 0; i < NB_LEDS-1-2; i++) {
                    for (size_t i = 0; i < NB_LEDS; i++)
                    {
                        /* code */
                        color_array[i] = 0;
                    }
                    color_array[i] = 0x2000;
                    for(int j = 1; j <= 1; j++) {
                        color_array[i+j] = 0xFF00;
                    }
                    color_array[i+1+1] = 0x2000;
                    if ((ret = ws2812b_write(&handle, color_array, NB_LEDS, temp, TEMP_ARRAY_SIZE)) != 0)
                    {
                        printf("ws2812b write failed: %d\n", ret);
                        break;
                    }
                    usleep(30000);
                }
            }
            else if(atoi(argv[1]) == 8){

                for(int i = 0; i < NB_LEDS-1-2; i++) {
                    for (size_t i = 0; i < NB_LEDS; i++)
                    {
                        /* code */
                        color_array[i] = 0;
                    }
                    color_array[i] = 0x200000;
                    for(int j = 1; j <= 1; j++) {
                        color_array[i+j] = 0xFF0000;
                    }
                    color_array[i+1+1] = 0x200000;
                    if ((ret = ws2812b_write(&handle, color_array, NB_LEDS, temp, TEMP_ARRAY_SIZE)) != 0)
                    {
                        printf("ws2812b write failed: %d\n", ret);
                        break;
                    }
                    usleep(30000);
                }
            }
        }
        else{

                for (size_t i = 0; i < NB_LEDS; i++)
                {
                    /* code */
                    color_array[i] = 0x8040;
                }

                if ((ret = ws2812b_write(&handle, color_array, NB_LEDS, temp, TEMP_ARRAY_SIZE)) != 0)
                {
                    printf("ws2812b write failed: %d\n", ret);
                    break;
                }
                usleep(1000);
        }
    }
    return 0;
}
