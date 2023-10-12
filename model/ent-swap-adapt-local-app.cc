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
  
  unsigned owners = m_qubits_former->GetSize ();
  
  std::vector<std::complex<double>> epr_dm = GetEPRwithFidelity (0.95);

  // God generate the EPR between Owner0 and Owner1, Owner1 and Owner2
  Simulator::ScheduleNow (&QuantumPhyEntity::GenerateQubitsMixed, m_qphyent,
                          "God", epr_dm, 
                          std::vector<std::string>{m_qubits_latter->GetQubit (0), m_qubits_former->GetQubit (1)});
  Simulator::ScheduleNow (&QuantumPhyEntity::GenerateQubitsMixed, m_qphyent,
                          "God", epr_dm, 
                          std::vector<std::string>{m_qubits_latter->GetQubit (1), m_qubits_former->GetQubit (2)});  
  // Owner1 apply local operations
  Simulator::ScheduleNow (&QuantumPhyEntity::ApplyGate, m_qphyent,
                          "God", QNS_GATE_PREFIX + "CNOT",
                          std::vector<std::complex<double>>{},
                          std::vector<std::string>{m_qubits_latter->GetQubit (1), m_qubits_former->GetQubit (1)});
  Simulator::ScheduleNow (&QuantumPhyEntity::ApplyGate, m_qphyent,
                          "God", QNS_GATE_PREFIX + "H",
                          std::vector<std::complex<double>>{},
                          std::vector<std::string>{m_qubits_former->GetQubit (1)});


  for (unsigned rank = 2; rank < owners - 1; ++rank)
    {
      // God generate the EPR between owner rank and owner rank + 1
      Simulator::ScheduleNow (&QuantumPhyEntity::GenerateQubitsMixed, m_qphyent,
                              "God", epr_dm, 
                              std::vector<std::string>{m_qubits_latter->GetQubit (rank), m_qubits_former->GetQubit (rank + 1)});

      // owner rank apply local operations
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
      
      // God pass the xor result from owner rank - 1 to owner rank
      std::pair<std::string, std::string> qubits_pred = {
        m_qubits_former->GetQubit (rank - 1), m_qubits_latter->GetQubit (rank - 1)};
      Simulator::ScheduleNow (&QuantumPhyEntity::ApplyGate, m_qphyent,
                              "God", QNS_GATE_PREFIX + "CNOT",
                              std::vector<std::complex<double>>{},
                              std::vector<std::string>{qubits.second, qubits_pred.second});  
      
      // the latter qubit of the previous owner not used anymore
      Simulator::ScheduleNow (&QuantumPhyEntity::PartialTrace, m_qphyent,
                              std::vector<std::string>{qubits_pred.second});
      
      Simulator::ScheduleNow (&QuantumPhyEntity::ApplyGate, m_qphyent,
                              "God", QNS_GATE_PREFIX + "CNOT",
                              std::vector<std::complex<double>>{},
                              std::vector<std::string>{qubits.first, qubits_pred.first});  
      // the former qubit of the previous owner not used anymore
      Simulator::ScheduleNow (&QuantumPhyEntity::PartialTrace, m_qphyent,
                              std::vector<std::string>{qubits_pred.first});


    }

  // between the last two owners
  std::string last_qubit = m_qubits_former->GetQubit (owners - 1);

  // God apply control operations with error equiv to that of PX / PZ gates
  Simulator::ScheduleNow (&QuantumPhyEntity::ApplyControlledOperation, m_qphyent,
                          GetNode ()->GetObject<QuantumNode> ()->GetOwner (), QNS_GATE_PREFIX + "PX",
                          QNS_GATE_PREFIX + "CX", cnot,
                          std::vector<std::string>{m_qubits_latter->GetQubit (owners - 2)},
                          std::vector<std::string>{last_qubit});
  // the latter qubit of the last but one owner not used anymore
  Simulator::ScheduleNow (&QuantumPhyEntity::PartialTrace, m_qphyent,
                          std::vector<std::string>{m_qubits_latter->GetQubit (owners - 2)});    

  Simulator::ScheduleNow (&QuantumPhyEntity::ApplyControlledOperation, m_qphyent,
                          GetNode ()->GetObject<QuantumNode> ()->GetOwner (), QNS_GATE_PREFIX + "PZ", 
                          QNS_GATE_PREFIX + "CZ", std::vector<std::complex<double>>{},
                          std::vector<std::string>{m_qubits_former->GetQubit (owners - 2)},
                          std::vector<std::string>{last_qubit});
  // the former qubit of the last but one owner not used anymore
  Simulator::ScheduleNow (&QuantumPhyEntity::PartialTrace, m_qphyent,
                          std::vector<std::string>{m_qubits_former->GetQubit (owners - 2)});

  // contract the circuit to discard all the intermediate qubits, leaving only the final EPR state
  Simulator::ScheduleNow (&QuantumPhyEntity::Contract, m_qphyent, "ascend");
  // peek the density matrix of the final EPR state, which is shared by the first and the last owner
  std::vector<std::complex<double>> unused;
  Simulator::ScheduleNow (&QuantumPhyEntity::PeekDM, m_qphyent, "God",
                          std::vector<std::string>{m_qubits_latter->GetQubit (0), last_qubit}, unused);
  // calculate the fidelity of the final EPR state
  double fidel;
  Simulator::ScheduleNow (&QuantumPhyEntity::CalculateFidelity, m_qphyent,
                          std::pair<std::string, std::string>{m_qubits_latter->GetQubit (0), last_qubit}, fidel); 
            
}

void
EntSwapAdaptLocalApp::StartApplication ()
{
  Simulator::ScheduleNow (&EntSwapAdaptLocalApp::EntanglementSwapping, this);
}

} // namespace ns3