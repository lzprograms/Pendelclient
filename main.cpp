#include <algorithm>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <string>
#include <cmath>

#pragma comment(lib, "ws2_32.lib")
std::string message;
// Liest genau eine Zeile ("\n") vom Server
std::string recvLine(SOCKET sock) {
    char buffer[1024];

    while (true) {
        size_t pos = message.find('\n');
        if (pos == std::string::npos) {
            int bytes = recv(sock, buffer, sizeof(buffer), 0);
            if (bytes <= 0) return "";
            message.append(buffer, bytes);
        }
        else{
            std::string line = message.substr(0, pos);
            message.erase(0, pos + 1);
            return line;
        }
    }
}

// Sendet eine Zeile + "\n"
void sendLine(SOCKET sock, const std::string &msg) {
    message = ""; // reset received Message
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



    int targetPos = 2500;
    int posSpeed = 0;
    double angle = 0;
    int targetAngle = 1202;
    double pos = 0;
    double u = 0;
    double angleVelocity = 0;
    double angleIntegral = 0;

    while (true) {
        std::string command;
        if (angle > -100 && angle < 100) {
            command = "getAngle\ngetAngleVelocity\ngetPos\ngetSpeed\nsetSpeed " + std::to_string(u);
        }else {
            command = "getAngle\ngetAngleVelocity\ngetPos\ngetSpeed\nsetSpeed 0";
        }
        sendLine(sock, command);
        std::string angleStr = recvLine(sock);
        std::string angleVelocityStr = recvLine(sock);
        std::string posStr = recvLine(sock);
        std::string speedStr = recvLine(sock);

        angle = atoi(angleStr.c_str());
        angle -= targetAngle;
        pos = atoi(posStr.c_str());
        pos -= targetPos;
        angleVelocity = atoi(angleVelocityStr.c_str());
        posSpeed = atoi(speedStr.c_str());

        angleIntegral += angle;
        std::clamp(angleIntegral, -20000.0, 20000.0);

        double kp = 620.0;
        double ki = 1.0;
        double kd = 4.0;
        double kdPos = 0.00095;
        posSpeed += pos;
        angle += posSpeed * kdPos;

        u = angle * kp + angleVelocity * ki + angleVelocity * kd;

    }
    closesocket(sock);
    WSACleanup();
    return 0;
}
