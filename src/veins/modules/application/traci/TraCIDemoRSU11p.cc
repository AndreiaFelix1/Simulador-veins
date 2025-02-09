#include "veins/modules/application/traci/TraCIDemoRSU11p.h"
#include "veins/modules/application/traci/TraCIDemo11pMessage_m.h"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <iostream>  // Incluindo a biblioteca para usar std::cout

using namespace veins;

Define_Module(veins::TraCIDemoRSU11p);

// Variáveis globais para o arquivo e contador de mensagens
std::ofstream rsuFile;
int messageCounter = 0; // Adicionando o contador de mensagens
const int rsuId = 99999;  // Declarando o ID da RSU como 99999

void TraCIDemoRSU11p::initialize(int stage)
{
    DemoBaseApplLayer::initialize(stage);
    if (stage == 0) {
        // Definir o caminho do arquivo
        std::string directory = "/home/andreia/projeto/workspace/veins/examples/Resultados_comunicacao/";
        std::string filename = directory + "RSU_messages.txt";

        // Abrir o arquivo para a RSU
        rsuFile.open(filename, std::ios::out);

        // Escrever cabeçalho no arquivo
        if (rsuFile.tellp() == 0) {
            rsuFile << "ID do Veículo que Mandou a Mensagem: Tempo de Recebimento: Mensagem de Resposta: Tempo de Envio: \n";
        }

        // Imprimir o ID da RSU no console
        std::cout << "ID da RSU: " << rsuId << std::endl;
    }
}

void TraCIDemoRSU11p::finish()
{
    // Fecha o arquivo ao terminar a simulação
    if (rsuFile.is_open()) {
        rsuFile.close();
    }
    DemoBaseApplLayer::finish();
}

void TraCIDemoRSU11p::onWSA(DemoServiceAdvertisment* wsa)
{
    // if this RSU receives a WSA for service 42, it will tune to the chan
    if (wsa->getPsid() == 42) {
        mac->changeServiceChannel(static_cast<Channel>(wsa->getTargetChannel()));
    }
}

void TraCIDemoRSU11p::onWSM(BaseFrame1609_4* frame)
{
    TraCIDemo11pMessage* wsm = check_and_cast<TraCIDemo11pMessage*>(frame);

    // Criar resposta
    TraCIDemo11pMessage* response = new TraCIDemo11pMessage();
    response->setByteLength(43); // Ajustado para 43 bytes
    response->setSenderAddress(rsuId);  // ID da RSU
    response->setSerial(messageCounter++); // Contador de mensagens para gerar ID único

    // Calcular o tempo de envio da resposta
    double responseSendTime = simTime().dbl() + 2 + uniform(0.01, 0.2); // Tempo de envio com atraso

    // Log no arquivo da RSU, incluindo o ID da mensagem recebida
    rsuFile << "ID do Veículo que Mandou a Mensagem: " << wsm->getSenderAddress()
            << " ID da Mensagem Recebida: " << wsm->getSerial()  // Novo campo
            << " Tempo de Recebimento: " << std::fixed << std::setprecision(3) << simTime().dbl()
            << " Mensagem de Resposta: " << response->getSerial()
            << " Tempo de Envio: " << std::fixed << std::setprecision(3) << responseSendTime
            << "\n";

    // Enviar a resposta após o tempo calculado
    sendDelayedDown(response, responseSendTime - simTime().dbl()); // Enviar após o tempo de resposta
}



