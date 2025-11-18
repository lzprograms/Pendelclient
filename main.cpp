#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <string>
#include <cmath>

#pragma comment(lib, "ws2_32.lib")

// Liest genau eine Zeile ("\n") vom Server
std::string recvLine(SOCKET sock) {
    std::string message;
    char buffer[1024];

    while (true) {
        int bytes = recv(sock, buffer, sizeof(buffer), 0);
        if (bytes <= 0) return "";

        message.append(buffer, bytes);

        size_t pos = message.find('\n');
        if (pos != std::string::npos) {
            std::string line = message.substr(0, pos);
            message.erase(0, pos + 1);
            return line;
        }
    }
}

// Sendet eine Zeile + "\n"
void sendLine(SOCKET sock, const std::string &msg) {
    std::string m = msg + "\n";
    send(sock, m.c_str(), (int)m.size(), 0);
}

int main() {
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);

    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(8080);

    // -----------------------------------------
    // Benutzerdefinierte IP eingeben
    // -----------------------------------------
    std::string ip;
    std::cout << "IP-Adresse eingeben: ";
    std::cin >> ip;

    if (inet_pton(AF_INET, ip.c_str(), &serverAddr.sin_addr) <= 0) {
        std::cout << "Ungültige IP-Adresse!\n";
        return 1;
    }

    std::cout << "Verbinde zu " << ip << "...\n";

    if (connect(sock, (sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        std::cout << "Fehler: connect() fehlgeschlagen\n";
        return 1;
    }

    std::cout << "Verbunden. Starte P-Regler...\n";

    // ---------------------------
    // P-Regler Parameter
    // ---------------------------
    const int zielWinkel = 1200;   // Sollwert
    const float Kp = 5.0f;      // Verstärkung
    const float Kd = 0.01f;
    int aktuellerWinkel = 0;
    int aktuelleGeschwindigkeit = 0;

    while (true) {

        // 1) Winkel abfragen
        sendLine(sock, "getAngle");

        // 2) Antwort empfangen
       std::string angleStr = recvLine(sock);

        sendLine(sock, "getAngleVelocity");

        // 2) Antwort empfangen
        std::string angleVelStr = recvLine(sock);
        if (angleStr.empty()) {
            std::cout << "Server getrennt.\n";
            break;
        }

        aktuellerWinkel = std::stoi(angleStr);
        aktuelleGeschwindigkeit = std::stoi(angleVelStr);
        //std::cout << "Winkel: " << aktuellerWinkel <<
        //    "aktuelle Geschwindigkeit: " << aktuelleGeschwindigkeit << "\n";

        // 3) Fehler berechnen
        int error = zielWinkel - aktuellerWinkel;

        // 4) Stellgröße berechnen
        int u = (int)std::round(Kp * error - Kd * aktuelleGeschwindigkeit);

        // Begrenzen (optional)
        if (u > 150) u = 150;
        if (u < -150) u = -150;

        //std::cout << "Sende Stellwert: " << u << "\n";

        // 5) Setzkommando senden
        sendLine(sock, "setRelPos " + std::to_string(u));

        // 6) OK empfangen
        std::string resp = recvLine(sock);
        if (resp != "OK") {
            std::cout << "Warnung: Antwort vom Server: " << resp << "\n";
        }

        Sleep(100);  // 100ms Loop
    }

    closesocket(sock);
    WSACleanup();
    return 0;
}
