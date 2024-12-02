#include <stdio.h>
#include <fstream>
#include <string>
#include <vector>
#include "edge-impulse-sdk/classifier/ei_run_classifier.h"

static float input_buf[EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE];

// Function to read features from file
bool read_features_from_file(const char* filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        printf("ERROR: Could not open file %s\n", filepath);
        return false;
    }

    std::string value;
    int i = 0;
    
    while (std::getline(file, value, ',') && i < EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE) {
        input_buf[i++] = std::stof(value);
    }

    file.close();

    if (i != EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE) {
        printf("ERROR: File contains wrong number of features. Expected %d, got %d\n", 
                EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE, i);
        return false;
    }

    return true;
}

static int get_signal_data(size_t offset, size_t length, float *out_ptr) {
    for (size_t i = 0; i < length; i++) {
        out_ptr[i] = (input_buf + offset)[i];
    }
    return EIDSP_OK;
}

int main(int argc, char **argv) {
    if (argc != 2) {
        printf("Usage: %s <features_file_path>\n", argv[0]);
        return 1;
    }

    if (!read_features_from_file(argv[1])) {
        return 1;
    }

    signal_t signal;
    ei_impulse_result_t result;
    EI_IMPULSE_ERROR res;

    signal.total_length = EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE;
    signal.get_data = &get_signal_data;

    res = run_classifier(&signal, &result, false);

    printf("run_classifier returned: %d\r\n", res);
    printf("Timing: DSP %d ms, inference %d ms, anomaly %d ms\r\n", 
            result.timing.dsp, 
            result.timing.classification, 
            result.timing.anomaly);

#if EI_CLASSIFIER_OBJECT_DETECTION == 1
    printf("Object detection bounding boxes:\r\n");
    for (uint32_t i = 0; i < EI_CLASSIFIER_OBJECT_DETECTION_COUNT; i++) {
        ei_impulse_result_bounding_box_t bb = result.bounding_boxes[i];
        if (bb.value == 0) continue;
        printf("  %s (%f) [ x: %u, y: %u, width: %u, height: %u ]\r\n", 
                bb.label, bb.value, bb.x, bb.y, bb.width, bb.height);
    }
#else
    printf("Predictions:\r\n");
    for (uint16_t i = 0; i < EI_CLASSIFIER_LABEL_COUNT; i++) {
        printf("  %s: ", ei_classifier_inferencing_categories[i]);
        printf("%.5f\r\n", result.classification[i].value);
    }
#endif

#if EI_CLASSIFIER_HAS_ANOMALY == 1
    printf("Anomaly prediction: %.3f\r\n", result.anomaly);
#endif

    return 0;
}
