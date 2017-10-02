/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2011 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
 *
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
 *
 * Author: Jaume Nin <jaume.nin@cttc.cat>
 */

#include "ns3/lte-helper.h"
#include "ns3/epc-helper.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/lte-module.h"
#include "ns3/applications-module.h"
#include "ns3/point-to-point-helper.h"
#include "ns3/config-store.h"
#include <ns3/netanim-module.h>
#include "ns3/flow-monitor-module.h"
#include <fstream>
#include <stdlib.h>
#include <iostream>
//#include "ns3/gtk-config-store.h"

using namespace ns3;
using namespace std;


NS_LOG_COMPONENT_DEFINE ("lwa_test");

int
main (int argc, char *argv[])
{

	//setup ue and enodeb
	uint16_t numberOfeNodeB =1;
	uint16_t numberOfue =3;
	double distance = 100.0;
	std::string dataRate = "1Gbps";

	// Command line arguments
	CommandLine cmd;
	cmd.AddValue ("numberOfUes", "Number of UEs", numberOfue);
	cmd.AddValue ("numberOfEnbs", "Number of eNBs", numberOfeNodeB);
	cmd.AddValue ("dataRate", "Application data ate", dataRate);
	cmd.AddValue("distance", "Distance between eNBs [m]", distance);
	cmd.Parse(argc, argv);

	//  LogComponentEnable("OnOffApplication", LOG_LEVEL_INFO);
	Time::SetResolution (Time::NS);

	Ptr<LteHelper> lteHelper = CreateObject<LteHelper> ();
	Ptr<PointToPointEpcHelper>  epcHelper = CreateObject<PointToPointEpcHelper> ();
	lteHelper->SetEpcHelper (epcHelper);

	ConfigStore inputConfig;
	inputConfig.ConfigureDefaults();

    // parse again so you can override default values from the command line
	cmd.Parse(argc, argv);

	Config::SetDefault ("ns3::LteAmc::AmcModel", EnumValue (LteAmc::PiroEW2010));
	Config::SetDefault ("ns3::LteAmc::Ber",DoubleValue (0.00005));

	/*PathlossModel is ItuR1411NlosOverRooftopPropagationLossModel*/
	Config::SetDefault("ns3::LteHelper::PathlossModel", StringValue("ns3::ItuR1411NlosOverRooftopPropagationLossModel"));

	/*UlBandwidth/DlBandwidth = Network bandwith (MHz)*/
	/*UlBandwidth/DlBandwidth (25,50,75,100) = Network bandwith 5,10,15,20(MHz)*/
	Config::SetDefault ("ns3::LteEnbNetDevice::UlBandwidth",UintegerValue (100));
	Config::SetDefault ("ns3::LteEnbNetDevice::DlBandwidth",UintegerValue (100));

	/*DlEarfcn 100 = Downlink 2120(MHz)*/
	/*UlEarfcn 18100 = Uplink 1930(MHz)*/
	Config::SetDefault ("ns3::LteEnbNetDevice::DlEarfcn",UintegerValue (100));
	Config::SetDefault ("ns3::LteEnbNetDevice::UlEarfcn",UintegerValue (18000));

	/*Scheduler is proportional fair scheduling*/
	Config::SetDefault ("ns3::LteHelper::Scheduler",StringValue("ns3::PfFfMacScheduler"));

	/**/
	lteHelper->SetEnbAntennaModelType ("ns3::IsotropicAntennaModel");
	Config::SetDefault ("ns3::LteEnbPhy::TxPower", DoubleValue (46));
	Config::SetDefault ("ns3::LteUePhy::TxPower", DoubleValue (20));

	/*Antenna configuration is MIMO Spatial Multiplexity (2 layers)*/
	Config::SetDefault ("ns3::LteEnbRrc::EpsBearerToRlcMapping",  EnumValue(ns3::LteEnbRrc::RLC_AM_ALWAYS));
	Config::SetDefault ("ns3::LteEnbRrc::DefaultTransmissionMode", UintegerValue(2));


	/*Transmission mode is Tcp(NewReno)*/
	Config::SetDefault ("ns3::TcpL4Protocol::SocketType", StringValue ("ns3::TcpNewReno"));

	Ptr<Node> pgw = epcHelper->GetPgwNode ();

	// Create a single RemoteHost
	NodeContainer server;
	server.Create (1);
	Ptr<Node> remoteHost = server.Get (0);

	NodeContainer epcnodes;
	epcnodes.Add(pgw);
	epcnodes.Add(server);

	InternetStackHelper internet;
	internet.Install (server);

	// Create the Internet
	PointToPointHelper p2ph;
	p2ph.SetDeviceAttribute ("DataRate", DataRateValue (DataRate ("1Gb/s")));
	p2ph.SetDeviceAttribute ("Mtu", UintegerValue (1500));
	p2ph.SetChannelAttribute ("Delay", TimeValue (Seconds (0.010)));
	NetDeviceContainer internetDevices = p2ph.Install (pgw, remoteHost);
	Ipv4AddressHelper ipv4h;
	ipv4h.SetBase ("1.0.0.0", "255.0.0.0");
	Ipv4InterfaceContainer internetIpIfaces = ipv4h.Assign (internetDevices);

	Ipv4StaticRoutingHelper ipv4RoutingHelper;
	Ptr<Ipv4StaticRouting> remoteHostStaticRouting = ipv4RoutingHelper.GetStaticRouting (remoteHost->GetObject<Ipv4> ());
	remoteHostStaticRouting->AddNetworkRouteTo (Ipv4Address ("7.0.0.0"), Ipv4Mask ("255.0.0.0"), 1);

	NodeContainer ueNodes;
	NodeContainer enbNodes;
	enbNodes.Create(numberOfeNodeB);
	ueNodes.Create(numberOfue);

//	Ptr<ListPositionAllocator> positionAllocepc = CreateObject<ListPositionAllocator> ();
//	for (uint16_t i = 0; i < epcnodes.GetN(); i++)
//		      {
//		    	positionAllocepc->Add (Vector(500 * (i+1), 0, 0));
//		      }
//	MobilityHelper mobilityepc;
//	mobilityepc.SetMobilityModel("ns3::ConstantPositionMobilityModel");
//	mobilityepc.SetPositionAllocator(positionAllocepc);
//	mobilityepc.Install(epcnodes);


	// Install Mobility Model
//	Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
//	for (uint16_t i = 0; i < numberOfeNodeB; i++){
//
//			positionAlloc->Add (Vector(distance * i, 0, 0));
//		}
	/* Mobility model */
	Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
	positionAlloc->Add (Vector (0.0, 0.0, 0.0));
	positionAlloc->Add (Vector (1.0, 1.0, 0.0));

	MobilityHelper mobility;
	mobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel",
	                             "Bounds", RectangleValue (Rectangle (-50, 50, -50, 50)));
	mobility.SetPositionAllocator(positionAlloc);
	mobility.Install(ueNodes);

	MobilityHelper mobilityepc;
	mobilityepc.SetMobilityModel("ns3::ConstantPositionMobilityModel");
	mobilityepc.SetPositionAllocator(positionAlloc);
	mobilityepc.Install(enbNodes);
	mobilityepc.Install(epcnodes);

	// Install LTE Devices to the nodes
	NetDeviceContainer enbLteDevs = lteHelper->InstallEnbDevice (enbNodes);
	NetDeviceContainer ueLteDevs = lteHelper->InstallUeDevice (ueNodes);

	// Install the IP stack on the UEs
	internet.Install (ueNodes);
	Ipv4InterfaceContainer ueIpIface;
	ueIpIface = epcHelper->AssignUeIpv4Address (NetDeviceContainer (ueLteDevs));
	// Assign IP address to UEs, and install applications
	for (uint32_t u = 0; u < ueNodes.GetN (); ++u)
		{
			Ptr<Node> ueNode = ueNodes.Get (u);
			// Set the default gateway for the UE
			Ptr<Ipv4StaticRouting> ueStaticRouting = ipv4RoutingHelper.GetStaticRouting (ueNode->GetObject<Ipv4> ());
			ueStaticRouting->SetDefaultRoute (epcHelper->GetUeDefaultGatewayAddress (), 1);
		}

	lteHelper -> Attach(ueLteDevs);
