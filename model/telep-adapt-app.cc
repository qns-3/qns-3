#include "ns3/telep-adapt-app.h"

#include "ns3/quantum-basis.h"
#include "ns3/quantum-phy-entity.h" // class QuantumPhyEntity
#include "ns3/quantum-node.h" // class QuantumNode
#include "ns3/quantum-channel.h" // class QuantumChannel
#include "ns3/distribute-epr-protocol.h"

namespace ns3 {
NS_LOG_COMPONENT_DEFINE ("TelepAdaptApp");

NS_OBJECT_ENSURE_REGISTERED (TelepAdaptApp);

TelepAdaptApp::TelepAdaptApp (Ptr<QuantumPhyEntity> qphyent_, Ptr<QuantumChannel> conn_,
                              const std::string& last_owner_,
                              const std::pair<std::string, std::string> &qubits_)
    : m_qphyent (qphyent_), m_conn (conn_), m_last_owner (last_owner_), m_qubits (qubits_)
{
}

TelepAdaptApp::~TelepAdaptApp ()
{
}

TelepAdaptApp::TelepAdaptApp (
) : m_qphyent (nullptr), m_conn (nullptr),
    m_last_owner (""),
    m_qubits ({"", ""})
{
}

TypeId
TelepAdaptApp::GetTypeId ()
{
  static TypeId tid =
      TypeId ("ns3::TelepAdaptApp")
          .AddConstructor<TelepAdaptApp> ()
          .SetParent<Application> ()
          .AddAttribute ("QPhyEntity", "The pointer to the quantum physical entity", PointerValue (),
                         MakePointerAccessor (&TelepAdaptApp::m_qphyent),
                         MakePointerChecker<QuantumPhyEntity> ())
          .AddAttribute ("QChannel", "The connection between Alice and Bob", PointerValue (),
                         MakePointerAccessor (&TelepAdaptApp::m_conn),
                         MakePointerChecker<QuantumChannel> ())
          .AddAttribute ("LastOwner", "The last owner of the qubit", StringValue (),
                          MakeStringAccessor (&TelepAdaptApp::m_last_owner),
                          MakeStringChecker ())
          .AddAttribute ("Qubits", "The two qubits of Alice",
                         PairValue<StringValue, StringValue> (),
                         MakePairAccessor<StringValue, StringValue> (&TelepAdaptApp::m_qubits),
                         MakePairChecker<StringValue, StringValue> ())
          .AddAttribute ("Qubit", "The qubit of Bob", StringValue (),
                         MakeStringAccessor (&TelepAdaptApp::m_qubit), MakeStringChecker ())
          .AddAttribute ("Input", "The pointer to the state Alice wants to teleport to Bob",
                         PointerValue (), MakePointerAccessor (&TelepAdaptApp::m_input),
                         MakePointerChecker<Qubit> ());
  return tid;
}

void
TelepAdaptApp::Teleport ()
{
  NS_LOG_LOGIC ("Teleport at time " << Simulator::Now ());

  // generate psi
  if (m_input)
    {
      Simulator::ScheduleNow (&QuantumPhyEntity::GenerateQubitsPure, m_qphyent,
                              m_conn->GetSrcOwner (), m_input->GetStateVector (),
                              std::vector<std::string>{m_qubits.first});
    }
  std::vector<std::complex<double>> input;
  // Simulator::ScheduleNow (&QuantumPhyEntity::PeekDM, m_qphyent,
  //                         m_conn->GetSrcOwner (), std::vector<std::string>{m_qubits.first}, input);

  // generate and distribute EPR
  Ptr<DistributeEPRSrcProtocol> dist_epr_src_app =
      m_qphyent->GetConn2Apps (m_conn, APP_DIST_EPR).first->GetObject<DistributeEPRSrcProtocol> ();
  Simulator::ScheduleNow (&DistributeEPRSrcProtocol::GenerateAndDistributeEPR,
                          dist_epr_src_app,
                          std::pair<std::string, std::string>{m_qubits.second, m_qubit});

  // Alice applies local operations
  Simulator::Schedule (Seconds (0.1),
                       &QuantumPhyEntity::ApplyGate, m_qphyent, m_conn->GetSrcOwner (),
                       QNS_GATE_PREFIX + "CNOT", std::vector<std::complex<double>>{},
                       std::vector<std::string>{m_qubits.second, m_qubits.first});
  Simulator::Schedule (Seconds (0.1),
                       &QuantumPhyEntity::ApplyGate, m_qphyent, m_conn->GetSrcOwner (),
                       QNS_GATE_PREFIX + "H", std::vector<std::complex<double>>{},
                       std::vector<std::string>{m_qubits.first});


  // God apply control operations
  Simulator::Schedule (
      Seconds (0.1), &QuantumPhyEntity::ApplyControledOperation,
      m_qphyent, m_conn->GetDstOwner (), QNS_GATE_PREFIX + "X", QNS_GATE_PREFIX + "CX", cnot,
      std::vector<std::string>{m_qubits.second}, std::vector<std::string>{m_qubit});

  Simulator::Schedule (Seconds (0.1),
                       &QuantumPhyEntity::ApplyControledOperation, m_qphyent,
                       m_conn->GetDstOwner (), QNS_GATE_PREFIX + "Z", QNS_GATE_PREFIX + "CZ",
                       std::vector<std::complex<double>>{},
                       std::vector<std::string>{m_qubits.first}, std::vector<std::string>{m_qubit});

  // Simulator::Schedule (Seconds (0.1), &QuantumPhyEntity::Contract, m_qphyent);


  if (m_last_owner == m_conn->GetDstOwner ())
    {
    std::vector<std::complex<double>> unused;
    Simulator::Schedule (Seconds (0.1), &QuantumPhyEntity::PeekDM,
                         m_qphyent, m_last_owner, std::vector<std::string>{m_qubit}, unused);
    }
}

void
TelepAdaptApp::SetQubits (const std::pair<std::string, std::string> &qubits_)
{
  NS_LOG_LOGIC ("Setting qubits " << qubits_.first << " " << qubits_.second);
  m_qubits = qubits_;
}

void
TelepAdaptApp::SetQubit (const std::string &qubit_)
{
  m_qubit = qubit_;
}

void
TelepAdaptApp::SetInput (Ptr<Qubit> input_)
{
  m_input = input_;
}

void
TelepAdaptApp::StartApplication ()
{
  Teleport ();
}

} // namespace ns3