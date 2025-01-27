#include "veins/modules/application/traci/TraCIDemo11p.h"
#include "veins/modules/application/traci/TraCIDemo11pMessage_m.h"
#include <iostream>
#include <fstream>  // Para manipulação de arquivos
#include <omnetpp.h>

using namespace veins;

Define_Module(veins::TraCIDemo11p);

const int VEHICLE_PACKET_SIZE = 62;
const int RSU_PACKET_SIZE = 44;
int TraCIDemo11p::nextPacketId = 1; // Inicializa o contador de IDs

void TraCIDemo11p::initialize(int stage) {
    DemoBaseApplLayer::initialize(stage);
    if (stage == 0) {
        sentMessage = false;
        lastDroveAt = simTime();
        currentSubscribedServiceId = -1;

//        std::cout << "Simulation Metrics Initialized.\n";
    }
}

void TraCIDemo11p::handleLowerMsg(cMessage* msg) {
    BaseFrame1609_4* wsm = dynamic_cast<BaseFrame1609_4*>(msg);
    ASSERT(wsm);

    if (TraCIDemo11pMessage* bsm = dynamic_cast<TraCIDemo11pMessage*>(wsm)) {
        vehiclePacketsReceived++;
        logReceivedPacket(getParentModule()->getId(), bsm->getPacketId(), simTime());

        // Gera uma resposta com um novo ID
        TraCIDemo11pMessage* response = new TraCIDemo11pMessage();
        response->setByteLength(RSU_PACKET_SIZE);
        response->setPacketId(nextPacketId++);

   //     std::cout << "RSU sending response with Packet ID: " << response->getId() << std::endl;

        sendDown(response);

        logPacketSent("RSU", response->getPacketId(), simTime().dbl(), response->getByteLength());
        logRSUPacketReceived(bsm->getPacketId(), simTime().dbl(), response->getPacketId(), simTime().dbl());
    }

    delete msg;
}

void TraCIDemo11p::handleSelfMsg(cMessage* msg) {
    switch (msg->getKind()) {
    case SEND_BEACON_EVT: {
        TraCIDemo11pMessage* beacon = new TraCIDemo11pMessage();
        populateWSM(beacon);
        beacon->setByteLength(VEHICLE_PACKET_SIZE);
        beacon->setPacketId(nextPacketId++);
        sendDown(beacon);

        vehiclePacketsSent++;
        logPacketSent("Vehicle", beacon->getPacketId(), simTime().dbl(), beacon->getByteLength());
        scheduleAt(simTime() + beaconInterval, sendBeaconEvt);
        break;
    }
    default:
        EV_WARN << "APP: Error: Got Self Message of unknown kind! Name: " << msg->getName() << endl;
        break;
    }
}

void TraCIDemo11p::onWSM(BaseFrame1609_4* frame) {
    std::cout << "onWSM called!" << std::endl;  // Log de depuração

    TraCIDemo11pMessage* wsm = check_and_cast<TraCIDemo11pMessage*>(frame);

    if (wsm->getByteLength() == VEHICLE_PACKET_SIZE) {
        std::cout << "Vehicle received packet ID: " << wsm->getId() << " at time: " << simTime().dbl() << std::endl;
        rsuPacketsReceived++;
        totalTransmissionTime += simTime().dbl() - wsm->getCreationTime().dbl();
        totalTransmissions++;

        TraCIDemo11pMessage* response = new TraCIDemo11pMessage();
        response->setByteLength(RSU_PACKET_SIZE);
        sendDown(response);
        rsuPacketsSent++;
        logPacketSent("RSU", response->getId(), simTime().dbl(), response->getByteLength());
        logReceivedPacket(getParentModule()->getId(), wsm->getId(), simTime());

        logRSUPacketReceived(wsm->getId(), simTime().dbl(), response->getId(), simTime().dbl());
    } else if (wsm->getByteLength() == RSU_PACKET_SIZE) {
        // Log para verificar o recebimento do pacote pela RSU
        std::cout << "RSU packet received by Vehicle with ID: " << wsm->getId()
                  << " at time: " << simTime().dbl() << std::endl;

        vehiclePacketsReceived++;
        logReceivedPacket(getParentModule()->getId(), wsm->getId(), simTime());
    } else {
        rsuPacketsLost++;
    }

    logMetrics();
    delete wsm;
}

