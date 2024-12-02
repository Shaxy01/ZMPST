#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <filesystem>

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

    for (int i = startFile; i <= endFile; ++i) {
        string filePath = folderPath + "/" + to_string(i) + ".txt";
        ifstream file(filePath);
        if (!file.is_open()) {
            cerr << "Nie można otworzyć pliku: " << filePath << endl;
            continue;
        }

        Demand demand;
        file >> demand.source >> demand.destination;

        double bitrate;
        while (file >> bitrate) {
            demand.bitrates.push_back(bitrate);
        }

        demands.push_back(demand);
        file.close();
    }

    return demands;
}

// Funkcja main
int main() {
    string demandsFolder = "demands_0";
    int startFile = 0;
    int endFile = 150; // Możesz zmieniać zakres wczytywanych plików

    // Wczytaj żądania z folderu w zadanym zakresie
    vector<Demand> demands = loadDemands(demandsFolder, startFile, endFile);

    // Debug: Wyświetlanie danych
    cout << "Liczba wczytanych żądań: " << demands.size() << "\n";
    for (const auto& demand : demands) {
        cout << "Źródło: " << demand.source << ", Cel: " << demand.destination << "\n";
        cout << "Bitrate: ";
        for (double b : demand.bitrates) {
            cout << b << " ";
        }
        cout << "\n";
    }

    // TODO: Implementacja algorytmu zachłannego
    //działaaaaaa

    return 0;
}
