/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "ns3/core-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/netanim-module.h"

// Default Network Topology
//
// Number of wifi or csma nodes can be increased up to 250
//                          |
//                 Rank 0   |   Rank 1
// -------------------------|----------------------------
//   Wifi 7.0.0.0
//                 AP
//  *    *    *    *
//  |    |    |    |    10.1.1.0
// n5   n6   n7   n0 -------------- n1   n2   n3   n4
//                   point-to-point  |    |    |    |
//                                   ================
//                                     LAN 10.1.2.0


using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("Wi-Fi_ScriptExample");

int
main (int argc, char *argv[])
{
  uint32_t nCsma = 1;
  uint32_t nWifi = 1;

  uint32_t payloadSize = 1500;                       /* Transport layer payload size in bytes. */
  std::string dataRate = "64Mbps";                  /* Application layer datarate. */
  std::string phyRate = "HtMcs7";                    /* Physical layer bitrate. */

  CommandLine cmd;
  cmd.AddValue ("nCsma", "Number of \"extra\" CSMA nodes/devices", nCsma);
  cmd.AddValue ("nWifi", "Number of wifi STA devices", nWifi);
  cmd.Parse (argc,argv);

//  LogComponentEnable("OnOffApplication", LOG_LEVEL_ALL);
  Time::SetResolution (Time::NS);

  /* Configure TCP Options */
  Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue (payloadSize));
  /*Transmission mode is Tcp(NewReno)*/
  Config::SetDefault ("ns3::TcpL4Protocol::SocketType", StringValue ("ns3::TcpNewReno"));
  /* Set channel width */
  Config::Set ("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/ChannelWidth", UintegerValue (20));


  NodeContainer p2pNodes;
  p2pNodes.Create (2);

  PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("100Mbps"));
//  pointToPoint.SetDeviceAttribute ("Mtu", UintegerValue (1500));
  pointToPoint.SetChannelAttribute ("Delay", StringValue ("2ms"));

  NetDeviceContainer p2pDevices;
  p2pDevices = pointToPoint.Install (p2pNodes);

  NodeContainer csmaNodes;
  csmaNodes.Add (p2pNodes.Get (0));
  csmaNodes.Create (nCsma);

  CsmaHelper csma;
  csma.SetChannelAttribute ("DataRate", StringValue ("100Mbps"));
  csma.SetChannelAttribute ("Delay", TimeValue (NanoSeconds (6560)));

  NetDeviceContainer csmaDevices;
  csmaDevices = csma.Install (csmaNodes);

  NodeContainer wifiStaNodes;
  wifiStaNodes.Create (nWifi);
  NodeContainer wifiApNode = p2pNodes.Get (1);

  WifiHelper wifiHelper;
  wifiHelper.SetStandard(WIFI_PHY_STANDARD_80211n_2_4GHZ );

  /* Set up Legacy Channel */
  YansWifiChannelHelper wifiChannel;
  wifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
  wifiChannel.AddPropagationLoss ("ns3::LogDistancePropagationLossModel","ReferenceLoss", DoubleValue (48.0));

  /* Setup Physical Layer */
  YansWifiPhyHelper wifiPhy = YansWifiPhyHelper::Default ();
  wifiPhy.SetChannel (wifiChannel.Create ());
  wifiHelper.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
                                      "DataMode", StringValue (phyRate),
                                      "ControlMode", StringValue ("HtMcs0"));



  HtWifiMacHelper mac = HtWifiMacHelper :: Default ();
  Ssid ssid = Ssid ("ns-3-ssid");
  mac.SetType ("ns3::StaWifiMac",
               "Ssid", SsidValue (ssid),
               "ActiveProbing", BooleanValue (false));

  NetDeviceContainer staDevices;
  staDevices = wifiHelper.Install (wifiPhy, mac, wifiStaNodes);

  mac.SetType ("ns3::ApWifiMac",
               "Ssid", SsidValue (ssid));

  NetDeviceContainer apDevices;
  apDevices = wifiHelper.Install (wifiPhy, mac, wifiApNode);

  MobilityHelper mobility;

  // /* Mobility model */
  // Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
  // positionAlloc->Add (Vector (0.0, 0.0, 0.0));

  mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
                                 "MinX", DoubleValue (10.0),
                                 "MinY", DoubleValue (7.0),
                                 "DeltaX", DoubleValue (5.0),
                                 "DeltaY", DoubleValue (10.0),
                                 "GridWidth", UintegerValue (10),
                                 "LayoutType", StringValue ("RowFirst"));

