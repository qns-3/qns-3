// Note that Alice's app must be created before Bob's app

#ifndef TELEP_APP_H
#define TELEP_APP_H

#include "ns3/socket.h"
#include "ns3/application.h"

#include <complex>

namespace ns3 {

class QuantumPhyEntity;
class QuantumNode;
class Qubit;
class QuantumChannel;

/**
 * \brief Basic teleportation application.
 * 
 * This application is used to schedule a teleportation between two nodes, say Alice and Bob.
 * The protocol teleports a single qubit state, say psi, at the cost of a EPR pair.
 * 
 * 1. Alice generates and distributes a EPR pair to Bob with the help of DistributeEPRApps.
 * 2. Alice applies local operations on her qubits and sends the result to Bob.
 * 3. Bob applies local operations on his qubits according to the received classical message.
*/

class TelepSrcApp : public Application // Alice
{
public:
  TelepSrcApp (Ptr<QuantumPhyEntity> qphyent_, Ptr<QuantumChannel> conn_,
               const std::pair<std::string, std::string> &qubits_);
  virtual ~TelepSrcApp ();

  TelepSrcApp ();
  static TypeId GetTypeId ();

  /**
   * \brief Schedule the epr sharing and the local operation events that start a teleportation.
   * 
   * \note Alice will generate the input qubit if and only if m_input is not null.
  */
  void Teleport ();

  void SetRemote (Address ip, uint16_t port);
  void SetFill (uint8_t *fill, uint32_t fillSize, uint32_t dataSize);
  void SendPacket (Ptr<Packet> packet, Ipv6Address destination, uint16_t port);
  /** 
   * \brief Measure the two qubits and send the outcomes to the destination node.
  */
  void MeasureAndSend ();

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

  Ptr<Socket> m_send_socket; /**< A socket to listen on a specific port */

  Address m_peerAddress; //!< Remote peer address
  uint16_t m_peerPort; //!< Remote peer port

  uint32_t m_size; //!< Size of the sent packet
  uint32_t m_dataSize; //!< packet payload size (must be equal to m_size)
  uint8_t *m_data; //!< packet payload data

  Ptr<QuantumPhyEntity> m_qphyent; //!< The quantum physical entity
  Ptr<QuantumChannel> m_conn; //!< The quantum connection
  std::pair<std::string, std::string> m_qubits; //!< The qubits of Alice
  std::string m_qubit; //!< The qubit of Bob
  Ptr<Qubit> m_input; //!< The state vector of the input qubit
};

class TelepDstApp : public Application // Bob
{
public:
  TelepDstApp (Ptr<QuantumPhyEntity> qphyent_, const std::pair<std::string, std::string> &conn_,
               const std::string &qubit_);
  virtual ~TelepDstApp ();

  TelepDstApp ();
  static TypeId GetTypeId ();

  void HandleRead (Ptr<Socket> socket);

  void SetQubit (const std::string &qubit_);
  std::vector<std::complex<double>> GetOutput () const;

private:
  void SetupReceiveSocket (Ptr<Socket> socket, uint16_t port);
  virtual void StartApplication ();

  Ptr<Socket> m_recv_socket; /**< A socket to receive on a specific port. */
  uint16_t m_port; /**< The port to receive on. */

  Ptr<QuantumPhyEntity> m_qphyent; /**< The quantum physical entity. */
  Ptr<QuantumNode> m_pnode; /**< The pointer to the quantum node. */
  std::string m_qubit; /**< The qubit of Bob. */

  std::vector<std::complex<double>> m_output; /**< The density matrix of Bob's qubit. */
};

} // namespace ns3

#endif /* TELEP_APP_H */