//	lteHelper -> Attach(ueLteDevs.Get(0), enbLteDevs.Get(0));

	// Install and start applications on UEs and remote host
	uint16_t dlPort = 1234;
	ApplicationContainer clientApps;
	ApplicationContainer serverApps;

	for (uint32_t u = 0; u < ueNodes.GetN (); ++u)
		{
			OnOffHelper onOffHelper ("ns3::TcpSocketFactory", InetSocketAddress (ueIpIface.GetAddress(u), dlPort));
			onOffHelper.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
			onOffHelper.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
			onOffHelper.SetAttribute ("DataRate", DataRateValue (DataRate (dataRate)));
			onOffHelper.SetAttribute ("PacketSize",UintegerValue(5000));
			serverApps.Add(onOffHelper.Install(remoteHost));//tcp sender

			PacketSinkHelper sink ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), dlPort));
			clientApps.Add( sink.Install (ueNodes.Get (u)));//tcp reciever

			dlPort++;
		}
	serverApps.Start (Seconds (0.0));
	clientApps.Start (Seconds (1.0));
	Simulator::Stop(Seconds(10));

	AnimationInterface anim ("lte-exp.xml");
	anim.SetMaxPktsPerTraceFile(50000);
	anim.UpdateNodeDescription(ueNodes.Get(0), "UE1");
	anim.UpdateNodeDescription(ueNodes.Get(1), "UE2");
	anim.UpdateNodeDescription(ueNodes.Get(2), "UE3");
	anim.UpdateNodeDescription(epcnodes.Get (0), "RemoteHost");
	anim.UpdateNodeDescription(enbNodes.Get(0), "eNB");



	FlowMonitorHelper flowmon;
	Ptr<FlowMonitor> monitor;
	monitor = flowmon.Install(server);
	monitor = flowmon.Install(ueNodes);

	NS_LOG_UNCOND("Running Lte");
	Simulator::Run();

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

	Simulator::Destroy();

	NS_LOG_UNCOND("Running Lte Completes");

	return 0;

}

