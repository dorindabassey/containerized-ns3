#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/ipv4-global-routing-helper.h"

using namespace ns3;

int main(int argc, char *argv[])
{
    // Set up logging
    LogComponentEnable("UdpEchoClientApplication", LOG_LEVEL_INFO);
    LogComponentEnable("UdpEchoServerApplication", LOG_LEVEL_INFO);

    // Create nodes for each subnet and routers
    NodeContainer subnetA, router1, subnetB, router2, subnetC;
    subnetA.Create(1);  // Node in Subnet A
    router1.Create(1);  // Router 1
    subnetB.Create(1);  // Node in Subnet B
    router2.Create(1);  // Router 2
    subnetC.Create(1);  // Node in Subnet C

    // Install the Internet stack on all nodes
    InternetStackHelper stack;
    stack.Install(subnetA);
    stack.Install(router1);
    stack.Install(subnetB);
    stack.Install(router2);
    stack.Install(subnetC);

    // Network Setup:
    // Each subnet is connected to the appropriate router using point-to-point links.
    // Set up point-to-point links between subnets and routers
    PointToPointHelper p2p;
    p2p.SetDeviceAttribute("DataRate", StringValue("5Mbps"));
    p2p.SetChannelAttribute("Delay", StringValue("2ms"));

    // Create devices for each point-to-point link
    NetDeviceContainer devicesAtoR1 = p2p.Install(subnetA.Get(0), router1.Get(0));
    NetDeviceContainer devicesR1toB = p2p.Install(router1.Get(0), subnetB.Get(0));
    NetDeviceContainer devicesBtoR2 = p2p.Install(subnetB.Get(0), router2.Get(0));
    NetDeviceContainer devicesR2toC = p2p.Install(router2.Get(0), subnetC.Get(0));

    // Assign IP addresses to each subnet
    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer ifaceAtoR1 = address.Assign(devicesAtoR1);

    address.SetBase("10.1.2.0", "255.255.255.0");
    Ipv4InterfaceContainer ifaceR1toB = address.Assign(devicesR1toB);

    address.SetBase("10.1.3.0", "255.255.255.0");
    Ipv4InterfaceContainer ifaceBtoR2 = address.Assign(devicesBtoR2);

    address.SetBase("10.1.4.0", "255.255.255.0");
    Ipv4InterfaceContainer ifaceR2toC = address.Assign(devicesR2toC);

    // Set up applications to generate traffic
    // Install a UDP echo server on the node in Subnet B
    // A UDP Echo Server is installed on the node in Subnet C (node at the destination).
    UdpEchoServerHelper echoServer(9);
    //ApplicationContainer serverApp = echoServer.Install(subnetB.Get(0));
    ApplicationContainer serverApp = echoServer.Install(subnetC.Get(0));
    serverApp.Start(Seconds(1.0));
    serverApp.Stop(Seconds(10.0));

    // Install a UDP echo client on the node in Subnet A
    // UdpEchoClientHelper echoClient(ifaceR1toB.GetAddress(1), 9); // Send to the server IP in Subnet B
    UdpEchoClientHelper echoClient(ifaceR2toC.GetAddress(1), 9); // Send to the server IP in Subnet C
    echoClient.SetAttribute("MaxPackets", UintegerValue(1));
    echoClient.SetAttribute("Interval", TimeValue(Seconds(1.0)));
    echoClient.SetAttribute("PacketSize", UintegerValue(1024));

    // A UDP Echo Client is installed on the node in Subnet A (node at the source).
    ApplicationContainer clientApp = echoClient.Install(subnetA.Get(0));
    clientApp.Start(Seconds(2.0));
    clientApp.Stop(Seconds(10.0));

    // Enable tracing to monitor the packet flow
    p2p.EnablePcapAll("network-topology");
    p2p.EnablePcap("network-topology-of-deviceBtoR2", devicesBtoR2);
    p2p.EnablePcap("network-topology-of-deviceAtoR1", devicesAtoR1);
    // The client sends packets to the server, creating network traffic 
    //  that traverses through Router 1 and Router 2.
    // captured packets only at router1's interface
    p2p.EnablePcap("network-topology-of-router1", devicesR1toB.Get(0));
    // captured packets only at subnetB's interface
    p2p.EnablePcap("network-topology-of-subnetB", devicesR1toB.Get(1));
    // captured packets only at router2's interface
    p2p.EnablePcap("network-topology-of-router2", devicesR2toC.Get(0));
    // captured packets only at subnetC's interface
    p2p.EnablePcap("network-topology-of-subnetC", devicesR2toC.Get(1));

    // Routing:
    //Global routing is enabled to ensure the packets can be routed between the subnets through the routers.
    // Populate routing tables
    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    // Run the simulation
    Simulator::Run();
    Simulator::Destroy();

    return 0;
}
