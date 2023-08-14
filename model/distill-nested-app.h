// Note that Alice's app must be created before Bob's app

#ifndef DISTILL_NESTED_APP_H
#define DISTILL_NESTED_APP_H

#include "ns3/socket.h"
#include "ns3/application.h"

namespace ns3 {

class QuantumPhyEntity;
class QuantumNode;
class QuantumChannel;
class QuantumMemory;

/**
 * \brief Nested distillation application.
 * 
 * This application is used to schedule a nested distillation between two nodes, say Alice and Bob.
 * The protocol distillates one pair of EPRs from n = 2^m pairs of EPRs.
 */
class DistillNestedApp : public Application
{

public:
  DistillNestedApp (Ptr<QuantumPhyEntity> qphyent_, bool checker_, Ptr<QuantumChannel> conn_,
                    const std::pair<std::string, std::string> &qubits_);
  virtual ~DistillNestedApp ();

  DistillNestedApp ();
  static TypeId GetTypeId ();

  // schedule all events to get one EPR pair (src_qubits[0], dst_qubits[0])
  void Distillate (const std::vector<std::string> &src_qubits,
                   const std::vector<std::string> &dst_qubits);
  // distillate the two EPR pairs (front) and (middle) to get one (front)                   
  void DistillateOnce (const std::vector<std::string> &src_qubits,
                       const std::vector<std::string> &dst_qubits);

  void SetRemote (Address ip, uint16_t port);
  void SetFill (std::string fill);
  void SendPacket (Ptr<Packet> packet, Ipv6Address destination, uint16_t port);
  void Send ();

  void HandleRead (Ptr<Socket> socket);

  void SetSrcQubits (Ptr<QuantumMemory> src_qubits);
  void SetDstQubits (Ptr<QuantumMemory> dst_qubits);
  bool GetWin () const;

  Time GetOccupied () const;
  void Occupy (Time time);

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

  Ptr<QuantumPhyEntity> m_qphyent;
  bool m_checker;
  Ptr<QuantumNode> m_pnode;
  Ptr<QuantumChannel> m_conn;
  Ptr<QuantumMemory> m_src_qubits;
  Ptr<QuantumMemory> m_dst_qubits;
  bool m_win;
  Time m_occupied;
};

} // namespace ns3

#endif /* DISTILL_NESTED_APP_H */
