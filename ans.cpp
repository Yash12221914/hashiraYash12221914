#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <string>
#include <sstream>
#include <algorithm>
#include "json.hpp"

using json = nlohmann::json;
using namespace std;

const int PRIME = 2089;

// Convert string number in given base to int
int baseToInt(const string& str, int base) {
    int value = 0;
    for (char c : str) {
        int digit;
        if (isdigit(c)) digit = c - '0';
        else if (isalpha(c)) digit = tolower(c) - 'a' + 10;
        else throw invalid_argument("Invalid digit in base value");

        if (digit >= base)
            throw invalid_argument("Digit exceeds base");

        value = (value * base + digit) % PRIME;
    }
    return value;
}

// Modular inverse using Fermat's Little Theorem
int modInverse(int a) {
    int res = 1, b = PRIME - 2;
    while (b) {
        if (b & 1) res = (res * a) % PRIME;
        a = (a * a) % PRIME;
        b >>= 1;
    }
    return res;
}

// Lagrange Interpolation at x = 0
int interpolateAtZero(const vector<pair<int, int>>& shares) {
    int secret = 0;
    int sz = static_cast<int>(shares.size());
    for (int i = 0; i < sz; ++i) {
        int xi = shares[i].first;
        int yi = shares[i].second;
        int num = 1, den = 1;
        for (int j = 0; j < sz; ++j) {
            if (i == j) continue;
            int xj = shares[j].first;
            num = (num * (-xj + PRIME)) % PRIME;
            den = (den * (xi - xj + PRIME)) % PRIME;
        }
        int term = (yi * num % PRIME) * modInverse(den) % PRIME;
        secret = (secret + term) % PRIME;
    }
    return secret;
}

// Evaluate polynomial at given x using Lagrange interpolation
int evaluateAtX(const vector<pair<int, int>>& shares, int x) {
    int result = 0;
    int sz = static_cast<int>(shares.size());
    for (int i = 0; i < sz; ++i) {
        int xi = shares[i].first;
        int yi = shares[i].second;
        int num = 1, den = 1;
        for (int j = 0; j < sz; ++j) {
            if (i == j) continue;
            int xj = shares[j].first;
            num = (num * (x - xj + PRIME)) % PRIME;
            den = (den * (xi - xj + PRIME)) % PRIME;
        }
        int term = (yi * num % PRIME) * modInverse(den) % PRIME;
        result = (result + term) % PRIME;
    }
    return result;
}

int main() {
    ifstream inFile("input.json");
    if (!inFile) {
        cerr << "❌ Error: Could not open input.json" << endl;
        return 1;
    }

    json j;
    inFile >> j;

    int k = j["keys"]["k"];
    // int n = j["keys"]["n"]; // Unused — removed warning

    vector<pair<int, int>> allShares;

    for (auto& item : j.items()) {
        if (item.key() == "keys") continue;

        try {
            int x = stoi(item.key());
            int base = stoi(item.value()["base"].get<string>());
            string valueStr = item.value()["value"];
            int y = baseToInt(valueStr, base);
            allShares.emplace_back(x, y);
        } catch (...) {
            cout << "Invalid secret: failed to parse or convert one of the keys" << endl;
            return 1;
        }
    }

    if ((int)allShares.size() < k) {
        cout << "Not enough shares to reconstruct the secret" << endl;
        return 1;
    }

    // Try all combinations of k shares
    bool found = false;
    vector<int> indices(allShares.size());
    iota(indices.begin(), indices.end(), 0);

    vector<pair<int, int>> selected;
    int finalSecret = -1;

    do {
        selected.clear();
        for (int i = 0; i < k; ++i) {
            selected.push_back(allShares[indices[i]]);
        }

        int candidateSecret = interpolateAtZero(selected);

        // ✅ VALIDATION BLOCK FIXED — no structured binding
        bool valid = true;
        for (size_t i = 0; i < allShares.size(); ++i) {
            int x = allShares[i].first;
            int actualY = allShares[i].second;
            int expectedY = evaluateAtX(selected, x);
            if (expectedY != actualY) {
                valid = false;
                break;
            }
        }

        if (valid) {
            finalSecret = candidateSecret;
            found = true;
            break;
        }

    } while (next_permutation(indices.begin(), indices.end()));

    if (found) {
        cout << "✅ Secret key is: " << finalSecret << endl;
    } else {
        cout << "❌ Could not validate secret with any combination of shares" << endl;
    }

    return 0;
}
