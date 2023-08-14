#include "ns3/ent-swap-adapt-local-app.h"

#include "ns3/quantum-basis.h"
#include "ns3/quantum-network-simulator.h" // class QuantumNetworkSimulator
#include "ns3/quantum-phy-entity.h" // class QuantumPhyEntity
#include "ns3/quantum-node.h" // class QuantumNode
#include "ns3/quantum-memory.h" // class QuantumMemory

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("EntSwapAdaptLocalApp");

NS_OBJECT_ENSURE_REGISTERED (EntSwapAdaptLocalApp);

EntSwapAdaptLocalApp::EntSwapAdaptLocalApp (Ptr<QuantumPhyEntity> qphyent_,
                                  Ptr<QuantumMemory> qubits_former_,
                                  Ptr<QuantumMemory> qubits_latter_)
    : m_qphyent (qphyent_),
      m_qubits_former (qubits_former_),
      m_qubits_latter (qubits_latter_)
{
}

EntSwapAdaptLocalApp::~EntSwapAdaptLocalApp ()
{
}

EntSwapAdaptLocalApp::EntSwapAdaptLocalApp ()
    : m_qphyent (nullptr), m_qubits_former (nullptr), m_qubits_latter (nullptr)
{
}

TypeId
EntSwapAdaptLocalApp::GetTypeId ()
{
  static TypeId tid =
      TypeId ("ns3::EntSwapAdaptLocalApp")
          .AddConstructor<EntSwapAdaptLocalApp> ()
          .SetParent<Application> ()
          .AddAttribute ("QPhyEntity", "The pointer to the quantum physical entity", PointerValue (),
                         MakePointerAccessor (&EntSwapAdaptLocalApp::m_qphyent),
                         MakePointerChecker<QuantumPhyEntity> ())
          .AddAttribute ("QubitsFormer", "The pointer to the former qubits of each owner",
                         PointerValue (),
                         MakePointerAccessor (&EntSwapAdaptLocalApp::m_qubits_former),
                         MakePointerChecker<QuantumMemory> ())
          .AddAttribute ("QubitsLatter", "The pointer to the latter qubits of each owner",
                         PointerValue (),
                         MakePointerAccessor (&EntSwapAdaptLocalApp::m_qubits_latter),
                         MakePointerChecker<QuantumMemory> ())
          ;
  return tid;
}

void
EntSwapAdaptLocalApp::EntanglementSwapping ()
{
  std::vector<std::complex<double>> unused;
  unsigned owners = m_qubits_former->GetSize ();

  // local operations
  for (unsigned rank = 1; rank < owners - 1; ++rank)
    {
      std::pair<std::string, std::string> qubits = {
        m_qubits_former->GetQubit (rank), m_qubits_latter->GetQubit (rank)};
      NS_LOG_LOGIC ("Owner " << rank << " has qubits " << qubits.first << " " << qubits.second);

      Simulator::ScheduleNow (&QuantumPhyEntity::ApplyGate, m_qphyent,
                              "God", QNS_GATE_PREFIX + "CNOT",
                              std::vector<std::complex<double>>{},
                              std::vector<std::string>{qubits.second, qubits.first});
      Simulator::ScheduleNow (&QuantumPhyEntity::ApplyGate, m_qphyent,
                              "God", QNS_GATE_PREFIX + "H",
                              std::vector<std::complex<double>>{},
                              std::vector<std::string>{qubits.first});
    }

  // God pass the xor result to every next owner's qubits
  for (unsigned rank = 1; rank < owners - 2; ++rank) {
      std::pair<std::string, std::string> qubits = {
        m_qubits_former->GetQubit (rank), m_qubits_latter->GetQubit (rank)};
      std::pair<std::string, std::string> qubits_next = {
        m_qubits_former->GetQubit (rank + 1), m_qubits_latter->GetQubit (rank + 1)};

      Simulator::ScheduleNow (&QuantumPhyEntity::ApplyGate, m_qphyent,
                              "God", QNS_GATE_PREFIX + "CNOT",
                              std::vector<std::complex<double>>{},
                              std::vector<std::string>{qubits_next.second, qubits.second});
      // the latter qubit not used anymore
      Simulator::ScheduleNow (&QuantumPhyEntity::PartialTrace, m_qphyent,
                                std::vector<std::string>{qubits.second}, unused);

      Simulator::ScheduleNow (&QuantumPhyEntity::ApplyGate, m_qphyent,
                              "God", QNS_GATE_PREFIX + "CNOT",
                              std::vector<std::complex<double>>{},
                              std::vector<std::string>{qubits_next.first, qubits.first});
      // the former qubit not used anymore
      Simulator::ScheduleNow (&QuantumPhyEntity::PartialTrace, m_qphyent,
                                std::vector<std::string>{qubits.first}, unused);
  }

  // between the last two owners
  std::pair<std::string, std::string> qubits = {
    m_qubits_former->GetQubit (owners - 2), m_qubits_latter->GetQubit (owners - 2)};
  std::string last_qubit = m_qubits_former->GetQubit (owners - 1);

  // God apply control operations with error equiv to that of PX / PZ gates
  Simulator::ScheduleNow (&QuantumPhyEntity::ApplyControledOperation, m_qphyent,
                          GetNode ()->GetObject<QuantumNode> ()->GetOwner (), QNS_GATE_PREFIX + "PX",
                          QNS_GATE_PREFIX + "CX", cnot,
                          std::vector<std::string>{qubits.second},
                          std::vector<std::string>{last_qubit});
  // the latter qubit of the last but one owner not used anymore
  Simulator::ScheduleNow (&QuantumPhyEntity::PartialTrace, m_qphyent,
                          std::vector<std::string>{qubits.second}, unused);    

  Simulator::ScheduleNow (&QuantumPhyEntity::ApplyControledOperation, m_qphyent,
                          GetNode ()->GetObject<QuantumNode> ()->GetOwner (), QNS_GATE_PREFIX + "PZ", 
                          QNS_GATE_PREFIX + "CZ", std::vector<std::complex<double>>{},
                          std::vector<std::string>{qubits.first},
                          std::vector<std::string>{last_qubit});
  // the former qubit of the last but one owner not used anymore
  Simulator::ScheduleNow (&QuantumPhyEntity::PartialTrace, m_qphyent,
                          std::vector<std::string>{qubits.first}, unused);
  Simulator::ScheduleNow (&QuantumPhyEntity::Contract, m_qphyent);
}

void
EntSwapAdaptLocalApp::StartApplication ()
{
  Simulator::ScheduleNow (&EntSwapAdaptLocalApp::EntanglementSwapping, this);
}

} // namespace ns3