void TraCIDemo11p::onWSA(DemoServiceAdvertisment* wsa) {
    if (currentSubscribedServiceId == -1) {
        mac->changeServiceChannel(static_cast<Channel>(wsa->getTargetChannel()));
        currentSubscribedServiceId = wsa->getPsid();
        if (currentOfferedServiceId != wsa->getPsid()) {
            stopService();
            startService(static_cast<Channel>(wsa->getTargetChannel()), wsa->getPsid(), "Mirrored Traffic Service");
        }
    }
}

void TraCIDemo11p::onBSM(DemoSafetyMessage* bsm) {
    std::cout << "onBSM called!" << std::endl;  // Log de depuração

    // Obter o ID do veículo que recebeu o pacote
    int vehicleId = getId();

    // Obter o ID do pacote recebido
    int packetId = bsm->getId();

    // Obter o tempo de recebimento
    simtime_t receiveTime = simTime();

    // Chama a função para registrar os dados do pacote
    logReceivedPacket(vehicleId, bsm->getPacketId(), simTime());

    // Criação da mensagem de resposta
    TraCIDemo11pMessage* response = new TraCIDemo11pMessage();

    // Define o tamanho do pacote para RSU
    response->setByteLength(RSU_PACKET_SIZE);
    populateWSM(response);  // Preenche os dados do pacote

    // Obtém o módulo de mobilidade da RSU
    TraCIMobility *mobility = check_and_cast<TraCIMobility*>(getParentModule()->getSubmodule("mobility"));

    if (mobility) {
        // Obtém a posição atual da RSU
        Coord position = mobility->getPositionAt(simTime());
        response->setSenderPos(position); // Isso deve ser substituído por um método válido, se necessário
    } else {
        EV << "Módulo de mobilidade não encontrado para a RSU!" << endl;
    }

    // Envia a resposta para o veículo
    sendDown(response);
    rsuPacketsSent++;  // Incrementa contagem de pacotes enviados pela RSU

    // Log para o pacote enviado pela RSU
    logPacketSent("RSU", response->getId(), simTime().dbl(), response->getByteLength());

    // Log geral de métricas
    logMetrics();

    // Log detalhado para o BSM recebido
    logRSUPacketReceived(bsm->getId(), simTime().dbl(), response->getId(), simTime().dbl());
}

void TraCIDemo11p::sendBeacon() {
    DemoSafetyMessage* beacon = new DemoSafetyMessage();
    populateWSM(beacon);  // Preenche os dados do pacote

    // Envia para a RSU ou para os veículos
    if (getParentModule()->getIndex() == 0) { // RSU
        beacon->setByteLength(RSU_PACKET_SIZE);
        beacon->setSenderId(getParentModule()->getIndex());
        sendDown(beacon);  // Envia para os veículos
        std::cout << "RSU sent beacon with ID: " << beacon->getId() << " at time: " << simTime().dbl() << std::endl;
    } else { // Veículo
        beacon->setByteLength(VEHICLE_PACKET_SIZE);
        beacon->setSenderId(getParentModule()->getIndex());
        sendDown(beacon);  // Envia para a RSU
        vehiclePacketsSent++;
        std::cout << "Vehicle sent beacon with ID: " << beacon->getId() << " at time: " << simTime().dbl() << std::endl;
    }
}

void TraCIDemo11p::handlePositionUpdate(cObject* obj) {
    DemoBaseApplLayer::handlePositionUpdate(obj);

    TraCIDemo11pMessage* wsm = new TraCIDemo11pMessage();
    wsm->setByteLength(VEHICLE_PACKET_SIZE);
    sendDown(wsm);
    vehiclePacketsSent++;
    logPacketSent("Vehicle", wsm->getId(), simTime().dbl(), wsm->getByteLength());
    logMetrics();
}

