// Includes de ns3
#include "ns3/ptr.h"
#include "ns3/nstime.h"
#include "ns3/data-rate.h"
#include "ns3/command-line.h"
#include "ns3/node-container.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/csma-helper.h"
#include "ns3/net-device-container.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/ipv4-interface-container.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/udp-echo-helper.h"
#include "ns3/application-container.h"
#include "ns3/simulator.h"
#include "ns3/log.h"
#include "ns3/trace-helper.h"
#include "ns3/applications-module.h"
#include "ns3/on-off-helper.h"
#include "ns3/packet-sink-helper.h"
#include "ns3/string.h"
#include "ns3/three-gpp-http-helper.h"

// Definiciones de constantes
#define NUM_CLIENTES 5
#define TASA_ENVIO   "500Mb/s"
#define RETARDO      "0.5ms"
#define PUERTO       9

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("PROYECTO");

void escenario(uint32_t num_clientes, 
			   DataRate tasa, 
			   Time retardo);
			   
		   
//void ManejadorPeticiones(Ptr<Socket> socket);
//void ConfigurarServidorVideollamadas(NodeContainer &servidor, Ipv4InterfaceContainer &serverAddr);

// Procesamos los parámetros de entrada, generamos el escenario y
// lanzamos la simulación
int main(int argc, char *argv[])
{
  Time::SetResolution(Time::NS);

  uint32_t num_clientes = NUM_CLIENTES;
  DataRate tasa 		= DataRate(TASA_ENVIO);
  Time 	   retardo	    = Time(RETARDO);

  CommandLine cmd;
  cmd.AddValue("clientes", "Número de clientes del escenario en cada sede", num_clientes);
  cmd.AddValue("regimenBinario", "Velocidad de transmisión", tasa);
  cmd.AddValue("retardoProp", "Retardo de propagación", retardo);
  cmd.Parse(argc, argv);
	
  //Llamamos a la función escenario y le pasamos los parámetros deseados
  escenario(num_clientes, tasa, retardo);

  //Simulator::Stop("15s")
  Simulator::Run();

  return 0;
}

