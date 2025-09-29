#include <ESP8266WiFi.h>
#include <DHT.h>
#include <ESP8266WebServer.h>
#include "secrets.h"

// --- CONFIGURAÇÃO DO SENSOR DHT22 ---
#define DHTPIN D2
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

// --- CONFIGURAÇÃO DO LDR ---
#define LDRPIN A0

// --- CONFIGURAÇÃO DA REDE WI-FI ---
const char* ssid = "WIFI_SSID";  // Rede 2.4 GHz
const char* password = "WIFI_PASSWORD";

// --- CONFIGURAÇÃO DO THINGSPEAK ---
const char* tsServer = "api.thingspeak.com";
String writeAPIKey = "TS_WRITE_KEY";  // Sua Write Key

// --- SERVIDOR WEB ---
ESP8266WebServer server(80);

// --- VARIÁVEIS GLOBAIS ---
float temperatura = 0;
float umidade = 0;
int ldrValor = 0;

// --- Controle de tempo sem Wi-Fi ---
unsigned long tempoDesconectado = 0;             // 0 = está conectado
const unsigned long LIMITE_DESCONEXAO = 300000;  // 5 minutos em ms

// --- FUNÇÃO PARA ENVIAR DADOS AO THINGSPEAK ---
void enviarThingSpeak(float t, float h, int ldrVal) {
  WiFiClient client;
  if (client.connect(tsServer, 80)) {
    String nivel = "Média";
    if (ldrVal < 300) nivel = "Baixa";
    else if (ldrVal >= 700) nivel = "Alta";

    String url = "/update?api_key=" + writeAPIKey + "&field1=" + String(t) + "&field2=" + String(h) + "&field3=" + nivel;
    client.print(String("GET ") + url + " HTTP/1.1\r\n" + "Host: " + tsServer + "\r\n" + "Connection: close\r\n\r\n");
    client.stop();
    Serial.println("✅ Dados enviados ao ThingSpeak!");
  } else {
    Serial.println("❌ Falha ao conectar ThingSpeak.");
  }
}

// --- SETUP ---
void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("🔹 Iniciando ESP8266...");

  dht.begin();

  // Conecta Wi-Fi
  Serial.print("📶 Conectando ao Wi-Fi: ");
  Serial.println(ssid);
  int tentativas = 0;
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED && tentativas < 20) {
    delay(500);
    Serial.print(".");
    tentativas++;
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✅ Wi-Fi Conectado!");
    Serial.print("🌐 IP da ESP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\n❌ Não foi possível conectar ao Wi-Fi.");
  }

  // Rotas servidor local
  server.on("/", HTTP_GET, []() {
    server.send(200, "text/html", paginaHTML());
  });
  server.on("/data", HTTP_GET, []() {
    String lum = "Média";
    if (ldrValor < 300) lum = "Baixa";
    else if (ldrValor >= 700) lum = "Alta";

    String json = "{\"temperatura\":" + String(temperatura) + ",\"umidade\":" + String(umidade) + ",\"luminosidade\":\"" + lum + "\"}";
    server.send(200, "application/json", json);
  });
  server.begin();

  Serial.println("✅ Servidor local iniciado!");
}

// --- LOOP ---
void loop() {
  server.handleClient();

  // --- Reconexão Wi-Fi e reboot se necessário ---
  if (WiFi.status() != WL_CONNECTED) {
    if (tempoDesconectado == 0) {  // Primeira vez que detecta desconexão
      tempoDesconectado = millis();
      Serial.println("⚠️ Wi-Fi perdido. Iniciando tentativa de reconexão...");
    }

    // Tenta reconectar a cada passagem no loop
    WiFi.begin(ssid, password);
    delay(5000);  // espera um pouco antes de testar de novo

    // Se passar do limite, reinicia o ESP
    if (millis() - tempoDesconectado > LIMITE_DESCONEXAO) {
      Serial.println("🔁 Muito tempo sem Wi-Fi. Reiniciando ESP...");
      ESP.restart();
    }

    return;  // evita ler sensores ou enviar dados enquanto não está conectado
  } else {
    // Se reconectar, zera o tempo
    if (tempoDesconectado != 0) {
      Serial.println("✅ Wi-Fi reconectado!");
      tempoDesconectado = 0;
    }
  }

  static unsigned long ultimoTempo = 0;
  static unsigned long ultimoThingSpeak = 0;

  // Atualiza sensores a cada 2s
  if (millis() - ultimoTempo > 2000) {
    float t = dht.readTemperature();
    float h = dht.readHumidity();
    int ldr = analogRead(LDRPIN);

    if (!isnan(t)) temperatura = t;
    if (!isnan(h)) umidade = h;
    ldrValor = ldr;

    Serial.print("📊 Dados lidos -> Temp: ");
    Serial.print(temperatura);
    Serial.print(" °C | Hum: ");
    Serial.print(umidade);
    Serial.print(" % | LDR: ");
    Serial.println(ldrValor);

    ultimoTempo = millis();
  }

  // Envia dados ao ThingSpeak a cada 17s
  if (millis() - ultimoThingSpeak > 15000) {
    Serial.println("🚀 Enviando dados ao ThingSpeak...");
    enviarThingSpeak(temperatura, umidade, ldrValor);
    ultimoThingSpeak = millis();
  }
}

