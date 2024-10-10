#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/csma-module.h"
#include "ns3/applications-module.h"

using namespace ns3;
NS_LOG_COMPONENT_DEFINE("NetworkSimulation");

int main(int argc, char *argv[])
{
  // Set up the logging component
  Time::SetResolution(Time::NS);
  LogComponentEnable("UdpEchoClientApplication", LOG_LEVEL_INFO);
  LogComponentEnable("UdpEchoServerApplication", LOG_LEVEL_INFO);
  LogComponentEnable("NetworkSimulation", LOG_LEVEL_INFO);

  // Create network nodes
  NodeContainer subnetA, subnetB, routers;
  subnetA.Create(2);  // 2 Nodes in Subnet A
  subnetB.Create(2);  // 2 Nodes in Subnet B
  routers.Create(2);  // 2 Routers

  // Create a point-to-point connection between routers
  // *the increase or decrease of datarate does not have any effect on the 
  // the time it takes for the server to recieve the packets. this makes no 
  // significant difference in the connections. that 
  PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute("DataRate", StringValue("5Mbps"));
  pointToPoint.SetChannelAttribute("Delay", StringValue("2ms"));

  // Create a NodeContainers hold the multiple Ptr<Node> which are used to refer to the nodes.
  NetDeviceContainer routerDevices;
  routerDevices = pointToPoint.Install(routers.Get(0), routers.Get(1));

  // Create CSMA connections for the subnets
  // *there is a bit more delay introduced when the server is recieving
  // the bytes and this is due to the datarate reduction in the chanel attrib. this makes a 
  // significant difference in the connections
  CsmaHelper csma;
  csma.SetChannelAttribute("DataRate", StringValue("100Mbps"));
  csma.SetChannelAttribute("Delay", TimeValue(NanoSeconds(6560)));

  NetDeviceContainer subnetADevices, subnetBDevices;
  subnetADevices = csma.Install(subnetA);
  subnetBDevices = csma.Install(subnetB);

  // Assign IP addresses to the subnets
  InternetStackHelper internet;
  internet.Install(routers);
  internet.Install(subnetA);
  internet.Install(subnetB);

  // *allocates IP addresses based on a given network number and 
  //mask combination along with an initial IP address.
  // *do for ipv4 and ipv6 and talk about the difference
  Ipv4AddressHelper address;
  address.SetBase("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer subnetAInterfaces = address.Assign(subnetADevices);

  address.SetBase("10.1.2.0", "255.255.255.0");
  Ipv4InterfaceContainer subnetBInterfaces = address.Assign(subnetBDevices);

  address.SetBase("10.1.3.0", "255.255.255.0");
  Ipv4InterfaceContainer routerInterfaces = address.Assign(routerDevices);

  // Set up routing
  Ipv4GlobalRoutingHelper::PopulateRoutingTables();

  // Create an application (e.g., a UDP echo server) on Subnet A on port 9
  UdpEchoServerHelper echoServer(9);
  // install the server on the node 2 then start and stop the server
  ApplicationContainer serverApps = echoServer.Install(subnetA.Get(1));
  serverApps.Start(Seconds(1.0));
  serverApps.Stop(Seconds(5.0));

  // Create a UDP client application on Subnet A interface
  UdpEchoClientHelper echoClient(subnetAInterfaces.GetAddress(1), 9);
  echoClient.SetAttribute("MaxPackets", UintegerValue(100));
  echoClient.SetAttribute("Interval", TimeValue(Seconds(1.0))); //1 packet per second
  echoClient.SetAttribute("PacketSize", UintegerValue(1024));

  ApplicationContainer clientApps = echoClient.Install(subnetA.Get(0));
  clientApps.Start(Seconds(2.0));
  clientApps.Stop(Seconds(5.0));

  // Enable tracing for visualization and analysis
  AsciiTraceHelper ascii;
  pointToPoint.EnableAsciiAll(ascii.CreateFileStream("network-simulation.tr"));

  // Enable PCAP tracing on a point-to-point link
  csma.EnablePcapAll("network-simulation");
  Simulator::Run();
  Simulator::Destroy();
  return 0;
}