//  mobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel",
//		  	  	  	  	  	 "Distance", DoubleValue(5.0),
//                             "Speed", StringValue("ns3::UniformRandomVariable[Min=2.0|Max=4.0]"),
//                             "Direction", StringValue("ns3::UniformRandomVariable[Min=0.0|Max=6.283184]"),
//                             "Bounds", RectangleValue (Rectangle (-25, 30, -25, 30)));
  mobility.SetMobilityModel ("ns3::SteadyStateRandomWaypointMobilityModel",
		  	  	  	  	  	 "MinSpeed", DoubleValue(5.0),
  	  	  	  	  	  	  	 "MaxSpeed", DoubleValue(10.0),
							 "MinX", DoubleValue(5.0),
							 "MaxX", DoubleValue(10.0),
							 "MinY", DoubleValue(5.0),
							 "MaxY", DoubleValue(10.0));
 mobility.Install (wifiStaNodes);

  /* Mobility model */
  Ptr<ListPositionAllocator> positionAllocAp = CreateObject<ListPositionAllocator> ();
  positionAllocAp->Add (Vector (0.0, 0.0, 0.0));
  mobility.SetPositionAllocator (positionAllocAp);
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (wifiApNode);
  // mobility.Install (wifiStaNodes);

  InternetStackHelper stack;
  stack.Install (csmaNodes);
  stack.Install (wifiApNode);
  stack.Install (wifiStaNodes);

  Ipv4AddressHelper address;

  address.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer p2pInterfaces;
  p2pInterfaces = address.Assign (p2pDevices);

  address.SetBase ("10.1.2.0", "255.255.255.0");
  Ipv4InterfaceContainer csmaInterfaces;
  csmaInterfaces = address.Assign (csmaDevices);

  address.SetBase ("7.0.0.0", "255.255.255.0");
  Ipv4InterfaceContainer apInterface;
  Ipv4InterfaceContainer staInterface;
  staInterface = address.Assign (staDevices);
  apInterface = address.Assign (apDevices);

  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  /* Install TCP Receiver on the access point */
  uint16_t dlPort = 1235;
  PacketSinkHelper sinkHelper ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), dlPort));
  ApplicationContainer serverApp;
  ApplicationContainer clientApp = sinkHelper.Install (wifiStaNodes);

  for (uint32_t i = 0;i < wifiStaNodes.GetN(); i++){

    /* Install TCP/UDP Transmitter on the station */
    OnOffHelper server ("ns3::TcpSocketFactory", (InetSocketAddress (staInterface.GetAddress (i), dlPort)));
    server.SetAttribute ("PacketSize", UintegerValue (payloadSize));
    server.SetAttribute ("DataRate", DataRateValue (DataRate (dataRate)));
    server.SetAttribute("MaxBytes", UintegerValue (2000000000));
    serverApp = server.Install (csmaNodes.Get (nCsma));

  }

  /* Start Applications */
  clientApp.Start (Seconds (1.0));
  serverApp.Start (Seconds (0.0));
//  serverApp.Stop(Seconds (20));

  Simulator::Stop (Seconds (215));

 AnimationInterface anim ("wifi-exp.xml");
 anim.SetMaxPktsPerTraceFile(9999999999999);
 anim.UpdateNodeDescription(wifiStaNodes.Get(0), "UE1");
// anim.UpdateNodeDescription(wifiStaNodes.Get(1), "UE2");
// anim.UpdateNodeDescription(wifiStaNodes.Get(2), "UE3");
 anim.UpdateNodeDescription(p2pNodes.Get(1), "AP");
 anim.SetConstantPosition(p2pNodes.Get(1), 2.0, 2.0);
 anim.UpdateNodeDescription(csmaNodes.Get(nCsma), "Server");
 anim.UpdateNodeDescription(p2pNodes.Get(0), "Server");

  FlowMonitorHelper flowmon;
  Ptr<FlowMonitor> monitor;
  monitor = flowmon.Install(wifiStaNodes);
  monitor = flowmon.Install(csmaNodes);

  NS_LOG_UNCOND("Running Wi-Fi");
  Simulator::Run ();

  monitor->CheckForLostPackets ();
  Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowmon.GetClassifier ());
  std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats ();

  for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin (); i != stats.end (); ++i) {
      Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (i->first);
    std::cout << "Flow " << i->first << " (" << t.sourceAddress << " -> " << t.destinationAddress << ")\n";
    std::cout << "  Tx Packets: " << i->second.txPackets << "\n";
    std::cout << "  Rx Packets: " << i->second.rxPackets << "\n";
    std::cout << "  Tx Bytes:   " << i->second.txBytes << "\n";
    std::cout << "  Rx Bytes:   " << i->second.rxBytes << "\n";
    std::cout << "  Delay:   " << (i->second.delaySum.GetSeconds() / i->second.rxPackets)   << "\n";
    std::cout << "  Delay Sum:   " << i->second.delaySum.GetSeconds()   << "\n";
    std::cout << "  Lost Packets:   " << ((i->second.lostPackets )) << "\n";
    std::cout << "  Packet Dropped:   " << i -> second.packetsDropped.size() << "\n";
    std::cout << "  Throughput: " << i->second.rxBytes * 8.0
         / (i ->second.timeLastRxPacket.GetSeconds() - i -> second.timeFirstTxPacket.GetSeconds()) / 1000 / 1000  << " Mbps\n";
        std::cout << "  Time:   " << i->second.timeLastRxPacket.GetSeconds()- i->second.timeLastRxPacket.GetSeconds() << "\n";
        std::cout << "  TxTime:   " << i->second.timeFirstTxPacket.GetSeconds() << "\n";
        std::cout << "  RxTime:   " << i->second.timeLastRxPacket.GetSeconds() << "\n";
    }

  Simulator::Destroy ();
  NS_LOG_UNCOND("Running Wi-Fi Completes");
  return 0;
}