// --- DASHBOARD HTML LOCAL ---
String paginaHTML() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html lang="pt-BR">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>Estação Meteorológica</title>
<script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
<style>
body { font-family:sans-serif; text-align:center; color:#fff; transition: background 1s; margin:0; padding:0;}
h1 { margin-top:20px; margin-bottom:30px;}
.card { background: rgba(255,255,255,0.08); border-radius:15px; margin:20px auto; padding:20px; max-width:500px;}
canvas { background: rgba(255,255,255,0.05); border-radius:10px; padding:10px;}
.valor-atual { font-size:1.5em; margin-bottom:10px; font-weight:bold;}
</style>
</head>
<body>
<h1>🌤️ Estação Meteorológica</h1>
<div class="card">
<h2>🌡️ Temperatura</h2>
<div class="valor-atual" id="temp-atual">--</div>
<canvas id="graficoTemp"></canvas>
</div>
<div class="card">
<h2>💧 Umidade</h2>
<div class="valor-atual" id="umid-atual">--</div>
<canvas id="graficoUmid"></canvas>
</div>
<div class="card">
<h2>☀️ Luminosidade</h2>
<div class="valor-atual" id="lum-atual">Média</div>
<canvas id="graficoLum"></canvas>
</div>

<script>
let tempData=[], umidData=[], lumData=[], labels=[];
const ctxTemp=document.getElementById('graficoTemp').getContext('2d');
const ctxUmid=document.getElementById('graficoUmid').getContext('2d');
const ctxLum=document.getElementById('graficoLum').getContext('2d');

const graficoTemp=new Chart(ctxTemp,{type:'line',data:{labels, datasets:[{label:'Temperatura',data:tempData,borderColor:'red',backgroundColor:'rgba(255,0,0,0.2)',tension:0.4,fill:true}]},options:{responsive:true}});
const graficoUmid=new Chart(ctxUmid,{type:'line',data:{labels,datasets:[{label:'Umidade',data:umidData,borderColor:'blue',backgroundColor:'rgba(0,0,255,0.2)',tension:0.4,fill:true}]},options:{responsive:true}});
const graficoLum=new Chart(ctxLum,{type:'bar',data:{labels,datasets:[{label:'Luminosidade',data:lumData,backgroundColor:'yellow'}]},options:{responsive:true,scales:{y:{beginAtZero:true,max:3,ticks:{stepSize:1,callback:function(v){return ['Baixa','Média','Alta'][v];}}}}}});

// --- FUNÇÃO PARA COMBINAR FUNDO TEMPERATURA + LUMINOSIDADE ---
function atualizarFundo(temp, lum){
  if(lum === 'Baixa'){
    if(temp<20) document.body.style.background='linear-gradient(135deg,#0b3d91,#1a4f9d)';
    else if(temp<30) document.body.style.background='linear-gradient(135deg,#1e6eff,#4dabff)';
    else document.body.style.background='linear-gradient(135deg,#ff4c4c,#ff7373)';
  } else if(lum === 'Média'){
    if(temp<20) document.body.style.background='linear-gradient(135deg,#13294b,#2c3e70)';
    else if(temp<30) document.body.style.background='linear-gradient(135deg,#0055ff,#00aaff)';
    else document.body.style.background='linear-gradient(135deg,#ff3f3f,#ff6f6f)';
  } else { // Alta luminosidade
    if(temp<20) document.body.style.background='linear-gradient(135deg,#254e6e,#4f7aa0)';
    else if(temp<30) document.body.style.background='linear-gradient(135deg,#0088ff,#33ccff)';
    else document.body.style.background='linear-gradient(135deg,#ff2f2f,#ff5f5f)';
  }
}

function atualizarDados(){
  fetch('/data')
  .then(r=>r.json())
  .then(d=>{
    const agora=new Date().toLocaleTimeString();
    if(labels.length>=20){labels.shift(); tempData.shift(); umidData.shift(); lumData.shift();}
    labels.push(agora);
    tempData.push(d.temperatura);
    umidData.push(d.umidade);

    let lum=1;
    if(d.luminosidade=="Baixa") lum=0;
    else if(d.luminosidade=="Alta") lum=2;
    lumData.push(lum);

    document.getElementById('temp-atual').innerText=d.temperatura.toFixed(1)+" °C";
    document.getElementById('umid-atual').innerText=d.umidade.toFixed(1)+" %";
    document.getElementById('lum-atual').innerText=d.luminosidade;

    atualizarFundo(d.temperatura, d.luminosidade);

    graficoTemp.update(); graficoUmid.update(); graficoLum.update();
  });
}

setInterval(atualizarDados,2000);
</script>
</body>
</html>
)rawliteral";
  return html;
}
