#include <iostream>
#include <fstream>
#include <vector>
#include <complex>
#include <stdexcept>

// Function to read DCA1000 binary data
std::vector<std::vector<std::complex<float>>> readDCA1000(const std::string& fileName, int numADCSamples, int numADCBits, int numRX, bool isReal) {
    // Open the binary file
    std::ifstream file(fileName, std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open file: " + fileName);
    }

    // Read the binary data into a vector
    file.seekg(0, std::ios::end);
    size_t fileSize = file.tellg();
    file.seekg(0, std::ios::beg);
    std::vector<short> adcData(fileSize / sizeof(short));
    file.read(reinterpret_cast<char*>(adcData.data()), fileSize);
    file.close();

    // If ADC bits are not 16, compensate for sign extension
    if (numADCBits != 16) {
        short l_max = (1 << (numADCBits - 1)) - 1;
        for (auto& sample : adcData) {
            if (sample > l_max) {
                sample -= (1 << numADCBits);
            }
        }
    }

    // Process data
    size_t numChirps;
    std::vector<std::complex<float>> LVDS;
    if (isReal) {
        numChirps = fileSize / (numADCSamples * numRX * sizeof(short));
        LVDS.resize(fileSize / sizeof(short));
        std::copy(adcData.begin(), adcData.end(), LVDS.begin());
    } else {
        numChirps = fileSize / (2 * numADCSamples * numRX * sizeof(short));
        LVDS.resize(fileSize / (2 * sizeof(short)));
        size_t counter = 0;
        for (size_t i = 0; i < fileSize / sizeof(short); i += 4) {
            LVDS[counter] = std::complex<float>(adcData[i], adcData[i + 2]);
            LVDS[counter + 1] = std::complex<float>(adcData[i + 1], adcData[i + 3]);
            counter += 2;
        }
    }

    // Organize data per receiver
    std::vector<std::vector<std::complex<float>>> adcDataPerRX(numRX, std::vector<std::complex<float>>(numChirps * numADCSamples));
    for (int rx = 0; rx < numRX; ++rx) {
        for (size_t chirp = 0; chirp < numChirps; ++chirp) {
            std::copy(LVDS.begin() + chirp * numADCSamples * numRX + rx * numADCSamples,
                      LVDS.begin() + chirp * numADCSamples * numRX + (rx + 1) * numADCSamples,
                      adcDataPerRX[rx].begin() + chirp * numADCSamples);
        }
    }

    return adcDataPerRX;
}

int main() {
    try {
        // Configuration parameters
        int numADCSamples = 256;
        int numADCBits = 16;
        int numRX = 4;
        bool isReal = false;
        std::string fileName = "E:\\ti\\adc_data.bin";

        // Read and process the data
        auto adcData = readDCA1000(fileName, numADCSamples, numADCBits, numRX, isReal);
        
        // Output the size of the processed data for verification
        std::cout << "Data read successfully.\n";
        std::cout << "Number of receivers: " << adcData.size() << "\n";
        std::cout << "Number of samples per receiver: " << adcData[0].size() << "\n";
        std::ofstream fout("E:\\ti\\adcdata.csv");
        for(const auto&elem:adcData)
        {
            for(int cnt=0;cnt<elem.size()-1;cnt++)
            {
                fout<<elem[cnt]<<',';
            }
            fout<<elem.back()<<'\n';
        }
    } catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << "\n";
    }

    return 0;
}
