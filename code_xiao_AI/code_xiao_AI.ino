/* Edge Impulse Arduino examples
 * Copyright (c) 2022 EdgeImpulse Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.

 * Modified 03/2026 for SmartGreenhouse project (github.com/Vinh1VT)
 */

/* Includes ---------------------------------------------------------------- */
#include <Plant_Disease_inferencing.h>
#include "edge-impulse-sdk/dsp/image/image.hpp"
#include "esp_camera.h"
#include <Wire.h>

// ============================================================================
// ======================== CONFIGURATION UTILISATEUR =========================
// ============================================================================

// --- Configuration I2C ---
#define I2C_SLAVE_ADDR      0x08    // Adresse I2C de cet ESP32S3 (Esclave)
#define I2C_SDA_PIN         5       // Pin SDA (Ex: 5 correspond à D4 sur XIAO ESP32S3)
#define I2C_SCL_PIN         6       // Pin SCL (Ex: 6 correspond à D5 sur XIAO ESP32S3)
#define I2C_CLOCK_SPEED     100000  // Vitesse du bus I2C en Hz (100000 = Standard mode)

// --- Configuration Deep Sleep & Réveil ---
#define WAKEUP_PIN          GPIO_NUM_1 // Pin GPIO utilisée pour réveiller la carte (Ex: GPIO_NUM_1 pour D1)
#define WAKEUP_LEVEL        0          // Etat qui déclenche le réveil : 0 (GND/LOW) ou 1 (3.3V/HIGH)
#define MASTER_TIMEOUT_MS   10000      // Temps d'attente max en ms (ex: 10 sec) pour la lecture I2C avant de forcer le sommeil

// --- Configuration Caméra (XIAO ESP32S3 Sense) ---
#define CAMERA_MODEL_XIAO_ESP32S3 
#define PWDN_GPIO_NUM       -1
#define RESET_GPIO_NUM      -1
#define XCLK_GPIO_NUM       10
#define SIOD_GPIO_NUM       40
#define SIOC_GPIO_NUM       39
#define Y9_GPIO_NUM         48
#define Y8_GPIO_NUM         11
#define Y7_GPIO_NUM         12
#define Y6_GPIO_NUM         14
#define Y5_GPIO_NUM         16
#define Y4_GPIO_NUM         18
#define Y3_GPIO_NUM         17
#define Y2_GPIO_NUM         15
#define VSYNC_GPIO_NUM      38
#define HREF_GPIO_NUM       47
#define PCLK_GPIO_NUM       13
#define LED_GPIO_NUM        21

// ============================================================================
// ======================== FIN DE LA CONFIGURATION ===========================
// ============================================================================


/* Constant defines -------------------------------------------------------- */
#define EI_CAMERA_RAW_FRAME_BUFFER_COLS           320
#define EI_CAMERA_RAW_FRAME_BUFFER_ROWS           240
#define EI_CAMERA_FRAME_BYTE_SIZE                 3

/* Private variables ------------------------------------------------------- */
static bool debug_nn = false;
static bool is_initialised = false;
uint8_t *snapshot_buf; 

volatile uint8_t inference_result = 255; // Stockera l'ID de la classe détectée (255 = erreur/non défini)
volatile bool data_requested = false;    // Passe à true quand le Master a lu la donnée

