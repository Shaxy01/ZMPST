#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <filesystem>
#include <iomanip>
#include <algorithm> // Do sortowania

#ifdef _WIN32
#include <windows.h>
#endif

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

// Struktura reprezentująca kanał optyczny
struct OpticalChannel {
    int source;                 // Węzeł źródłowy
    int destination;            // Węzeł docelowy
    double capacity;            // Dostępna pojemność kanału
    bool active = false;        // Czy kanał jest aktywny
    vector<int> activeDemands;  // Lista żądań przypisanych do kanału

    OpticalChannel(int src, int dest, double cap)
        : source(src), destination(dest), capacity(cap), active(true) {}
};

struct Modulation {
    string type;       // Typ modulacji (np. QPSK, 16-QAM)
    double maxDistance; // Maksymalna odległość w km (0 = bez limitu)
    double bitrate;     // Bitrate w Gbps
    int slots;          // Wymagana liczba slotów

    Modulation(string t, double dist, double bit, int s)
        : type(t), maxDistance(dist), bitrate(bit), slots(s) {}
};

vector<Modulation> modulationTable = {
    {"QPSK", 0.0, 200.0, 6},      // QPSK: bez limitu odległości, 200 Gbps, 6 slotów
    {"8-QAM", 0.0, 400.0, 9},    // 8-QAM: bez limitu odległości, 400 Gbps, 9 slotów
    {"16-QAM", 800.0, 400.0, 6}, // 16-QAM: do 800 km, 400 Gbps, 6 slotów
    {"16-QAM", 1600.0, 600.0, 9},// 16-QAM: do 1600 km, 600 Gbps, 9 slotów
    {"32-QAM", 200.0, 800.0, 9}  // 32-QAM: do 200 km, 800 Gbps, 9 slotów
};

// Funkcja zachłanna do obsługi żądań
int greedyAllocation(const Network& network, vector<Demand>& demands, int iterations) {
    vector<OpticalChannel> channels; // Lista kanałów optycznych
    int totalTransceivers = 0;       // Liczba aktywnych transceiverów

    for (int iter = 0; iter < iterations; ++iter) {
        cout << "Iteracja: " << iter + 1 << "\n";

        // Aktualizacja pojemności kanałów w oparciu o aktualne bitrate żądań
        for (auto& channel : channels) {
            double usedCapacity = 0.0;
            for (int demandId : channel.activeDemands) {
                usedCapacity += demands[demandId].bitrates[iter];
            }
            channel.capacity = channel.capacity - usedCapacity;
        }

        // Usuń puste kanały
        channels.erase(remove_if(channels.begin(), channels.end(),
                                 [](const OpticalChannel& channel) {
                                     return channel.activeDemands.empty() || channel.capacity <= 0.0;
                                 }),
                       channels.end());

        // Obsługa żądań w iteracji
        for (size_t demandIdx = 0; demandIdx < demands.size(); ++demandIdx) {
            auto& demand = demands[demandIdx];
            double distance = network.distanceMatrix[demand.source][demand.destination];
            double bitrate = demand.bitrates[iter];
            bool allocated = false;

            // Znajdź odpowiedni format modulacji
            Modulation selectedModulation{"", 0.0, 0.0, 0};
            for (const auto& modulation : modulationTable) {
                if ((modulation.maxDistance == 0.0 || distance <= modulation.maxDistance) &&
                    bitrate <= modulation.bitrate) {
                    selectedModulation = modulation;
                    break;
                }
            }

            if (selectedModulation.type.empty()) {
                cerr << "Brak dostępnej modulacji dla żądania od " << demand.source
                     << " do " << demand.destination << " (bitrate: " << bitrate
                     << " Gbps, odległość: " << distance << " km)\n";
                continue;
            }

            // Próbuj przydzielić żądanie do istniejących kanałów
            for (auto& channel : channels) {
                if (channel.source == demand.source && channel.destination == demand.destination &&
                    channel.capacity >= bitrate) {
                    channel.capacity -= bitrate;
                    channel.activeDemands.push_back(demandIdx);
                    allocated = true;
                    break; // Jeśli znaleziono odpowiedni kanał, zakończ pętlę
                }
            }

            // Jeśli nie znaleziono kanału, utwórz nowy
            if (!allocated) {
                OpticalChannel newChannel(demand.source, demand.destination, selectedModulation.bitrate);
                newChannel.capacity -= bitrate;
                newChannel.activeDemands.push_back(demandIdx);
                channels.push_back(newChannel);
                totalTransceivers += 2; // Dodaj dwa nowe transceivery
            }
        }

        // Wyświetl liczbę aktywnych kanałów
        cout << "Liczba aktywnych kanałów po iteracji " << iter + 1 << ": " << channels.size() << "\n";
    }

    return totalTransceivers;
}

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

    network.routingPaths.resize(network.nodeCount, vector<vector<Path>>(network.nodeCount));

    for (int src = 0; src < network.nodeCount; ++src) {
        for (int dest = 0; dest < network.nodeCount; ++dest) {
            for (int k = 0; k < 30; ++k) {
                Path path;
                path.links.resize(network.linkCount);
                for (int l = 0; l < network.linkCount; ++l) {
                    file >> path.links[l];
                }
                if (k < 10) {
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
    Demand lastDemand;

    for (int i = startFile; i <= endFile; ++i) {
        string filePath = folderPath + "/" + to_string(i) + ".txt";
        ifstream file(filePath);
        if (!file.is_open()) {
            cerr << "Nie można otworzyć pliku: " << filePath << endl;
            continue;
        }

        Demand demand;
        string line;

        if (getline(file, line)) {
            demand.source = stoi(line);
        }
        if (getline(file, line)) {
            demand.destination = stoi(line);
        }

        getline(file, line);

        while (getline(file, line)) {
            try {
                demand.bitrates.push_back(stod(line));
            } catch (const invalid_argument&) {
                cerr << "Nieprawidłowa wartość bitrate w pliku: " << filePath << endl;
            }
        }

        if (!demand.bitrates.empty()) {
            cout << "Plik: " << filePath << "\n";
            cout << "Pierwszy bitrate: " << demand.bitrates.front()
                 << ", Ostatni bitrate: " << demand.bitrates.back() << "\n";
            lastDemand = demand;
        } else {
            cout << "Plik: " << filePath << " - brak danych bitrate.\n";
        }

        demands.push_back(demand);
        file.close();
    }

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

#ifdef _WIN32
    SetConsoleOutputCP(65001);
#endif

    string demandsFolder = "demands_0";
    int startFile = 0;
    int endFile = 275;
    int iterations = 288;

    cout << std::fixed << std::setprecision(15);

    vector<Demand> demands = loadDemands(demandsFolder, startFile, endFile);

    cout << "\nLiczba wczytanych żądań: " << demands.size() << "\n";
    for (const auto& demand : demands) {
        cout << "Start: " << demand.source << ", Cel: " << demand.destination << "\n";
        cout << "Liczba iteracji bitrate: " << demand.bitrates.size() << "\n";
    }

    string networkFile = "pol12.net";
    Network network = loadNetwork(networkFile);

    int totalTransceivers = greedyAllocation(network, demands, iterations);

    cout << "\nŚrednia liczba transceiverów: " << totalTransceivers/288 << "\n";

    return 0;
}
