#include "veins/modules/application/traci/TraCIDemoRSU11p.h"
#include "veins/modules/application/traci/TraCIDemo11pMessage_m.h"
#include <fstream>
#include <iostream>  // Para usar std::cout

using namespace veins;

Define_Module(veins::TraCIDemoRSU11p);

// Arquivo para registrar respostas enviadas pelo RSU
std::ofstream file2;

void TraCIDemoRSU11p::initialize(int stage)
{
    DemoBaseApplLayer::initialize(stage);
    if (stage == 0) {
        // Tenta abrir o arquivo para registrar as respostas
        file2.open("respostas_rsu.txt", std::ios::out);
        if (!file2.is_open()) {
            std::cerr << "Erro ao abrir o arquivo respostas_rsu.txt para escrita.\n";
        } else {
            std::cout << "Arquivo respostas_rsu.txt aberto com sucesso.\n";
        }
    }
}

void TraCIDemoRSU11p::onWSA(DemoServiceAdvertisment* wsa)
{
    // Se este RSU receber um WSA para o serviço 42, ele irá ajustar para o canal alvo
    if (wsa->getPsid() == 42) {
        mac->changeServiceChannel(static_cast<Channel>(wsa->getTargetChannel()));
    }
}

void TraCIDemoRSU11p::onWSM(BaseFrame1609_4* frame)
{
    // Verificação para ver se a função está sendo chamada
    std::cout << "Mensagem recebida na RSU\n";  // Verificação

    TraCIDemo11pMessage* wsm = check_and_cast<TraCIDemo11pMessage*>(frame);

    if (wsm != nullptr) {
        std::cout << "ID da Mensagem Recebida pela RSU: " << wsm->getSerial() << "\n";

        // Adiciona legenda no arquivo antes de salvar os dados (se for o primeiro registro)
        if (file2.tellp() == 0) {
            file2 << "ID da Mensagem Recebida pela RSU: Tempo de Recebimento: ID da Mensagem de Resposta Criado: Momento que Enviou\n";
        }

        // Escreve os dados recebidos no arquivo em uma única linha
        file2 << "ID da Mensagem Recebida pela RSU: " << wsm->getSerial()
              << " Tempo de Recebimento: " << simTime()
              << " ID da Mensagem de Resposta Criado: " << wsm->getSerial() + 1
              << " Momento que Enviou: " << simTime() + 0.2 << "\n";
    } else {
        std::cout << "Erro ao receber a mensagem. Mensagem nula.\n";
    }

    // Criar uma mensagem de resposta de 44 bytes
    TraCIDemo11pMessage* response = new TraCIDemo11pMessage();
    response->setByteLength(44);
    response->setSenderAddress(myId);
    response->setSerial(wsm->getSerial() + 1);

    // Envia a resposta para o veículo sem atraso
    std::cout << "Enviando resposta com serial: " << response->getSerial() << "\n";
    sendDown(response);  // Envia a resposta imediatamente
}

void TraCIDemoRSU11p::finish()
{
    // Verifica se o arquivo foi aberto e o fecha corretamente
    if (file2.is_open()) {
        file2.close();
        std::cout << "Arquivo respostas_rsu.txt fechado com sucesso.\n";
    } else {
        std::cerr << "O arquivo respostas_rsu.txt não foi aberto corretamente.\n";
    }

    DemoBaseApplLayer::finish();
}
