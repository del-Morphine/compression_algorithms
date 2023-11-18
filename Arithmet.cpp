#include <iostream>
#include <fstream>
#include <unordered_map>
#include <vector>
#include <algorithm>
#include <cmath>

using namespace std;

unsigned index_from_cumFreqTable(const char& c, const vector<pair<char, int>>& cumFreq ) {
    for (unsigned i = 1; i < cumFreq.size(); i++) {
        if (cumFreq[i].first == c) {
            return i;
        }
    }
    return -1;
}

void write_bit(ofstream& outFile, const uint8_t& bit) {
    static uint8_t buffer = 0;
    static int num_bits_in_buffer = 0;

    buffer = (buffer << 1) | bit;
    num_bits_in_buffer++;
    if (num_bits_in_buffer == 8) {
        outFile.write(reinterpret_cast<char*>(&buffer), sizeof(buffer));
        buffer = 0;
        num_bits_in_buffer = 0;
    }
}

uint16_t read_bit(ifstream& inFile) {
    static uint8_t buffer = 0;
    static int num_bits_in_buffer = 7;
    inFile.read(reinterpret_cast<char*>(&buffer), sizeof(buffer));

    num_bits_in_buffer--;
    unsigned bit = buffer & (1 << num_bits_in_buffer);
    if (num_bits_in_buffer == 0) {
        buffer = 0;
        num_bits_in_buffer = 7;
    }
    return bit;
}

void add_bits_to_follow(ofstream& outFile, const uint8_t& bit, int& bits_to_follow) {
    write_bit(outFile, bit);
    for (int i = 0; bits_to_follow; bits_to_follow--) {
        write_bit(outFile, !bit);
    }

}

void arithmetic_decode(const string& input) {
    ifstream inFile(input, ios::binary);

    // Читаем количество интервалов 
    int intervalCount;
    inFile.read(reinterpret_cast<char*>(&intervalCount), sizeof(intervalCount));

    vector<pair<char, int>> cumFreq;
    cumFreq.push_back({'\0', 0});
    for (int i = 1; i < intervalCount; i++) {
        
        // Читаем символ
        char c;
        inFile.read(reinterpret_cast<char*>(&c), sizeof(c));
        
        // Читаем верхнюю границу символа
        int high;
        inFile.read(reinterpret_cast<char*>(&high), sizeof(high));
    
        cumFreq.push_back({c, high});
    }

    ofstream outFile("decoded.txt", ios::binary);
    uint16_t high = 65535;
    uint16_t low = 0;
    uint16_t first_qrt = 16384;
    uint16_t half = 32768;
    uint16_t third_qrt = 49152;
    uint16_t denom = cumFreq[cumFreq.size() - 1].second;
    int bits_to_follow = 0;

    uint16_t code;
    inFile.read(reinterpret_cast<char*>(&code), sizeof(code));
    while (!inFile.eof()) {
        int freq = ((code - low + 1) * denom - 1) / (high - low + 1);
        int index;
        for (index = 1; cumFreq[index].second <= freq; index++);

        low = low + cumFreq[index - 1].second * (high - low + 1) / denom;
        high = low + cumFreq[index].second * (high - low + 1) / denom - 1;
    
        while (true) {
            if (low >= half) {
                low -= half; high -= half; code -= half;
            } else if (low >= first_qrt && high < third_qrt) {
                low -= first_qrt; high -= first_qrt; code -= first_qrt;
            } else {
                break;
            }
            low += low; high += high + 1;
            code += code + read_bit(inFile);
        }
        char c = cumFreq[index].first;
        outFile.write(reinterpret_cast<const char*>(&c), sizeof(c));
    }
    inFile.close();
    outFile.close();
}

void arithmetic_encode(const string& input) {
    ifstream inFile(input, ios::binary);
    string text((istreambuf_iterator<char>(inFile)), (istreambuf_iterator<char>()));
    inFile.close();
    // Алфавит и частоты символов алфавита
    unordered_map<char, int> freqTable;
    for (char c : text) {
        freqTable[c]++;
    }

    // Вектор для представления кумулятивных частот
    vector<pair<char, int>> cumFreq;
    cumFreq.push_back({'\0', 0});
    for (auto pair : freqTable) {
        cumFreq.push_back({pair.first, pair.second});
    }

    // Формирование конечного вида "таблицы" кумулятивных частот
    sort(cumFreq.begin(), cumFreq.end(), [](const auto& el1, const auto& el2) {return el1.second < el2.second;});        
    for (int i = 1; i < cumFreq.size(); i++) {
        cumFreq[i].second += cumFreq[i - 1].second;
    }

    ofstream outFile("encoded.bin", ios::binary);

    // Запись размера таблицы частот
    int intervalCount = cumFreq.size();
    outFile.write(reinterpret_cast<const char*>(&intervalCount), sizeof(intervalCount));

    // Записываем интервалы
    for (int i = 1; i < intervalCount; i++) {
        
        // Запись символа
        char c = cumFreq[i].first;
        outFile.write(&c, sizeof(c));

        // Запись верхней границы
        int high = cumFreq[i].second;
        outFile.write(reinterpret_cast<const char*>(&high), sizeof(high));
    }

    uint16_t high = 65535;
    uint16_t low = 0;
    uint16_t first_qrt = 16384;
    uint16_t half = 32768;
    uint16_t third_qrt = 49152;
    uint16_t denom = cumFreq[cumFreq.size() - 1].second;
    int bits_to_follow = 0;
    
    for (char c : text) {
        int index = index_from_cumFreqTable(c, cumFreq);
        low = low + cumFreq[index - 1].second * (high - low + 1) / denom;
        high = low + cumFreq[index].second * (high - low + 1) / denom - 1;
    
        while (true) {
            if (high < half) {
                add_bits_to_follow(outFile, 0, bits_to_follow);
            } else if (low >= half) {
                add_bits_to_follow(outFile, 1, bits_to_follow);
                low -= half; high -= half;
            } else if (low >= first_qrt && high < third_qrt) {
                bits_to_follow++;
                low -= first_qrt; high -= first_qrt;
            } else {
                break;
            }
            low += low; high += high + 1;
        }
        
    }
    outFile.close();
}

int main() {
    string inputFile = "input.txt";
    string outputFile = "encoded.bin";

    arithmetic_encode(inputFile);
    arithmetic_decode(outputFile);
    return 0;
}
