#ifndef DISTILL_NESTED_ADAPT_APP_H
#define DISTILL_NESTED_ADAPT_APP_H

#include "ns3/socket.h"
#include "ns3/application.h"

namespace ns3 {

class QuantumPhyEntity;
class QuantumNode;
class QuantumChannel;
class QuantumMemory;

/**
 * \brief Nested distillation application with adaptation.
 * 
 * This application is used to recursively schedule a distillation with adaptation
 * between two nodes.
 * The protocol distillates one pair of EPRs from n = 2^m pairs of EPRs.
 */
class DistillNestedAdaptApp : public Application
{
public:
  DistillNestedAdaptApp (Ptr<QuantumPhyEntity> qphyent_, bool checker_, Ptr<QuantumChannel> conn_,
                         const std::pair<std::string, std::string> &qubits_);
  virtual ~DistillNestedAdaptApp ();

  DistillNestedAdaptApp ();
  static TypeId GetTypeId ();

  /**
   * \brief Schedule all events to get one EPR pair by nested distillation.
   * \param src_qubits All the source qubits in the nested distillation.
   * \param dst_qubits All the destination qubits in the nested distillation.
   * 
   * \note The goal EPR pair is src_qubits[0] and dst_qubits[0].
  */
  void Distillate (const std::vector<std::string> &src_qubits,
                   const std::vector<std::string> &dst_qubits);

  // distillate the two EPR pairs (front) and (middle) to get one (front)
  /**
   * \brief Schedule events to get one EPR pair by a single distillation.
   * \param src_qubits The source qubits.
   * \param dst_qubits The destination qubits.
   * 
   * \note The size of src_qubits and dst_qubits shoule be the same.
   * \note The goal EPR pair is src_qubits[0] and dst_qubits[0].
   * \note The measured EPR pair is src_qubits[size / 2] and dst_qubits[size / 2].
  */
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
  uint16_t m_port; /**< Port to receive on */

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
   * \brief All the source qubits in the nested distillation.
   * \note The goal source qubit in the goal EPR pair is src_qubits[0].
  */
  Ptr<QuantumMemory> m_src_qubits;

  /**
   * \brief All the destination qubits in the nested distillation.
   * \note The goal destination qubit in the goal EPR pair is dst_qubits[0].
  */
  Ptr<QuantumMemory> m_dst_qubits;

  /**
   * \brief The flag qubit indicating whether the distillation succeeds.
   * 
   * To find out whether the distillation succeeds,
   * just measure the flag qubit in the computational basis.
  */
  std::string m_flag_qubit;

  /**
   * \brief The current time occupied by the event scheduler.
  */
  Time m_occupied;

};

} // namespace ns3

#endif /* DISTILL_NESTED_ADAPT_APP_H */
