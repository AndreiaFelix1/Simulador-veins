#include "veins/modules/application/traci/TraCIDemo11p.h"
#include "veins/modules/application/traci/TraCIDemo11pMessage_m.h"
#include <fstream>
#include <iomanip>

using namespace veins;

Define_Module(veins::TraCIDemo11p);

std::ofstream file1("envios.txt", std::ios::out);
std::ofstream file3("recepcao_veiculo.txt", std::ios::out);

// TraCIDemo11p.cc
void TraCIDemo11p::initialize(int stage)
{
    DemoBaseApplLayer::initialize(stage);
    if (stage == 0) {
        sentMessage = false;
        lastDroveAt = simTime();
        currentSubscribedServiceId = -1;

        // Inicializar o contador de mensagens para este veículo
        messageCounter = 0;

        // Inicializar o evento beaconEvent para o envio das mensagens
        beaconEvent = new cMessage("beacon evt");

        // Agendar o primeiro envio de mensagem assim que a simulação começar
        scheduleAt(simTime(), beaconEvent);  // Envio imediato no momento que o veículo entra na simulação
    }
}

void TraCIDemo11p::sendWSM()
{
    TraCIDemo11pMessage* wsm = new TraCIDemo11pMessage();
    wsm->setByteLength(62);
    populateWSM(wsm);
    wsm->setSenderAddress(myId);

    // Garantir ID único combinando myId e messageCounter
    wsm->setSerial(myId * 10000 + (messageCounter++)); // Combina myId com messageCounter

    // Registra apenas se a mensagem for enviada por um veículo (não RSU)
    if (myId != -1) { // Verifica se o 'myId' não é da RSU, considerando -1 como ID da RSU
        // Escrever legenda e informações no arquivo de envios com a formatação correta
        if (file1.tellp() == 0) { // Verifica se é o primeiro registro
            file1 << "ID do Veículo que Envia: ID da Mensagem: Momento de Envio\n";
        }
        file1 << "ID do Veículo que Envia: " << myId
              << " ID da Mensagem: " << wsm->getSerial()
              << " Momento de Envio: " << std::fixed << std::setprecision(3) << simTime().dbl() << "\n";
    }

    sendDown(wsm);

    // Agendar o próximo envio a cada 1 segundo
    scheduleAt(simTime() + 1.0, beaconEvent);
}

void TraCIDemo11p::onWSA(DemoServiceAdvertisment* wsa)
{
    if (currentSubscribedServiceId == -1) {
        mac->changeServiceChannel(static_cast<Channel>(wsa->getTargetChannel()));
        currentSubscribedServiceId = wsa->getPsid();
        if (currentOfferedServiceId != wsa->getPsid()) {
            stopService();
            startService(static_cast<Channel>(wsa->getTargetChannel()), wsa->getPsid(), "Mirrored Traffic Service");
        }
    }
}

void TraCIDemo11p::onWSM(BaseFrame1609_4* frame)
{
    TraCIDemo11pMessage* wsm = check_and_cast<TraCIDemo11pMessage*>(frame);

    // Escrever legenda e informações no arquivo de recepção de forma compacta
    if (file3.tellp() == 0) { // Verifica se é o primeiro registro
        file3 << "ID do Veículo que Recebeu: ID da Mensagem: Momento que Recebeu\n";
    }
    file3 << "ID do Veículo que Recebeu: " << myId
          << " ID da Mensagem: " << wsm->getSerial()
          << " Momento que Recebeu: " << simTime() << "\n";

    // Registrar que o veículo recebeu a mensagem
    // (Você pode adicionar outras lógicas de registro aqui, se necessário)

    // Criar mensagem de resposta
    TraCIDemo11pMessage* response = new TraCIDemo11pMessage();
    response->setByteLength(44);
    response->setSenderAddress(myId);

    // Garantir ID único para resposta, combinando myId e messageCounter
    response->setSerial(myId * 10000 + (messageCounter++));

    sendDown(response);
}

void TraCIDemo11p::handleSelfMsg(cMessage* msg)
{
    if (msg == beaconEvent) {
        sendWSM();
    }
    else {
        DemoBaseApplLayer::handleSelfMsg(msg);
    }
}

void TraCIDemo11p::handlePositionUpdate(cObject* obj)
{
    DemoBaseApplLayer::handlePositionUpdate(obj);
    // Handle vehicle position update if needed, you can modify this for additional logic
}

void TraCIDemo11p::finish()
{
    if (beaconEvent != nullptr) {
        cancelAndDelete(beaconEvent);
        beaconEvent = nullptr;
    }
    DemoBaseApplLayer::finish();
}