static camera_config_t camera_config = {
    .pin_pwdn = PWDN_GPIO_NUM,
    .pin_reset = RESET_GPIO_NUM,
    .pin_xclk = XCLK_GPIO_NUM,
    .pin_sscb_sda = SIOD_GPIO_NUM,
    .pin_sscb_scl = SIOC_GPIO_NUM,
    .pin_d7 = Y9_GPIO_NUM,
    .pin_d6 = Y8_GPIO_NUM,
    .pin_d5 = Y7_GPIO_NUM,
    .pin_d4 = Y6_GPIO_NUM,
    .pin_d3 = Y5_GPIO_NUM,
    .pin_d2 = Y4_GPIO_NUM,
    .pin_d1 = Y3_GPIO_NUM,
    .pin_d0 = Y2_GPIO_NUM,
    .pin_vsync = VSYNC_GPIO_NUM,
    .pin_href = HREF_GPIO_NUM,
    .pin_pclk = PCLK_GPIO_NUM,
    .xclk_freq_hz = 20000000,
    .ledc_timer = LEDC_TIMER_0,
    .ledc_channel = LEDC_CHANNEL_0,
    .pixel_format = PIXFORMAT_JPEG, 
    .frame_size = FRAMESIZE_QVGA,   
    .jpeg_quality = 12, 
    .fb_count = 1,       
    .fb_location = CAMERA_FB_IN_PSRAM,
    .grab_mode = CAMERA_GRAB_WHEN_EMPTY,
};

/* Function definitions ------------------------------------------------------- */
bool ei_camera_init(void);
void ei_camera_deinit(void);
bool ei_camera_capture(uint32_t img_width, uint32_t img_height, uint8_t *out_buf);
static int ei_camera_get_data(size_t offset, size_t length, float *out_ptr);
void run_inference_logic();
void goToDeepSleep();

/**
* @brief Callback I2C quand le Master vient lire
*/
void requestEvent() {
    Wire.write(inference_result); // On envoie l'ID de la maladie/plante
    data_requested = true;        // On signale qu'on peut aller dormir
}

/**
* @brief      Arduino setup function
*/
void setup() {
    Serial.begin(115200);

    Serial.println("\n--- Wake Up ---");

    // 1. Initialiser la caméra
    if (ei_camera_init() == false) {
        ei_printf("Failed to initialize Camera!\r\n");
        goToDeepSleep(); // S'il y a une erreur critique, on se rendort
    }

    // 2. Prendre une photo et faire l'inférence
    run_inference_logic();

    // 3. Eteindre la caméra pour économiser l'énergie pendant l'attente I2C
    ei_camera_deinit();

    // 4. Initialiser l'I2C en mode Slave
    Wire.onReceive(NULL); // On ne reçoit rien du Master, on fait juste du TX
    Wire.onRequest(requestEvent); // Fonction appelée quand le master fait une requête de lecture
    Wire.begin((uint8_t)I2C_SLAVE_ADDR, I2C_SDA_PIN, I2C_SCL_PIN, I2C_CLOCK_SPEED);
    
    Serial.printf("Ready to send result (Class ID: %d) over I2C.\n", inference_result);
    Serial.println("Waiting for Master to read...");

    // 5. Attendre que le Master vienne lire la donnée ou timeout
    unsigned long start_time = millis();
    while (!data_requested && (millis() - start_time < MASTER_TIMEOUT_MS)) {
        delay(10); // Petit délai pour ne pas bloquer le watchdog
    }

    if (data_requested) {
        Serial.println("Data successfully sent to Master!");
    } else {
        Serial.println("Timeout: Master didn't request data.");
    }

    // 6. Retourner en Deep Sleep
    goToDeepSleep();
}

/**
* @brief La boucle loop est vide car la carte est soit en Setup, soit en Deep Sleep
*/
void loop() {
}

