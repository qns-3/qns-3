#include "ns3/ent-swap-adapt-app.h"

#include "ns3/quantum-basis.h"
#include "ns3/quantum-network-simulator.h" // class QuantumNetworkSimulator
#include "ns3/quantum-phy-entity.h" // class QuantumPhyEntity
#include "ns3/quantum-node.h" // class QuantumNode
#include "ns3/quantum-memory.h" // class QuantumMemory

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("EntSwapAdaptApp");

NS_OBJECT_ENSURE_REGISTERED (EntSwapAdaptApp);

EntSwapAdaptApp::EntSwapAdaptApp (Ptr<QuantumPhyEntity> qphyent_,
                                  Ptr<QuantumMemory> qubits_former_,
                                  Ptr<QuantumMemory> qubits_latter_)
    : m_qphyent (qphyent_),
      m_qubits_former (qubits_former_),
      m_qubits_latter (qubits_latter_)
{
}

EntSwapAdaptApp::~EntSwapAdaptApp ()
{
}

EntSwapAdaptApp::EntSwapAdaptApp ()
    : m_qphyent (nullptr), m_qubits_former (nullptr), m_qubits_latter (nullptr)
{
}

TypeId
EntSwapAdaptApp::GetTypeId ()
{
  static TypeId tid =
      TypeId ("ns3::EntSwapAdaptApp")
          .AddConstructor<EntSwapAdaptApp> ()
          .SetParent<Application> ()
          .AddAttribute ("QPhyEntity", "The pointer to the quantum physical entity", PointerValue (),
                         MakePointerAccessor (&EntSwapAdaptApp::m_qphyent),
                         MakePointerChecker<QuantumPhyEntity> ())
          .AddAttribute ("QubitsFormer", "The pointer to the former qubits of each owner",
                         PointerValue (),
                         MakePointerAccessor (&EntSwapAdaptApp::m_qubits_former),
                         MakePointerChecker<QuantumMemory> ())
          .AddAttribute ("QubitsLatter", "The pointer to the latter qubits of each owner",
                         PointerValue (),
                         MakePointerAccessor (&EntSwapAdaptApp::m_qubits_latter),
                         MakePointerChecker<QuantumMemory> ())
          ;
  return tid;
}

void
EntSwapAdaptApp::EntanglementSwapping ()
{
  std::string flag_x = AllocAncillaQubit ();
  std::string flag_z = AllocAncillaQubit ();
  Simulator::ScheduleNow (&QuantumPhyEntity::GenerateQubitsPure, m_qphyent,
                          "God", q_ket_0, std::vector<std::string>{flag_x});
  Simulator::ScheduleNow (&QuantumPhyEntity::GenerateQubitsPure, m_qphyent,
                          "God", q_ket_0, std::vector<std::string>{flag_z});

  std::string last_qubit = m_qubits_former->GetQubit (m_qubits_former->GetSize () - 1);
  std::vector<std::complex<double>> unused;
  for (unsigned rank = 1; rank < m_qubits_former->GetSize () - 1; ++rank)
    {
      std::pair<std::string, std::string> qubits = {
        m_qubits_former->GetQubit (rank), m_qubits_latter->GetQubit (rank)};
      NS_LOG_LOGIC ("Owner " << rank << " has qubits " << qubits.first << " " << qubits.second);

      // local operations
      Simulator::ScheduleNow (&QuantumPhyEntity::ApplyGate, m_qphyent,
                              "God", QNS_GATE_PREFIX + "CNOT",
                              std::vector<std::complex<double>>{},
                              std::vector<std::string>{qubits.second, qubits.first});
      Simulator::ScheduleNow (&QuantumPhyEntity::ApplyGate, m_qphyent,
                              "God", QNS_GATE_PREFIX + "H",
                              std::vector<std::complex<double>>{},
                              std::vector<std::string>{qubits.first});

      // God apply control operations
      Simulator::ScheduleNow (&QuantumPhyEntity::ApplyGate, m_qphyent,
                              "God", QNS_GATE_PREFIX + "CNOT",
                              std::vector<std::complex<double>>{},
                              std::vector<std::string>{flag_x, qubits.second});
      // the latter qubit not used anymore
      Simulator::ScheduleNow (&QuantumPhyEntity::PartialTrace, m_qphyent,
                                std::vector<std::string>{qubits.second}, unused);

      Simulator::ScheduleNow(&QuantumPhyEntity::ApplyGate, m_qphyent,
                              "God", QNS_GATE_PREFIX + "CNOT",
                              std::vector<std::complex<double>>{},
                              std::vector<std::string>{flag_z, qubits.first});
      // the former qubit not used anymore
      Simulator::ScheduleNow (&QuantumPhyEntity::PartialTrace, m_qphyent,
                                std::vector<std::string>{qubits.first}, unused);
    }

  // God apply control operations
  Simulator::ScheduleNow (&QuantumPhyEntity::ApplyControledOperation, m_qphyent,
                          GetNode ()->GetObject<QuantumNode> ()->GetOwner (), QNS_GATE_PREFIX + "PX",
                          QNS_GATE_PREFIX + "CX", cnot,
                          std::vector<std::string>{flag_x},
                          std::vector<std::string>{last_qubit});
                          
  // flag x not used anymore
  Simulator::ScheduleNow (&QuantumPhyEntity::PartialTrace, m_qphyent,
                          std::vector<std::string>{flag_x}, unused);    

  Simulator::ScheduleNow (&QuantumPhyEntity::ApplyControledOperation, m_qphyent,
                          GetNode ()->GetObject<QuantumNode> ()->GetOwner (), QNS_GATE_PREFIX + "PZ",
                          QNS_GATE_PREFIX + "CZ", std::vector<std::complex<double>>{},
                          std::vector<std::string>{flag_z},
                          std::vector<std::string>{last_qubit});
  // flag z not used anymore
  Simulator::ScheduleNow (&QuantumPhyEntity::PartialTrace, m_qphyent,
                          std::vector<std::string>{flag_z}, unused);
  Simulator::ScheduleNow (&QuantumPhyEntity::Contract, m_qphyent);
}

void
EntSwapAdaptApp::StartApplication ()
{
  Simulator::ScheduleNow (&EntSwapAdaptApp::EntanglementSwapping, this);
}

} // namespace ns3