void TraCIDemo11p::logPacketSent(const std::string& sender, int packetId, double time, int byteLength) {
    std::ofstream outFile;
    std::string fileName;

    if (sender == "Vehicle") {
        fileName = "/home/vboxuser/projeto/workspace/vehicle_metrics.txt";
    } else if (sender == "RSU") {
        fileName = "/home/vboxuser/projeto/workspace/rsu_metrics.txt";
    } else {
        std::cerr << "Unknown sender type: " << sender << "\n";
        return;
    }

    int senderId = (sender == "Vehicle") ? getParentModule()->getIndex() : -1; // -1 para RSU

    outFile.open(fileName, std::ios::app);
    if (outFile.is_open()) {
        outFile << "Sender ID: " << senderId << " sent packet ID: " << packetId
                << " with size: " << byteLength << " bytes at time: " << time << "\n";
        outFile.close();
    } else {
        std::cerr << "Error opening file: " << fileName << "\n";
    }
}

void TraCIDemo11p::logReceivedPacket(int vehicleId, int packetId, simtime_t receiveTime) {
    // Arquivo para armazenar os dados dos pacotes recebidos
    std::ofstream logFile;
    logFile.open("/home/vboxuser/projeto/workspace/received_packets.log", std::ios_base::app);

    // Verifica se o arquivo foi aberto com sucesso
    if (logFile.is_open()) {
        logFile << "Vehicle ID: " << vehicleId
                << ", Packet ID: " << packetId
                << ", Receive Time: " << receiveTime.dbl() << " seconds\n";
        logFile.close(); // Fecha o arquivo após gravar os dados
    } else {
        std::cerr << "Error opening received_packets.log file!" << std::endl;
    }

}

void TraCIDemo11p::logRSUPacketReceived(int receivedPacketId, double receivedTime, int responsePacketId, double responseTime) {
    std::ofstream outFile("/home/vboxuser/projeto/workspace/rsu_metrics_received.txt", std::ios::app);
    if (outFile.is_open()) {
        outFile << "Received Packet ID: " << receivedPacketId << " at Time: " << receivedTime
                << " Responded with Packet ID: " << responsePacketId << " at Time: " << responseTime << "\n";
        outFile.close();
    } else {
        std::cerr << "Error opening file for RSU received packets log!" << std::endl;
    }
}

void TraCIDemo11p::logMetrics() {
    std::ofstream outFile("/home/vboxuser/projeto/workspace/metrics.txt", std::ios::app);
    if (outFile.is_open()) {
        outFile << "Simulation Metrics:\n"
                << "Sent by RSU: " << rsuPacketsSent << "\n"
                << "Received by RSU: " << rsuPacketsReceived << "\n"
                << "Sent by Vehicle: " << vehiclePacketsSent << "\n"
                << "Received by Vehicle: " << vehiclePacketsReceived << "\n"
                << "Total Packets Lost: " << rsuPacketsLost << "\n"
                << "Average Transmission Time: " << (totalTransmissionTime / totalTransmissions) << " seconds\n"
                << "--------------------------------------------\n";
        outFile.close();
    } else {
        std::cerr << "Error opening metrics log file!" << std::endl;
    }
}

void TraCIDemo11p::finish() {
    recordScalar("RSU Packets Sent", rsuPacketsSent);
    recordScalar("RSU Packets Received", rsuPacketsReceived);
    recordScalar("Vehicle Packets Sent", vehiclePacketsSent);
    recordScalar("Vehicle Packets Received", vehiclePacketsReceived);

 //   std::cout << "Simulation Finished!" << std::endl;
}

void TraCIDemo11p::checkAndTrackPacket(cMessage *msg) {
    // Exemplo de implementação: apenas um log simples
    EV << "Pacote recebido: " << msg->getName() << endl;

    // Aqui você pode adicionar lógica de rastreamento de pacotes, como contador ou verificação de tipo de pacote
}
