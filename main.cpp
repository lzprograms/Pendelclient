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




    int acceleration = 0;
    int pos = 0;
    int targetPos = 2500;
    int posSpeed = 0;
    double angle = 0;
    int targetAngle = 1200;
    double angleVelocity = 0;
    double oldRawAngleVelocity = 0;
    int posError;

    float cPos = -2.5; //-2.7f;
    float cPosSpeed = 0.47f;//0.48f; //0.46f;//2.5f;//5.9f;
    float cAngle = 3000; //2040.0f;
    float cAngleVelocity = 100.0f;//46.0f;//48.01f;//-4050.0f;
    const float predictionFactor = 0.78f; // multiplies change in variables by factor
    //used to predict value in the future
    //proportion between time to recieve and process to send and drive motor

    while (true) {
        std::string command;
        if (angle > -100 && angle < 100) {
            command = "getAngle\ngetAngleVelocity\ngetPos\ngetSpeed\nsetAcceleration " + std::to_string(acceleration);
        }else {
            command = "getAngle\ngetAngleVelocity\ngetPos\ngetSpeed\nsetSpeed 0";
        }
        sendLine(sock, command);
        std::string angleStr = recvLine(sock);
        std::string angleVelocityStr = recvLine(sock);
        std::string posStr = recvLine(sock);
        std::string speedStr = recvLine(sock);

        posError -= pos;
        if (posError <  -15000) posError = -15000;
        if (posError >   15000) posError =  15000;

        if (angleStr.empty()) {
            std::cout << "Server getrennt.\n";
            break;
        }

        double newAngle = std::stoi(angleStr);
        angle = newAngle + ((angle+targetAngle)-newAngle) * predictionFactor; // predicted value when signal reaches Motor
        //angle = newAngle;
        angle = angle-targetAngle;
        //angle -= round(pos * 0.01);
        double newRawAngleVelocity = std::stoi(angleVelocityStr);
        newRawAngleVelocity = 0.9 * newRawAngleVelocity + 0.1 * oldRawAngleVelocity;
        angleVelocity = newRawAngleVelocity + (newRawAngleVelocity - oldRawAngleVelocity) * predictionFactor;
        oldRawAngleVelocity = newRawAngleVelocity;
        //angleVelocity = newAngleVelocity;
        pos = std::stoi(posStr);
        pos = pos - targetPos;

        double newPosSpeed = std::stoi(speedStr);
        posSpeed = posSpeed + (newPosSpeed - posSpeed) * predictionFactor;

        acceleration = pos                  * cPos ;
        acceleration += posSpeed            * cPosSpeed;
        acceleration += angle               * cAngle;
        acceleration += angleVelocity       * cAngleVelocity;
        //acceleration += posError            * cPosError         *multiplicator;

        acceleration;
    }

    closesocket(sock);
    WSACleanup();
    return 0;
}
