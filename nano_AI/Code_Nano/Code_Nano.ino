/* Edge Impulse ingestion SDK
 * Copyright (c) 2022 EdgeImpulse Inc.
 */

#define EIDSP_QUANTIZE_FILTERBANK   0

/* Includes ---------------------------------------------------------------- */
#include <PDM.h>
#include <FaultyFanDetector_inferencing.h>

/** Audio buffers, pointers and selectors */
typedef struct {
    int16_t *buffer;
    uint8_t buf_ready;
    uint32_t buf_count;
    uint32_t n_samples;
} inference_t;

static inference_t inference;
static signed short sampleBuffer[2048];
static bool debug_nn = false;

unsigned long derniere_inference = 0;
const unsigned long INTERVALLE_30_MIN = 30UL * 60UL * 1000UL; 
bool premiere_execution = true;

// Tableaux pour stocker les derniers résultats
float derniers_resultats[EI_CLASSIFIER_LABEL_COUNT] = {0};
#if EI_CLASSIFIER_HAS_ANOMALY == 1
float derniere_anomalie = 0.0;
#endif
// -------------------------------------------------------------------

/**
 * @brief      Arduino setup function
 */
void setup()
{
    // Port Série USB (pour le debug)
    Serial.begin(115200);
    
    Serial1.begin(115200); 


    
    Serial.println("Edge Impulse Inferencing Demo - Mode 30 min / UART");

    if (microphone_inference_start(EI_CLASSIFIER_RAW_SAMPLE_COUNT) == false) {
        ei_printf("ERR: Could not allocate audio buffer\r\n");
        return;
    }
}

void loop()
{
    // 1. ÉCOUTE DU PORT SÉRIE (RX/TX)
    if (Serial1.available() > 0) {
        char commande = Serial1.read();
        
        // Si on reçoit la lettre 'G' (ou 'g'), on envoie les données
        if (commande == 'G' || commande == 'g') {
            envoyer_donnees_uart();
        }
    }

    // 2. VÉRIFICATION DU MINUTEUR POUR L'INFÉRENCE (Toutes les 30 min)
    unsigned long temps_actuel = millis();
    if (premiere_execution || (temps_actuel - derniere_inference >= INTERVALLE_30_MIN)) {
        
        Serial.println("Démarrage de l'inférence planifiée...");
        executer_inference();
        
        derniere_inference = millis(); // On met à jour le chronomètre
        premiere_execution = false;
    }
}


void executer_inference() {
    bool m = microphone_inference_record();
    if (!m) {
        Serial.println("ERR: Failed to record audio...");
        return;
    }

    signal_t signal;
    signal.total_length = EI_CLASSIFIER_RAW_SAMPLE_COUNT;
    signal.get_data = &microphone_audio_signal_get_data;
    ei_impulse_result_t result = { 0 };

    EI_IMPULSE_ERROR r = run_classifier(&signal, &result, debug_nn);
    if (r != EI_IMPULSE_OK) {
        Serial.print("ERR: Failed to run classifier (");
        Serial.print(r);
        Serial.println(")");
        return;
    }

    // Sauvegarde et affichage USB des résultats
    Serial.println("Résultats de la nouvelle inférence :");
    for (size_t ix = 0; ix < EI_CLASSIFIER_LABEL_COUNT; ix++) {
        derniers_resultats[ix] = result.classification[ix].value;
        Serial.print("    ");
        Serial.print(result.classification[ix].label);
        Serial.print(": ");
        Serial.println(derniers_resultats[ix], 5);
    }
    
#if EI_CLASSIFIER_HAS_ANOMALY == 1
    derniere_anomalie = result.anomaly;
    Serial.print("    anomaly score: ");
    Serial.println(derniere_anomalie, 3);
#endif
}

void envoyer_donnees_uart() {
    // Format : Val1,Val2,Val3,Anomalie\n (Valeurs avec 2 décimales pour plus de compacité)
    for (size_t ix = 0; ix < EI_CLASSIFIER_LABEL_COUNT; ix++) {
        Serial1.print(derniers_resultats[ix], 2); 
        if (ix < EI_CLASSIFIER_LABEL_COUNT - 1) {
            Serial1.print(",");
        }
    }
    
#if EI_CLASSIFIER_HAS_ANOMALY == 1
    Serial1.print(",");
    Serial1.print(derniere_anomalie, 2);
#endif

    // Fin de ligne pour indiquer la fin du message
    Serial1.println(); 
}

--------------------------------------------------------------------------------

static void pdm_data_ready_inference_callback(void)
{
    int bytesAvailable = PDM.available();
    int bytesRead = PDM.read((char *)&sampleBuffer[0], bytesAvailable);

    if (inference.buf_ready == 0) {
        for(int i = 0; i < bytesRead>>1; i++) {
            inference.buffer[inference.buf_count++] = sampleBuffer[i];

            if(inference.buf_count >= inference.n_samples) {
                inference.buf_count = 0;
                inference.buf_ready = 1;
                break;
            }
        }
    }
}

static bool microphone_inference_start(uint32_t n_samples)
{
    inference.buffer = (int16_t *)malloc(n_samples * sizeof(int16_t));
    if(inference.buffer == NULL) { return false; }

    inference.buf_count  = 0;
    inference.n_samples  = n_samples;
    inference.buf_ready  = 0;

    PDM.onReceive(&pdm_data_ready_inference_callback);
    PDM.setBufferSize(4096);

    if (!PDM.begin(1, EI_CLASSIFIER_FREQUENCY)) {
        ei_printf("Failed to start PDM!");
        microphone_inference_end();
        return false;
    }

    PDM.setGain(127);
    return true;
}

static bool microphone_inference_record(void)
{
    inference.buf_ready = 0;
    inference.buf_count = 0;

    while(inference.buf_ready == 0) {
        delay(10);
    }
    return true;
}

static int microphone_audio_signal_get_data(size_t offset, size_t length, float *out_ptr)
{
    numpy::int16_to_float(&inference.buffer[offset], out_ptr, length);
    return 0;
}

static void microphone_inference_end(void)
{
    PDM.end();
    free(inference.buffer);
}

#if !defined(EI_CLASSIFIER_SENSOR) || EI_CLASSIFIER_SENSOR != EI_CLASSIFIER_SENSOR_MICROPHONE
#error "Invalid model for current sensor."
#endif