// Creación del escenario.
// Parámetros:
//   - número de clientes de eco
//   - velocidad de transmisión
//   - retardo de propagación del canal
void
escenario (uint32_t num_clientes,
           DataRate tasa,
           Time     retardo)
{
  
  NodeContainer c_clientes_sede_1 (num_clientes);
  NodeContainer c_clientes_sede_2 (num_clientes);
  NodeContainer c_sede_1 ();
  NodeContainer c_sede_2 ();
  NodeContainer c_sedes ();
  
  // Crear nodos para el servidor, el router y los clientes
  Ptr<Node> n_servidor = CreateObject<Node> ();
  Ptr<Node> n_router_sede_1 = CreateObject<Node> (); 
  
  Ptr<Node> n_router_sede_2 = CreateObject<Node> ();
  Ptr<Node> n_isp = CreateObject<Node>(); // Nuevo nodo para el ISP
  
  c_sede_1.Add(n_servidor);
  c_sede_1.Add(n_router_sede_1);
  c_sede_1.Add(c_clientes_sede_1);
  
  
  c_sede_2.Add(n_router_sede_2);
  c_sede_2.Add(c_clientes_sede_2);
  
  c_sedes.Add(c_sede_1);
  c_sedes.Add(c_sede_2);


  // Instalamos la pila en todos los nodos
  NS_LOG_INFO("Instalamos la pila en todos los nodos");
  InternetStackHelper h_pila;
  h_pila.SetIpv6StackInstall (false);
  //h_pila.Install (NodeContainer (c_clientes_sede_1, n_servidor, n_router_sede_1, n_router_sede_2, c_clientes_sede_2, n_isp));
  h_pila.Install (NodeContainer (c_sede_1, c_sede_2, n_isp));
  
  

  // Creamos el dispositivo CSMA-CD para la conexión en la SEDE 1
  NS_LOG_INFO("Creamos el dispositivo CSMA-CD para la conexión en la SEDE 1");
  CsmaHelper h_csma_sede_1;
  h_csma_sede_1.SetChannelAttribute ("DataRate", DataRateValue (tasa));
  h_csma_sede_1.SetChannelAttribute ("Delay", TimeValue (retardo));
  NetDeviceContainer Sede1 = h_csma_sede_1.Install (NodeContainer (c_sede_1));

  // Creamos el dispositivo CSMA-CD para la conexión entre sedes (routers)
  NS_LOG_INFO("Creamos el dispositivo CSMA-CD para la conexión entre sedes (routers)");
  CsmaHelper h_csma_enlace;
  h_csma_enlace.SetChannelAttribute ("DataRate", DataRateValue (tasa));
  h_csma_enlace.SetChannelAttribute ("Delay", TimeValue (retardo));
  NetDeviceContainer Enlace = h_csma_enlace.Install (NodeContainer (n_router_sede_1, n_router_sede_2));

  // Creamos el dispositivo CSMA-CD para la conexión en la SEDE 2
  NS_LOG_INFO("Creamos el dispositivo CSMA-CD para la conexión en la SEDE 2");
  CsmaHelper h_csma_sede_2;
  h_csma_sede_2.SetChannelAttribute ("DataRate", DataRateValue (tasa));
  h_csma_sede_2.SetChannelAttribute ("Delay", TimeValue (retardo));
  NetDeviceContainer Sede2 = h_csma_sede_2.Install (NodeContainer (c_sede_2));

  // Dispositivo CSMA-CD para la conexión entre el ISP y el router de la SEDE 1
  NS_LOG_INFO("Dispositivo CSMA-CD para la conexión entre el ISP y el router de la SEDE 1");
  CsmaHelper h_csma_isp;
  h_csma_isp.SetChannelAttribute("DataRate", DataRateValue(tasa));
  h_csma_isp.SetChannelAttribute("Delay", TimeValue(retardo));
  NetDeviceContainer IspToRouterSede1 = h_csma_isp.Install(NodeContainer(n_router_sede_1, n_isp));

  // Definimos el rango de direcciones a utilizar en el escenario
  NS_LOG_INFO("Definimos el rango de direcciones a utilizar en el escenario");
  Ipv4AddressHelper h_direcciones_sede1 ("10.1.10.0", "255.255.255.0");
  Ipv4AddressHelper h_direcciones_enlace ("10.1.1.0", "255.255.255.0");
  Ipv4AddressHelper h_direcciones_sede2 ("10.1.20.0", "255.255.255.0");
  Ipv4AddressHelper h_direcciones_isp("30.30.30.0", "255.255.255.0");

  
  // Asignamos direcciones IPv4 a los dispositivos
  NS_LOG_INFO("Asignamos direcciones IPv4 a los dispositivos");
  Ipv4InterfaceContainer sede1Interfaces = h_direcciones_sede1.Assign (Sede1);
  Ipv4InterfaceContainer enlaceInterfaces = h_direcciones_enlace.Assign (Enlace);
  Ipv4InterfaceContainer sede2Interfaces = h_direcciones_sede2.Assign (Sede2);
  Ipv4InterfaceContainer ispToRouterSede1Interfaces = h_direcciones_isp.Assign(IspToRouterSede1);

  //ConfigurarServidorVideollamadas(c_clientes_sede_1,sede1Interfaces);

  Ipv4Address servidorAddress = sede1Interfaces.GetAddress(0); // Obtén la dirección IP del servidor
  NS_LOG_INFO("La dirección IP del servidor es: " << servidorAddress);
  std::cout << "La dirección IP del servidor es: " << servidorAddress << std::endl;


  // Generamos las tablas de encaminamiento de los nodos en el escenario
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  /*
  // Definimos el parámetro de la aplicación servidor:
  //    - indicando el puerto en el que corre el servicio
  UdpEchoServerHelper h_echoServer(PUERTO);
  // Instalamos una aplicación servidor en el nodo servidor
  ApplicationContainer c_serverApps = h_echoServer.Install(n_servidor); */



  

    NS_LOG_INFO("instalo servers en todos madge");
    uint16_t port = PUERTO;
    UdpServerHelper server(port);
    ApplicationContainer apps = server.Install(c_sedes);
    apps.Start(Seconds(1.0));
    apps.Stop(Seconds(10.0));

    //ApplicationContainer apps2;
 
    NS_LOG_INFO("Creo los cleitnes");
    uint32_t MaxPacketSize = 1024;
    Time interPacketInterval = Seconds(0.250);
    uint32_t maxPacketCount = 120;
	
	Ipv4Address destino = sede1Interfaces.GetAddress(3);
	
    UdpClientHelper client(destino, port);
    client.SetAttribute("MaxPackets", UintegerValue(maxPacketCount));
    client.SetAttribute("Interval", TimeValue(interPacketInterval));
    client.SetAttribute("PacketSize", UintegerValue(MaxPacketSize));
    apps = client.Install(c_clientes_sede_2.Get(3));
    apps.Start(Seconds(2.0));
    apps.Stop(Seconds(10.0));
	
	
    //apps2 = client.Install(c_clientes_sede_2);
    //apps2.Start(Seconds(2.0));
    //apps2.Stop(Seconds(10.0));
  //httpServidor.Start(Seconds(1.0));
  //httpServidor.Stop(Seconds(10.0));
  //httpCliente.Start(Seconds(2.0));
  //httpCliente.Stop(Seconds(9.0));


 

 /*
  
  // Configuración de OnOffApplication para los clientes
  OnOffHelper onOffHelper("ns3::UdpSocketFactory", Address(InetSocketAddress(sede1Interfaces.GetAddress(0), PUERTO)));
  onOffHelper.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
  onOffHelper.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
  onOffHelper.SetAttribute("DataRate", DataRateValue(DataRate(TASA_ENVIO))); 

  // Instalación de OnOffApplication en los nodos cliente de la Sede 1
  ApplicationContainer clientAppsSede1 = onOffHelper.Install(c_clientes_sede_1);
  clientAppsSede1.Start(Seconds(1.0));
  clientAppsSede1.Stop(Seconds(10.0));

  // Instalación de OnOffApplication en los nodos cliente de la Sede 2
  ApplicationContainer clientAppsSede2 = onOffHelper.Install(c_clientes_sede_2);
  clientAppsSede2.Start(Seconds(1.0));
  clientAppsSede2.Stop(Seconds(10.0));

*/
  // Instalamos una aplicación cliente en el ISP para probar que funciona
  ApplicationContainer clientAppsISP = onOffHelper.Install(n_isp);
  clientAppsISP.Start(Seconds(1.0));
  clientAppsISP.Stop(Seconds(10.0)); 
  
  // Habilitamos el registro de paquetes para el servidor
  h_csma_sede_1.EnablePcap ("salida_server_sede1", Sede1.Get (0));
  // Habilitamos el registro de paquetes para los clientes y asi mostrar el funcionamiento mejor
  h_csma_sede_2.EnablePcap ("salida_cliente_sede2", Sede2.Get (0));
}

