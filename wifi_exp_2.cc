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
#include "ns3/netanim-module.h"
#include "ns3/flow-monitor-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("ThirdScriptExample");

int 
main (int argc, char *argv[])
{

  uint32_t nCsma = 1;
  uint32_t nWifi = 1;
  std::string phyRate = "HtMcs7";
  std::string dataRate = "100Mbps";
  uint32_t payloadSize = 1472;

  Time::SetResolution (Time::NS);

  CommandLine cmd;
  cmd.AddValue ("nCsma", "Number of \"extra\" CSMA nodes/devices", nCsma);
  cmd.AddValue ("nWifi", "Number of wifi STA devices", nWifi);
  cmd.Parse (argc,argv);

  /* Set channel width */
  Config::Set ("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/ChannelWidth", UintegerValue (20));
  /*Transmission mode is Tcp(NewReno)*/
  Config::SetDefault ("ns3::TcpL4Protocol::SocketType", StringValue ("ns3::TcpNewReno"));

  NodeContainer p2pNodes;
  p2pNodes.Create (2);

  PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
  pointToPoint.SetDeviceAttribute ("Mtu", UintegerValue (1500));
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
//  wifiHelper.SetRemoteStationManager ("ns3::AarfWifiManager");
  wifiHelper.SetStandard(WIFI_PHY_STANDARD_80211n_2_4GHZ );

  /*Set up Channel*/
  YansWifiChannelHelper channel = YansWifiChannelHelper::Default ();
  channel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
  channel.AddPropagationLoss ("ns3::LogDistancePropagationLossModel","ReferenceLoss", DoubleValue (48.0));

  YansWifiPhyHelper wifiPhy = YansWifiPhyHelper::Default ();
  wifiPhy.SetChannel (channel.Create ());
  wifiPhy.Set ("TxPowerStart", DoubleValue (10.0));
  wifiPhy.Set ("TxPowerEnd", DoubleValue (10.0));
  wifiPhy.Set ("TxPowerLevels", UintegerValue (1));
  wifiPhy.Set ("TxGain", DoubleValue (0));
  wifiPhy.Set ("RxGain", DoubleValue (0));
  wifiPhy.Set ("RxNoiseFigure", DoubleValue (10));
  wifiPhy.Set ("CcaMode1Threshold", DoubleValue (-79));
  wifiPhy.Set ("EnergyDetectionThreshold", DoubleValue (-79 + 3));
  wifiPhy.SetErrorRateModel ("ns3::YansErrorRateModel");
  wifiHelper.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
                                      "DataMode", StringValue (phyRate),
                                      "ControlMode", StringValue ("HtMcs0"));


  /**Install mac address for sta/ap**/
  WifiMacHelper mac;
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

  mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
                                 "MinX", DoubleValue (0.0),
                                 "MinY", DoubleValue (0.0),
                                 "DeltaX", DoubleValue (5.0),
                                 "DeltaY", DoubleValue (10.0),
                                 "GridWidth", UintegerValue (5),
                                 "LayoutType", StringValue ("RowFirst"));

//  /* Mobility model */
//  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
//  positionAlloc->Add (Vector (0.0, 0.0, 0.0));
//  positionAlloc->Add (Vector (1.0, 1.0, 0.0));

//  mobility.SetPositionAllocator (positionAlloc);

  mobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel",
                             "Bounds", RectangleValue (Rectangle (-50, 50, -50, 50)));
  mobility.Install (wifiStaNodes);

  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (wifiApNode);

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

  address.SetBase ("10.1.3.0", "255.255.255.0");
  Ipv4InterfaceContainer apInterface;
  Ipv4InterfaceContainer staInterface;
  staInterface = address.Assign (staDevices);
  apInterface = address.Assign (apDevices);

  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  PacketSinkHelper sinkHelper ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), 1235));
  ApplicationContainer serverApps;
  ApplicationContainer clientApps = sinkHelper.Install (wifiStaNodes);

  for (uint32_t i = 0;i < wifiStaNodes.GetN(); i++){
	  /* Install TCP/UDP Transmitter on the station */
	  OnOffHelper server ("ns3::TcpSocketFactory", (InetSocketAddress (staInterface.GetAddress (i), 1235)));
	  server.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
	  server.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
	  server.SetAttribute ("DataRate", DataRateValue (DataRate (dataRate)));
	  server.SetAttribute ("PacketSize", UintegerValue (payloadSize));
//	  server.SetAttribute("MaxBytes", UintegerValue (1000000000));
	  serverApps = server.Install (csmaNodes.Get (nCsma));
  }

  clientApps.Start (Seconds (1.0));
  serverApps.Start (Seconds (1.0));
  Simulator::Stop (Seconds (25.0));

  AnimationInterface anim ("wifi-exp_2.xml");
  anim.SetMaxPktsPerTraceFile(9999999999999);
  anim.UpdateNodeDescription(wifiStaNodes.Get(0), "UE1");
//  anim.UpdateNodeDescription(wifiStaNodes.Get(1), "UE2");
//  anim.UpdateNodeDescription(wifiStaNodes.Get(2), "UE3");
  anim.UpdateNodeDescription(p2pNodes.Get(1), "AP");
//  anim.SetConstantPosition(p2pNodes.Get(1), 0.2, 0.2);
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
		std::cout << "  Delay:   " << (i->second.delaySum / i->second.rxPackets)   << "\n";
		std::cout << "  Lost Packets:   " << ((i->second.lostPackets )) << "\n";
		std::cout << "  Packet Dropped:   " << i -> second.packetsDropped.size() << "\n";
		std::cout << "  Throughput: " << i->second.rxBytes * 8.0
				 / (i ->second.timeLastRxPacket.GetSeconds() - i -> second.timeFirstTxPacket.GetSeconds()) / 1000 / 1000  << " Mbps\n";
		//          std::cout << "  Packet Loss:   " << ((i->second.txPackets - i->second.rxPackets) / i->second.txPackets) * 2 << "\n";
		}


  Simulator::Destroy ();
  return 0;
}
