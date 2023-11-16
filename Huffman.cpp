#include <iostream>
#include <fstream>
#include <queue>
#include <unordered_map>
using namespace std;

struct Node {
    char data;
    unsigned frequency;
    Node* left, * right;
};

Node* createNode(char data, unsigned frequency, Node* left, Node* right) {
    Node* node = new Node();
    node->data = data;
    node->frequency = frequency;
    node->left = left;
    node->right = right;
    return node;
}

struct compare {
    bool operator()(Node* l, Node* r) {
        return l->frequency > r->frequency;
    }
};

void encode(Node* root, string code, unordered_map<char, string>& codeTable);
void decode(Node* root, string& encodedText, string& decodedText, int& index);
void encodeFile(string inputFile, string outputFile);
void decodeFile(string inputFile, string outputFile);

void encode(Node* root, string code, unordered_map<char, string>& codeTable) {
    if (root->left == nullptr && root->right == nullptr) {
        codeTable[root->data] = code;
        return;
    }
    encode(root->left, code + "0", codeTable);
    encode(root->right, code + "1", codeTable);
}

void decode(Node* root, string& encodedText, string& decodedText, int& index) {
    if (root == nullptr) {
        return;
    }
    if (root->left == nullptr && root->right == nullptr) {
        decodedText += root->data;
        return;
    }
    index++;
    if (encodedText[index] == '0') {
        decode(root->left, encodedText, decodedText, index);
    }
    else {
        decode(root->right, encodedText, decodedText, index);
    }
}

void encodeFile(string inputFile, string outputFile) {
    ifstream inFile(inputFile, ios::binary);
    ofstream outFile(outputFile, ios::binary);
    string text((istreambuf_iterator<char>(inFile)), (istreambuf_iterator<char>()));
    inFile.close();
    unordered_map<char, unsigned> frequencyTable;
    for (char c : text) {
        frequencyTable[c]++;
    }
    priority_queue<Node*, vector<Node*>, compare> pq;
    for (auto pair : frequencyTable) {
        pq.push(createNode(pair.first, pair.second, nullptr, nullptr));
    }
    while (pq.size() != 1) {
        Node* left = pq.top();
        pq.pop();
        Node* right = pq.top();
        pq.pop();
        unsigned sumFreq = left->frequency + right->frequency;
        pq.push(createNode('\0', sumFreq, left, right));
    }
    Node* root = pq.top();
    unordered_map<char, string> codeTable;
    string code;
    encode(root, code, codeTable);

    // Записываем размер таблицы кодов в байтах
    size_t tableSize = codeTable.size();
    outFile.write(reinterpret_cast<const char*>(&tableSize), sizeof(tableSize));

    // Записываем таблицу кодов
    for (auto pair : codeTable) {
        char symbol = pair.first;
        string code = pair.second;
        outFile.write(&symbol, sizeof(symbol));

        // Записываем размер кода в байтах
        size_t codeSize = code.size();
        outFile.write(reinterpret_cast<const char*>(&codeSize), sizeof(codeSize));

        // Записываем код в виде битов
        unsigned char bits = 0;
        int bitCount = 0;
        for (char c : code) {
            if (c == '1') {
                bits |= (1 << (7 - bitCount));
            }
            bitCount++;
            if (bitCount == 8) {
                outFile.write(reinterpret_cast<const char*>(&bits), sizeof(bits));
                bits = 0;
                bitCount = 0;
            }
        }
        if (bitCount > 0) {
            outFile.write(reinterpret_cast<const char*>(&bits), sizeof(bits));
        }
    }

    // Записываем закодированный текст
    string encodedText;
    for (char c : text) {
        encodedText += codeTable[c];
    }

    // Записываем размер закодированного текста в байтах
    size_t encodedSize = encodedText.size();
    outFile.write(reinterpret_cast<const char*>(&encodedSize), sizeof(encodedSize));

    // Записываем закодированный текст в виде битов
    unsigned char bits = 0;
    int bitCount = 0;
    for (char c : encodedText) {
        if (c == '1') {
            bits |= (1 << (7 - bitCount));
        }
        bitCount++;
        if (bitCount == 8) {
            outFile.write(reinterpret_cast<const char*>(&bits), sizeof(bits));
            bits = 0;
            bitCount = 0;
        }
    }
    if (bitCount > 0) {
        outFile.write(reinterpret_cast<const char*>(&bits), sizeof(bits));
    }

    outFile.close();
}

void decodeFile(string inputFile, string outputFile) {
    ifstream inFile(inputFile, ios::binary);
    ofstream outFile(outputFile);
    size_t tableSize;

    // Считываем размер таблицы кодов
    inFile.read(reinterpret_cast<char*>(&tableSize), sizeof(tableSize));

    unordered_map<string, char> codeTable;
    for (size_t i = 0; i < tableSize; i++) {
        char symbol;
        inFile.read(&symbol, sizeof(symbol));

        // Считываем размер кода
        size_t codeSize;
        inFile.read(reinterpret_cast<char*>(&codeSize), sizeof(codeSize));

        // Считываем код в виде битов
        string code;
        unsigned char bits;
        for (size_t j = 0; j < codeSize; j++) {
            if (j % 8 == 0) {
                inFile.read(reinterpret_cast<char*>(&bits), sizeof(bits));
            }
            if ((bits >> (7 - (j % 8))) & 1) {
                code += '1';
            }
            else {
                code += '0';
            }
        }
        codeTable[code] = symbol;
    }

    // Считываем размер закодированного текста
    size_t encodedSize;
    inFile.read(reinterpret_cast<char*>(&encodedSize), sizeof(encodedSize));

    // Считываем закодированный текст в виде битов
    string encodedBits;
    unsigned char bits;
    for (size_t i = 0; i < encodedSize; i++) {
        if (i % 8 == 0) {
            inFile.read(reinterpret_cast<char*>(&bits), sizeof(bits));
        }
        if ((bits >> (7 - (i % 8))) & 1) {
            encodedBits += '1';
        }
        else {
            encodedBits += '0';
        }
    }

    inFile.close();

    Node* root = createNode('\0', 0, nullptr, nullptr);
    for (auto pair : codeTable) {
        Node* curr = root;
        string code = pair.first;
        char symbol = pair.second;
        for (char c : code) {
            if (c == '0') {
                if (curr->left == nullptr) {
                    curr->left = createNode('\0', 0, nullptr, nullptr);
                }
                curr = curr->left;
            }
            else {
                if (curr->right == nullptr) {
                    curr->right = createNode('\0', 0, nullptr, nullptr);
                }
                curr = curr->right;
            }
        }
        curr->data = symbol;
    }

    string decodedText;
    int index = -1;
    while (index < (int)encodedBits.size() - 2) {
        decode(root, encodedBits, decodedText, index);
    }

    outFile << decodedText;
    outFile.close();
}

int main() {
    string inputFile = "input.txt";
    string encodedFile = "encoded.bin";
    string decodedFile = "decoded.txt";

    encodeFile(inputFile, encodedFile);
    cout << "Файл успешно закодирован." << endl;

    decodeFile(encodedFile, decodedFile);
    cout << "Файл успешно декодирован." << endl;

    return 0;
}