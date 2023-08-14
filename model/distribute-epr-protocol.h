// Note that Alice's app must be created before Bob's app

#ifndef DISTRIBUTE_EPR_PROTOCOL
#define DISTRIBUTE_EPR_PROTOCOL

#include "ns3/socket.h"
#include "ns3/application.h"

#include <complex>

namespace ns3 {

class QuantumPhyEntity;
class QuantumChannel;

class DistributeEPRSrcProtocol : public Application // Alice
{
public:
  DistributeEPRSrcProtocol (Ptr<QuantumPhyEntity> qphyent_, Ptr<QuantumChannel> conn_,
                            const std::pair<std::string, std::string> &epr_);
  virtual ~DistributeEPRSrcProtocol ();

  DistributeEPRSrcProtocol ();
  static TypeId GetTypeId ();

  void GenerateAndDistributeEPR (const std::pair<std::string, std::string> &epr = {});

  void SetRemote (Address ip, uint16_t port);
  void SetFill (std::string fill);
  void SendPacket (Ptr<Packet> packet, Ipv6Address destination, uint16_t port);
  void Send (const std::pair<std::string, std::string> &epr = {});

  bool SetEPR (const std::pair<std::string, std::string> &m_epr);

private:
  virtual void StartApplication ();

  Ptr<Socket> m_send_socket; /**< A socket to listen on a specific port */

  Address m_peerAddress; //!< Remote peer address
  uint16_t m_peerPort; //!< Remote peer port

  uint32_t m_size; //!< Size of the sent packet
  uint32_t m_dataSize; //!< packet payload size (must be equal to m_size)
  uint8_t *m_data; //!< packet payload data

  Ptr<QuantumPhyEntity> m_qphyent;
  Ptr<QuantumChannel> m_conn;
  std::pair<std::string, std::string> m_epr;
};

class DistributeEPRDstProtocol : public Application // Bob
{
public:
  DistributeEPRDstProtocol (Ptr<QuantumPhyEntity> qphyent_, Ptr<QuantumChannel> conn_);
  virtual ~DistributeEPRDstProtocol ();

  DistributeEPRDstProtocol ();
  static TypeId GetTypeId ();

  void HandleRead (Ptr<Socket> socket);

  std::vector<std::complex<double>> GetOutput () const;

private:
  void SetupReceiveSocket (Ptr<Socket> socket, uint16_t port);
  virtual void StartApplication ();

  Ptr<Socket> m_recv_socket;
  uint16_t m_port;

  Ptr<QuantumPhyEntity> m_qphyent;
  Ptr<QuantumChannel> m_conn;
  std::vector<std::complex<double>> m_output;
};

} // namespace ns3

#endif /* DISTRIBUTE_EPR_PROTOCOL */