/*
void ConfigurarServidorVideollamadas(NodeContainer &servidor, Ipv4InterfaceContainer &serverAddr)
{
    TypeId tid = TypeId::LookupByName("ns3::UdpSocketFactory");
    Ptr<Socket> recvSink = Socket::CreateSocket(servidor.Get(0), tid);
    InetSocketAddress local = InetSocketAddress(serverAddr.GetAddress(0), PUERTO);
    recvSink->Bind(local);
    recvSink->SetRecvCallback(MakeCallback(&ManejadorPeticiones, recvSink));
}

void ManejadorPeticiones(Ptr<Socket> socket)
{
    NS_LOG_FUNCTION(socket);
    Ptr<Packet> packet;
    Address fromAddress;

   
    while ((packet = socket->RecvFrom(fromAddress)))
    {
        if (packet->GetSize() > 0)
        {
            NS_LOG_INFO("Recibida petición de " << InetSocketAddress::ConvertFrom(fromAddress).GetIpv4());
	    std::cout << "Recibida petición de ";
            // Aquí puede agregar la lógica para manejar la petición
            // Por ejemplo, enviar un flujo de video simulado de vuelta al cliente
            uint32_t tamañoPaqueteVideo = 1024; // Establezca el tamaño apropiado para su simulación
            Ptr<Packet> replyPacket = Create<Packet>(tamañoPaqueteVideo);
            socket->SendTo(replyPacket, 0, fromAddress);
        }
    }
}
*/
