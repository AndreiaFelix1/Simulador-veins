#include "veins/modules/application/traci/TraCIDemoRSU11p.h"
#include "veins/modules/application/traci/TraCIDemo11pMessage_m.h"
#include <iostream> // Incluído para usar std::cout

using namespace veins;

Define_Module(veins::TraCIDemoRSU11p);

void TraCIDemoRSU11p::onWSA(DemoServiceAdvertisment* wsa)
{
    // Se esta RSU receber um WSA para o serviço 42, ela muda para o canal correspondente
    if (wsa->getPsid() == 42) {
        mac->changeServiceChannel(static_cast<Channel>(wsa->getTargetChannel()));
        std::cout << "[RSU] Mudou para o canal do serviço PSID 42. Canal: " << wsa->getTargetChannel() << std::endl;
    }
}

void TraCIDemoRSU11p::onWSM(BaseFrame1609_4* frame)
{
    TraCIDemo11pMessage* wsm = check_and_cast<TraCIDemo11pMessage*>(frame);

    // Log do pacote recebido
    std::cout << "[RSU] Recebeu WSM com ID: " << wsm->getId()
              << " do veículo com ID: " << wsm->getSenderAddress()
              << " no tempo: " << simTime() << std::endl;

    // Verifica o destinatário e cria uma cópia do pacote
    auto duplicatedWsm = wsm->dup();
    if (!duplicatedWsm) {
        std::cerr << "[RSU] Erro ao duplicar o pacote recebido!" << std::endl;
        return;
    }

    // Adiciona informações de depuração ao pacote duplicado
    duplicatedWsm->setSenderAddress(getParentModule()->getId()); // Define o ID da RSU como remetente
    std::cout << "[RSU] Criando resposta do pacote com ID: " << duplicatedWsm->getId()
              << ". Enviando com atraso." << std::endl;

    // Envia o pacote com atraso
    sendDelayedDown(duplicatedWsm, 1 + uniform(0.01, 0.1));

    // Log após envio
    std::cout << "[RSU] Pacote com ID: " << duplicatedWsm->getId()
              << " enviado para o canal com atraso." << std::endl;
}
