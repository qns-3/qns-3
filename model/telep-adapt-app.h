// Note that Alice's app must be created before Bob's app

#ifndef TELEP_ADAPT_APP_H
#define TELEP_ADAPT_APP_H

#include "ns3/socket.h"
#include "ns3/application.h"

namespace ns3 {

class QuantumPhyEntity;
class Qubit;
class QuantumChannel;

/**
 * \brief Teleportation application with adaption.
 * 
 * This application is used to schedule a teleportation between two nodes, say Alice and Bob.
 * The protocol teleports a single qubit state, say psi, at the cost of a EPR pair.
 * 
 * 1. Alice generates and distributes a EPR pair to Bob with the help of DistributeEPRApps.
 * 2. Alice applies local operations on her qubits.
 * 2. God apply control operations on Alice's qubits and the Bob's qubit (goal).
*/
class TelepAdaptApp : public Application
{
public:
  TelepAdaptApp (Ptr<QuantumPhyEntity> qphyent_, Ptr<QuantumChannel> conn_,
                 const std::string& last_owner_,
                 const std::pair<std::string, std::string> &qubits_);
  virtual ~TelepAdaptApp ();

  TelepAdaptApp ();
  static TypeId GetTypeId ();

  /**
   * \brief Schedule all the events in a teleportation.
   * 
   * \note Alice generates the input qubit if and only if m_input is not null.
  */
  void Teleport ();

  void SetQubits (const std::pair<std::string, std::string> &qubits_);
  void SetQubit (const std::string &qubit_);

  /**
   * \brief Set the pointer to (the state vector of) the input qubit.
   * \param input_ The pointer to the input qubit.
   * 
   * \note Alice will generate the input qubit in Telep () if and only if m_input is not null.
  */
  void SetInput (Ptr<Qubit> input_);

private:
  virtual void StartApplication ();

  Ptr<QuantumPhyEntity> m_qphyent; /**< The quantum physical entity. */
  Ptr<QuantumChannel> m_conn; /**< The quantum connection. */
  std::string m_last_owner; /**< The last owner of the teleportation chain. */
  std::pair<std::string, std::string> m_qubits; /**< The two qubits of Alice. */
  std::string m_qubit; /**< The qubit of Bob. */
  Ptr<Qubit> m_input; /**< The pointer to the input qubit. */
};

} // namespace ns3

#endif /* TELEP_ADAPT_APP_H */
