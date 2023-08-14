// Note that Alice's app must be created before Bob's app

#ifndef ENT_SWAP_APP_H
#define ENT_SWAP_APP_H

#include "ns3/socket.h"
#include "ns3/application.h"

#include <complex>

namespace ns3 {

class QuantumPhyEntity;
class QuantumNode;
class Qubit;
class QuantumChannel;

/**
 * \brief Basic entanglement swapping application.
 * 
 * This application is used to schedule a chained entanglement swapping.
 * A longer EPR pair is gained at the cost of several shorter (more local) EPR pairs.
 * 
 * 1. Every node generates and distributes a EPR pair to its neighbor with the help of DistributeEPRApps.
 * 2. Every node applies bell measurement and sends the result to the last node.
 * 3. The last node applies correction gates according to the received classical message.
*/


class EntSwapSrcApp : public Application
{
public:
  EntSwapSrcApp (Ptr<QuantumPhyEntity> qphyent_, Ptr<QuantumChannel> conn_,
               const std::pair<std::string, std::string> &qubits_);
  virtual ~EntSwapSrcApp ();

  EntSwapSrcApp ();
  static TypeId GetTypeId ();

  void SetRemote (Address ip, uint16_t port);
  void SetFill (uint8_t *fill, uint32_t fillSize, uint32_t dataSize);
  void SendPacket (Ptr<Packet> packet, Ipv6Address destination, uint16_t port);
  /** 
   * \brief Measure the two qubits and send the outcomes to the last node.
  */
  void MeasureAndSend ();

  void SetQubits (const std::pair<std::string, std::string> &qubits_);
  
private:
  virtual void StartApplication ();

  Ptr<Socket> m_send_socket; /**< A socket to listen on a specific port */

  Address m_peerAddress; //!< Remote peer address
  uint16_t m_peerPort; //!< Remote peer port

  uint32_t m_size; //!< Size of the sent packet
  uint32_t m_dataSize; //!< packet payload size (must be equal to m_size)
  uint8_t *m_data; //!< packet payload data

  Ptr<QuantumPhyEntity> m_qphyent; //!< The quantum physical entity
  Ptr<QuantumChannel> m_conn; //!< The quantum connection with the next node
  std::pair<std::string, std::string> m_qubits; //!< The two qubits of this owner
};

class EntSwapDstApp : public Application
{
public:
  EntSwapDstApp (Ptr<QuantumPhyEntity> qphyent_, Ptr<QuantumNode> pnode_,
               const std::string &qubit_);
  virtual ~EntSwapDstApp ();

  EntSwapDstApp ();
  static TypeId GetTypeId ();

  void HandleRead (Ptr<Socket> socket);

  void SetQubit (const std::string &qubit_);

private:
  void SetupReceiveSocket (Ptr<Socket> socket, uint16_t port);
  virtual void StartApplication ();

  Ptr<Socket> m_recv_socket; /**< A socket to receive on a specific port. */
  uint16_t m_port; /**< The port to receive on. */

  Ptr<QuantumPhyEntity> m_qphyent; /**< The quantum physical entity. */
  Ptr<QuantumNode> m_pnode; /**< The pointer to the quantum node. */
  std::string m_qubit; /**< The qubit of the last owner. */
  unsigned m_count; /**< The number of packets to receive from middle nodes. */
  bool m_flag_x; /**< The flag of X correction. */
  bool m_flag_z; /**< The flag of Z correction. */
};

} // namespace ns3

#endif /* ENT_SWAP_APP_H */
