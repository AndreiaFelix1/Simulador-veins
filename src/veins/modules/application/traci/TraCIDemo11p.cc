// TraCIDemo11p.cc
#include "veins/modules/application/traci/TraCIDemo11p.h"
#include "veins/modules/application/traci/TraCIDemo11pMessage_m.h"
#include <fstream>
#include <iomanip>
#include <sstream>

using namespace veins;

Define_Module(veins::TraCIDemo11p);

void TraCIDemo11p::initialize(int stage)
{
    DemoBaseApplLayer::initialize(stage);
    if (stage == 0) {
        sentMessage = false;
        lastDroveAt = simTime();
        currentSubscribedServiceId = -1;
        messageCounter = 0;
        beaconEvent = new cMessage("beacon evt");

        // Garantir que o ID do veículo está sendo atribuído corretamente
        myId = getId(); // Supondo que o método getId() seja utilizado para atribuir um ID único para cada veículo

        scheduleAt(simTime(), beaconEvent);
    }
}

std::ofstream& TraCIDemo11p::getFileForVehicle(int vehicleId)
{
    // Verifica se o arquivo já está no mapa
    if (vehicleFiles.find(vehicleId) == vehicleFiles.end()) {
        // Define o caminho do diretório onde os arquivos serão salvos
        std::string directory = "/home/andreia/projeto/workspace/veins/examples/Resultados_comunicacao/";

        // Cria o nome do arquivo com o caminho completo
        std::stringstream filename;
        filename << directory << "veiculo_" << vehicleId << ".txt";

        // Abre o arquivo no diretório especificado
        vehicleFiles[vehicleId].open(filename.str(), std::ios::out);

        // Se o arquivo foi aberto corretamente e está vazio, escreve o cabeçalho
        if (vehicleFiles[vehicleId].tellp() == 0) {
            vehicleFiles[vehicleId] << "ID do Veículo: ID da Mensagem: Tempo de Envio da Mensagem: ID do Remetente que Precisa Receber: 99999\n";
        }
    }

    return vehicleFiles[vehicleId];
}

void TraCIDemo11p::sendWSM()
{
    TraCIDemo11pMessage* wsm = new TraCIDemo11pMessage();
    wsm->setByteLength(82); // Ajustado para 82 bytes
    populateWSM(wsm);
    wsm->setSenderAddress(myId);

    // Usar apenas messageCounter para garantir um ID único
    wsm->setSerial(messageCounter++); // Apenas messageCounter é usado agora

    std::ofstream& file = getFileForVehicle(myId);
    file << "ID do Veículo: " << myId
         << " ID da Mensagem: " << wsm->getSerial()
         << " Tempo de Envio da Mensagem: " << std::fixed << std::setprecision(3) << simTime().dbl()
         << " ID do Remetente que Precisa Receber: 99999\n";

    sendDelayedDown(wsm, uniform(0.01, 0.2));

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

    // Verificar se o remetente é a RSU (ID 99999)
    if (wsm->getSenderAddress() == 99999) {
        // Calcular o tempo de recepção da mensagem no veículo (simTime é o tempo de simulação)
        double receptionTime = simTime().dbl();

        std::ofstream& file = getFileForVehicle(myId);
        file << "ID do No que Enviou a Mensagem: " << wsm->getSenderAddress()
             << " ID da Mensagem: " << wsm->getSerial()
             << " Tempo de Recebimento da Mensagem: " << std::fixed << std::setprecision(3) << receptionTime << "\n";
    }
}

void TraCIDemo11p::handleSelfMsg(cMessage* msg)
{
    if (msg == beaconEvent) {
        sendWSM();
    } else {
        DemoBaseApplLayer::handleSelfMsg(msg);
    }
}

void TraCIDemo11p::handlePositionUpdate(cObject* obj)
{
    DemoBaseApplLayer::handlePositionUpdate(obj);
}

void TraCIDemo11p::finish()
{
    // Fecha os arquivos abertos para cada veículo
    for (auto& pair : vehicleFiles) {
        if (pair.second.is_open()) {
            pair.second.close();
        }
    }

    if (beaconEvent != nullptr) {
        cancelAndDelete(beaconEvent);
        beaconEvent = nullptr;
    }
    DemoBaseApplLayer::finish();
}
