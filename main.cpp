#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <filesystem>
#include <iomanip>

using namespace std;

// Struktury danych
struct Demand {
    int source = 0;
    int destination = 0;
    vector<double> bitrates;

    Demand() = default; // Domyślny konstruktor
    Demand(int src, int dest) : source(src), destination(dest) {}
};


struct Path {
    vector<int> links; // Ścieżka binarna
};

struct Network {
    int nodeCount = 0;
    int linkCount = 0;
    vector<vector<int>> distanceMatrix;
    vector<vector<vector<Path>>> routingPaths;

    Network() = default; // Domyślny konstruktor
    Network(int nodes, int links)
        : nodeCount(nodes), linkCount(links),
          distanceMatrix(nodes, vector<int>(nodes, 0)),
          routingPaths(nodes, vector<vector<Path>>(nodes)) {}
};


// Funkcja wczytująca topologię z pliku .net
Network loadNetwork(const string& filePath) {
    ifstream file(filePath);
    if (!file.is_open()) {
        cerr << "Nie można otworzyć pliku: " << filePath << endl;
        exit(EXIT_FAILURE);
    }

    Network network;
    file >> network.nodeCount >> network.linkCount;

    network.distanceMatrix.resize(network.nodeCount, vector<int>(network.nodeCount, 0));
    for (int i = 0; i < network.nodeCount; ++i) {
        for (int j = 0; j < network.nodeCount; ++j) {
            file >> network.distanceMatrix[i][j];
        }
    }

    file.close();
    return network;
}

// Funkcja wczytująca ścieżki routingu z pliku .pat
void loadPaths(const string& filePath, Network& network) {
    ifstream file(filePath);
    if (!file.is_open()) {
        cerr << "Nie można otworzyć pliku: " << filePath << endl;
        exit(EXIT_FAILURE);
    }

    int totalPaths;
    file >> totalPaths;

    // Poprawiona inicjalizacja rozmiaru routingPaths
    network.routingPaths.resize(network.nodeCount, vector<vector<Path>>(network.nodeCount));

    for (int src = 0; src < network.nodeCount; ++src) {
        for (int dest = 0; dest < network.nodeCount; ++dest) {
            for (int k = 0; k < 30; ++k) { // Zakładamy maksymalnie 30 ścieżek
                Path path;
                path.links.resize(network.linkCount);
                for (int l = 0; l < network.linkCount; ++l) {
                    file >> path.links[l];
                }
                if (k < 10) { // Wybieramy tylko pierwsze 10 ścieżek
                    network.routingPaths[src][dest].push_back(path);
                }
            }
        }
    }

    file.close();
}

// Funkcja wczytująca żądania z folderu w zadanym zakresie
vector<Demand> loadDemands(const string& folderPath, int startFile, int endFile) {
    vector<Demand> demands;
    Demand lastDemand; // Zmienna do przechowywania ostatniego żądania

    for (int i = startFile; i <= endFile; ++i) {
        string filePath = folderPath + "/" + to_string(i) + ".txt";
        ifstream file(filePath);
        if (!file.is_open()) {
            cerr << "Nie można otworzyć pliku: " << filePath << endl;
            continue;
        }

        Demand demand;
        string line;

        // Wczytaj źródło i cel
        if (getline(file, line)) {
            demand.source = stoi(line);
        }
        if (getline(file, line)) {
            demand.destination = stoi(line);
        }

        // Ignoruj trzecią linię (literę `b`)
        getline(file, line);

        // Wczytaj bitrate
        while (getline(file, line)) {
            try {
                demand.bitrates.push_back(stod(line)); // Konwersja string -> double
            } catch (const invalid_argument&) {
                cerr << "Nieprawidłowa wartość bitrate w pliku: " << filePath << endl;
            }
        }

        if (!demand.bitrates.empty()) {
            // Wyświetl pierwszy i ostatni bitrate z pliku
            cout << "Plik: " << filePath << "\n";
            cout << "Pierwszy bitrate: " << demand.bitrates.front()
                 << ", Ostatni bitrate: " << demand.bitrates.back() << "\n";
            lastDemand = demand; // Zapisz ostatnio wczytane żądanie
        } else {
            cout << "Plik: " << filePath << " - brak danych bitrate.\n";
        }

        demands.push_back(demand);
        file.close();
    }

    // Wyświetl wszystkie bitrate'y z ostatniego wczytanego pliku
    if (!lastDemand.bitrates.empty()) {
        cout << "\nFragment tabeli (bitrate'y z ostatniego pliku):\n";
        for (size_t i = 0; i < lastDemand.bitrates.size(); ++i) {
            cout << "Iteracja " << i + 1 << ": " << lastDemand.bitrates[i] << "\n";
        }
    }

    return demands;
}

// Funkcja main
int main() {
    string demandsFolder = "demands_0";
    int startFile = 0;
    int endFile = 150; // Możesz zmieniać zakres wczytywanych plików

    // Ustaw pełną precyzję dla wszystkich wyjść
    cout << std::fixed << std::setprecision(15);

    // Wczytaj żądania z folderu w zadanym zakresie
    vector<Demand> demands = loadDemands(demandsFolder, startFile, endFile);

    cout << "\nLiczba wczytanych żądań: " << demands.size() << "\n";
    for (const auto& demand : demands) {
        cout << "Źródło: " << demand.source << ", Cel: " << demand.destination << "\n";
        cout << "Liczba iteracji bitrate: " << demand.bitrates.size() << "\n";
    }

    return 0;
}

