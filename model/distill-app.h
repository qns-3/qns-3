#ifndef DISTILL_APP_H
#define DISTILL_APP_H

#include "ns3/socket.h"
#include "ns3/application.h"

namespace ns3 {

class QuantumPhyEntity;
class QuantumNode;
class QuantumChannel;

/**
 * \brief Basic distillation application.
 * 
 * This application is used to schedule a distillation between two nodes, say Alice and Bob.
 * The protocol distillates one pair of EPRs from two pairs of EPRs.
 * 
 * 1. Alice generates and distributes two EPR pairs to Bob with the help of DistributeEPRApps.
 * 2. Alice applies local operations on her qubits and sends the result to Bob.
 * 3. Bob applies local operations on his qubits and sends the result to Alice.
 * 
 * \note The source app must be created before the destination app.
 * 
*/
class DistillApp : public Application
{
public:
  DistillApp (Ptr<QuantumPhyEntity> qphyent_, bool checker_, Ptr<QuantumChannel> conn_,
              const std::pair<std::string, std::string> &qubits_);
  virtual ~DistillApp ();

  DistillApp ();
  static TypeId GetTypeId ();

  /**
   * \brief Schedule the epr sharing and the local operation events that start a distillation.
  */
  void Distillate ();

  /**
   * \brief Set the address and port of the destination/source node.
  */
  void SetRemote (Address ip, uint16_t port);
  /**
   * \brief Fill the packet to send with a specific string.
   * \param fill The string to fill the packet with.
  */
  void SetFill (std::string fill);
  void SendPacket (Ptr<Packet> packet, Ipv6Address destination, uint16_t port);
  /** 
   * \brief Send the filled packet to the destination/source node.
  */
  void Send ();

  /**
   * \brief Parse a classical packet from destination/source and
   * schedule the local operations that close the distillation.
  */
  void HandleRead (Ptr<Socket> socket);

  void SetEPRGoal (const std::pair<std::string, std::string> &epr_goal_);
  void SetEPRMeas (const std::pair<std::string, std::string> &epr_meas_);
  void SetQubits (const std::pair<std::string, std::string> &qubits_);
  bool GetWin () const;

private:
  void SetupReceiveSocket (Ptr<Socket> socket, uint16_t port);
  virtual void StartApplication ();

  Ptr<Socket> m_recv_socket; /**< A socket to receive on a specific port */
  uint16_t m_port;

  Ptr<Socket> m_send_socket; /**< A socket to listen on a specific port */

  Address m_peerAddress; //!< Remote peer address
  uint16_t m_peerPort; //!< Remote peer port

  uint32_t m_size; //!< Size of the sent packet
  uint32_t m_dataSize; //!< packet payload size (must be equal to m_size)
  uint8_t *m_data; //!< packet payload data

  
  Ptr<QuantumPhyEntity> m_qphyent; //!< Quantum physical entity encapsulating the quantum circuit.

  bool m_checker; //!< True if the app is on a checker (bob), false if not (alice).

  Ptr<QuantumNode> m_pnode; //!< Pointer to the node this application is installed on.

  Ptr<QuantumChannel> m_conn; //!< Pointer to the quantum channel of this distillation.

  /**
   * \brief The goal EPR pair of this distillation.
   * \note The former qubit is Alice's and the latter Bob's.
   * \note Only used by Alice.
  */
  std::pair<std::string, std::string> m_epr_goal;

  /**
   * \brief The measured EPR pair of this distillation.
   * \note The former qubit is Alice's and the latter Bob's.
   * \note Only used by Alice.
  */
  std::pair<std::string, std::string> m_epr_meas;

  /**
   * \brief Bob's qubits.
   * \note Only used by Bob.
  */
  std::pair<std::string, std::string> m_qubits;

  /**
   * \brief True if the distillation is successful, false if not.
  */
  bool m_win;
};

} // namespace ns3

#endif /* DISTILL_APP_H */
