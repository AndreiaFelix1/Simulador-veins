#include "veins/modules/application/traci/TraCIDemoRSU11p.h"
#include "veins/modules/application/traci/TraCIDemo11pMessage_m.h"
#include <fstream>
#include <iostream>  // Para usar std::cout

using namespace veins;

Define_Module(veins::TraCIDemoRSU11p);

void TraCIDemoRSU11p::initialize(int stage)
{
    DemoBaseApplLayer::initialize(stage);
    if (stage == 0) {
        std::cout << "RSU inicializada.\n";
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
    std::cout << "Mensagem recebida na RSU\n";

    TraCIDemo11pMessage* wsm = check_and_cast<TraCIDemo11pMessage*>(frame);

    if (wsm != nullptr) {
        std::cout << "ID da Mensagem Recebida pela RSU: " << wsm->getSerial() << "\n";

        // Abre o arquivo, escreve o registro e fecha
        std::ofstream file2("respostas_rsu.txt", std::ios::app); // Abre em modo de anexação
        if (file2.is_open()) {
            // Adiciona legenda no arquivo antes de salvar os dados (se for o primeiro registro)
            if (file2.tellp() == 0) {
                file2 << "ID da Mensagem Recebida pela RSU: Tempo de Recebimento: ID da Mensagem de Resposta Criado: Momento que Enviou\n";
            }

            file2 << "ID da Mensagem Recebida pela RSU: " << wsm->getSerial()
                  << " Tempo de Recebimento: " << simTime()
                  << " ID da Mensagem de Resposta Criado: " << wsm->getSerial() + 1
                  << " Momento que Enviou: " << simTime() + 0.2 << "\n";

            file2.close();
        } else {
            std::cerr << "Erro ao abrir o arquivo respostas_rsu.txt para escrita.\n";
        }
    } else {
        std::cout << "Erro ao receber a mensagem. Mensagem nula.\n";
    }

    TraCIDemo11pMessage* response = new TraCIDemo11pMessage();
    response->setByteLength(44);
    response->setSenderAddress(myId);
    response->setSerial(wsm->getSerial() + 1);

    std::cout << "Enviando resposta com serial: " << response->getSerial() << "\n";
    sendDelayedDown(wsm->dup(), uniform(0.01, 0.2));
}

void TraCIDemoRSU11p::finish()
{
    std::cout << "Finalizando RSU.\n";
    DemoBaseApplLayer::finish();
}
