void loop() 
{
  int packetSize = udp.parsePacket();
  if (packetSize)
  {
    char incomingPacket[255];
    Serial.printf("Received %d bytes from %s, port %d\n", packetSize, udp.remoteIP().toString().c_str(), udp.remotePort());
    int len = udp.read(incomingPacket, 255);
    if (len > 0)
    {
      incomingPacket[len] = 0;
    }
    Serial.printf("Incoming UDP packet: %s\n", incomingPacket);
  }
  
  unsigned long currentMillis = millis();

  previousMillis = currentMillis;
  taskRestart(currentMillis, previousMillisReboot);
  previousMillisConfig = taskConfig(currentMillis, previousMillisConfig);
  mainProcess();

  if (currentMillis - previousMillisPing >= PING_INTERVAL) {
    previousMillisPing = currentMillis;
    pingServer();
  }

  if (currentMillis - previousMillisReport >= REPORT_INTERVAL) {
    previousMillisReport = currentMillis;
    STATUS_REPORT_SEND = false;
  }

  if (!STATUS_REPORT_SEND) {
    //sending status report
    
    STATUS_REPORT_SEND = true;
  }

  int n = WiFi.scanComplete();
  if(n >= 0)
  {
    Serial.printf("%d network(s) found\n", n);
    for (int i = 0; i < n; i++)
    {
      Serial.printf("%d: %s, Ch:%d (%ddBm) %s\n", i+1, WiFi.SSID(i).c_str(), WiFi.channel(i), WiFi.RSSI(i), WiFi.encryptionType(i) == ENC_TYPE_NONE ? "open" : "");
    }
    WiFi.scanDelete();
  }

  tinyUPnP.updatePortMappings(600000);
}