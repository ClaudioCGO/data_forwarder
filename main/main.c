#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2s_std.h"
#include "esp_log.h"

static const char *TAG = "EI_DATA_FORWARDER";

// Definições de Pinos para o ESP32-S3
#define I2S_WS          GPIO_NUM_15
#define I2S_SD          GPIO_NUM_17
#define I2S_SCK         GPIO_NUM_16
#define SAMPLE_RATE     16000
#define DMA_BUF_LEN     1024 

i2s_chan_handle_t rx_chan;

void init_inmp441(void) {
    // 1. Configuração do Canal
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
    ESP_ERROR_CHECK(i2s_new_channel(&chan_cfg, NULL, &rx_chan));

    // 2. Configuração do padrão I2S (Standard)
    i2s_std_config_t std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(SAMPLE_RATE),
        .slot_cfg = I2S_STD_MSB_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_32BIT, I2S_SLOT_MODE_STEREO), // Mude para STEREO,
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = I2S_SCK,
            .ws = I2S_WS,
            .dout = I2S_GPIO_UNUSED,
            .din = I2S_SD
        },
    };

    ESP_ERROR_CHECK(i2s_channel_init_std_pulse(rx_chan, &std_cfg));
    ESP_ERROR_CHECK(i2s_channel_enable(rx_chan));
    ESP_LOGI(TAG, "INMP441 pronto. Sample Rate: %dHz", SAMPLE_RATE);
}

void audio_capture_task(void *pvParameters) {
    int32_t *raw_buf = malloc(DMA_BUF_LEN * sizeof(int32_t));
    size_t bytes_read = 0;

    while (1) {
        // Leitura do hardware
        esp_err_t ret = i2s_channel_read(rx_chan, raw_buf, DMA_BUF_LEN * sizeof(int32_t), &bytes_read, portMAX_DELAY);
        
        if (ret == ESP_OK) {
            size_t samples_read = bytes_read / sizeof(int32_t);
            
            for (size_t i = 0; i < samples_read; i+=2) {
                
                // Lê os dados brutos dos dois microfones
                int32_t raw_L = raw_buf[i];
                int32_t raw_R = raw_buf[i+1];
                
                // Converte ambos para 16 bits (usando um shift de 14 para evitar clipping)
                int16_t sample_L = (int16_t)(raw_L >> 14);
                int16_t sample_R = (int16_t)(raw_R >> 14);

                // Soma e faz a média dos dois microfones (Downmix para Mono)
                int16_t mixed_sample = (sample_L + sample_R) / 2;
               
                // O Data Forwarder espera um valor por linha
                printf("%d\n", mixed_sample);
            }
        }
        // Pequena pausa para não travar o watchdog (opcional dependendo da prioridade)
        vTaskDelay(pdMS_TO_TICKS(1));
    }
    free(raw_buf);
}

void app_main(void) {
    init_inmp441();
    
    // Criamos a task em um núcleo específico para melhor performance de áudio
    xTaskCreatePinnedToCore(audio_capture_task, "audio_task", 4096, NULL, 5, NULL, 1);
}