/**
* @brief      Logique principale de capture et d'inférence
*/
void run_inference_logic() {
    snapshot_buf = (uint8_t*)malloc(EI_CAMERA_RAW_FRAME_BUFFER_COLS * EI_CAMERA_RAW_FRAME_BUFFER_ROWS * EI_CAMERA_FRAME_BYTE_SIZE);

    if(snapshot_buf == nullptr) {
        ei_printf("ERR: Failed to allocate snapshot buffer!\n");
        return;
    }

    ei::signal_t signal;
    signal.total_length = EI_CLASSIFIER_INPUT_WIDTH * EI_CLASSIFIER_INPUT_HEIGHT;
    signal.get_data = &ei_camera_get_data;

    if (ei_camera_capture((size_t)EI_CLASSIFIER_INPUT_WIDTH, (size_t)EI_CLASSIFIER_INPUT_HEIGHT, snapshot_buf) == false) {
        ei_printf("Failed to capture image\r\n");
        free(snapshot_buf);
        return;
    }

    // Lancer le classifieur Edge Impulse
    ei_impulse_result_t result = { 0 };
    EI_IMPULSE_ERROR err = run_classifier(&signal, &result, debug_nn);
    if (err != EI_IMPULSE_OK) {
        ei_printf("ERR: Failed to run classifier (%d)\n", err);
        free(snapshot_buf);
        return;
    }

    // Trouver la prédiction avec le score le plus haut
    float max_score = 0.0;
    uint8_t best_class_id = 0;

    for (size_t ix = 0; ix < EI_CLASSIFIER_LABEL_COUNT; ix++) {
        ei_printf("    %s: %.5f\n", result.classification[ix].label, result.classification[ix].value);
        if (result.classification[ix].value > max_score) {
            max_score = result.classification[ix].value;
            best_class_id = ix;
        }
    }

    // On stocke l'ID de la meilleure classe
    inference_result = best_class_id; 

    free(snapshot_buf);
}

/**
* @brief Prépare le pin de réveil et lance le deep sleep
*/
void goToDeepSleep() {
    Serial.println("Going to deep sleep...");
    Wire.end(); // Fermer le bus I2C proprement
    
    // Configurer le réveil (ext0) via la pin choisie
    esp_sleep_enable_ext0_wakeup(WAKEUP_PIN, WAKEUP_LEVEL); 
    
    // Dodo !
    esp_deep_sleep_start();
}


// ============== FONCTIONS CAMERA EDGE IMPULSE (INCHANGÉES) ============== //

bool ei_camera_init(void) {
    if (is_initialised) return true;
    esp_err_t err = esp_camera_init(&camera_config);
    if (err != ESP_OK) {
      Serial.printf("Camera init failed with error 0x%x\n", err);
      return false;
    }
    sensor_t * s = esp_camera_sensor_get();
    if (s->id.PID == OV3660_PID) {
      s->set_vflip(s, 1);
      s->set_brightness(s, 1);
      s->set_saturation(s, 0);
    }
    is_initialised = true;
    return true;
}

void ei_camera_deinit(void) {
    esp_err_t err = esp_camera_deinit();
    if (err != ESP_OK) {
        ei_printf("Camera deinit failed\n");
        return;
    }
    is_initialised = false;
    return;
}

bool ei_camera_capture(uint32_t img_width, uint32_t img_height, uint8_t *out_buf) {
    bool do_resize = false;
    if (!is_initialised) return false;
    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb) return false;
    bool converted = fmt2rgb888(fb->buf, fb->len, PIXFORMAT_JPEG, snapshot_buf);
    esp_camera_fb_return(fb);
    if(!converted) return false;
    if ((img_width != EI_CAMERA_RAW_FRAME_BUFFER_COLS) || (img_height != EI_CAMERA_RAW_FRAME_BUFFER_ROWS)) {
        do_resize = true;
    }
    if (do_resize) {
        ei::image::processing::crop_and_interpolate_rgb888(out_buf, EI_CAMERA_RAW_FRAME_BUFFER_COLS, EI_CAMERA_RAW_FRAME_BUFFER_ROWS, out_buf, img_width, img_height);
    }
    return true;
}

static int ei_camera_get_data(size_t offset, size_t length, float *out_ptr) {
    size_t pixel_ix = offset * 3;
    size_t pixels_left = length;
    size_t out_ptr_ix = 0;
    while (pixels_left != 0) {
        out_ptr[out_ptr_ix] = (snapshot_buf[pixel_ix] << 16) + (snapshot_buf[pixel_ix + 1] << 8) + snapshot_buf[pixel_ix + 2];
        out_ptr_ix++;
        pixel_ix+=3;
        pixels_left--;
    }
    return 0;
}