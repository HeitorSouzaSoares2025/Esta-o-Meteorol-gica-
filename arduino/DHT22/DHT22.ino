#include <ESP8266WiFi.h>          // Biblioteca para conexão Wi-Fi no ESP8266
#include <DHT.h>                   // Biblioteca para sensores DHT (temperatura e umidade)
#include <ESP8266WebServer.h>      // Biblioteca para criar servidor web no ESP8266

// --- CONFIGURAÇÃO SENSOR DHT22 ---
#define DHTPIN D2                  // Pino digital onde o sensor DHT22 está conectado
#define DHTTYPE DHT22              // Tipo de sensor (DHT22)
DHT dht(DHTPIN, DHTTYPE);          // Cria objeto 'dht' para leitura de temperatura e umidade

// --- LED DE STATUS ---
#define LED_PIN D1                 // Pino do LED de status
enum EstadoSistema {INICIALIZANDO, CONECTANDO_WIFI, NORMAL, ERRO_SENSOR}; // Estados possíveis do sistema
EstadoSistema estadoAtual = INICIALIZANDO;   // Estado inicial

unsigned long ultimaTroca = 0;     // Guarda o último instante que o LED mudou de estado (para piscar)
bool ledLigado = false;            // Estado atual do LED (ligado/desligado)

// --- LDR ---
#define LDRPIN A0                  // Pino analógico onde o LDR está conectado

// --- REDE WI-FI ---
const char* ssid = "HEITOR_5G";    // Nome da rede Wi-Fi
const char* password = "0107081970Nr"; // Senha da rede Wi-Fi

// --- ADMINISTRADORES ---
struct Admin {
  const char* nome;
  const char* senha;
};

// Lista de administradores que podem reiniciar a ESP
Admin admins[] = {
  {"HeitorSS", "26121970"},        // Administrador principal
  {"OutroADM", "123456"}           // Exemplo de outro administrador
};
int numAdmins = sizeof(admins) / sizeof(Admin); // Calcula quantos admins foram cadastrados

// --- THINGSPEAK ---
const char* tsServer = "api.thingspeak.com";   // Servidor do ThingSpeak
String writeAPIKey = "R7JASIKSUV7QEOZ8";       // Chave para enviar dados ao ThingSpeak

// --- SERVIDOR WEB ---
ESP8266WebServer server(80);        // Cria servidor web na porta 80

// --- VARIÁVEIS GLOBAIS ---
float temperatura = 0;              // Temperatura lida do sensor
float umidade = 0;                  // Umidade lida do sensor
int ldrValor = 0;                   // Valor do LDR lido (0 a 1023)

// --- TEMPO SEM WI-FI ---
unsigned long tempoDesconectado = 0;          // Armazena quando a ESP perdeu Wi-Fi
const unsigned long LIMITE_DESCONEXAO = 300000; // 5 minutos (em milissegundos)

// --- FUNÇÃO PARA ENVIAR DADOS AO THINGSPEAK ---
void enviarThingSpeak(float t, float h, int ldrVal) {
  WiFiClient client;                 // Cria cliente para conexão TCP
  if (client.connect(tsServer, 80)) {        // Conecta ao servidor ThingSpeak

    // Converte valor LDR em nível (0=Baixa, 1=Média, 2=Alta)
    int nivel = 1;                   // Padrão: Média
    if (ldrVal < 300) nivel = 0;     // Baixa luminosidade
    else if (ldrVal >= 700) nivel = 2; // Alta luminosidade

    // Monta URL da requisição GET
    String url = "/update?api_key=" + writeAPIKey +
                 "&field1=" + String(t) +
                 "&field2=" + String(h) +
                 "&field3=" + String(nivel);   // envia nível de luminosidade

    // Envia requisição HTTP
    client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                 "Host: " + tsServer + "\r\n" +
                 "Connection: close\r\n\r\n");

    client.stop();                    // Fecha a conexão
    Serial.println("✅ Dados enviados ao ThingSpeak!");
  } else {
    Serial.println("❌ Falha ao conectar ThingSpeak.");
  }
}

// --- SETUP ---
void setup() {
  Serial.begin(115200);               // Inicializa porta serial para debug
  pinMode(LED_PIN, OUTPUT);           // Define pino do LED como saída
  digitalWrite(LED_PIN, LOW);         // Desliga LED inicialmente

  dht.begin();                        // Inicializa sensor DHT22

  estadoAtual = INICIALIZANDO;        // Estado inicial do sistema
  delay(1000);                        // Aguarda 1 segundo
  Serial.println("🔹 Iniciando ESP8266...");

  // --- Conectar Wi-Fi ---
  estadoAtual = CONECTANDO_WIFI;      // Atualiza estado
  WiFi.begin(ssid, password);         // Inicia conexão Wi-Fi
  Serial.print("📶 Conectando ao Wi-Fi");
  int tentativas = 0;
  while (WiFi.status() != WL_CONNECTED && tentativas < 20) { // Tenta conectar por 20*0.5=10s
    delay(500);
    Serial.print(".");
    tentativas++;
  }
  if (WiFi.status() == WL_CONNECTED) {
    estadoAtual = NORMAL;
    Serial.println("\n✅ Wi-Fi Conectado!");
    Serial.print("🌐 IP da ESP: ");
    Serial.println(WiFi.localIP());   // Mostra IP da ESP
  } else {
    Serial.println("\n❌ Não foi possível conectar ao Wi-Fi.");
    estadoAtual = CONECTANDO_WIFI;
  }

  // --- Rotas do servidor ---
  server.on("/", HTTP_GET, []() {                     // Página principal
    server.send(200, "text/html", paginaHTML());
  });

  server.on("/data", HTTP_GET, []() {                // Retorna dados em JSON
    String lum = "Média";
    if (ldrValor < 300) lum = "Baixa";
    else if (ldrValor >= 700) lum = "Alta";

    String json = "{\"temperatura\":" + String(temperatura) +
                  ",\"umidade\":" + String(umidade) +
                  ",\"luminosidade\":\"" + lum + "\"}";
    server.send(200, "application/json", json);
  });

  // --- Reset via múltiplos admins ---
  server.on("/reset", HTTP_GET, [&]() {
    bool autorizado = false;

    // Verifica se algum admin está autenticado
    for (int i = 0; i < numAdmins; i++) {
      if (server.authenticate(admins[i].nome, admins[i].senha)) {
        autorizado = true;
        break;
      }
    }

    if (!autorizado) {                     // Se não autorizado, pede login
      server.requestAuthentication();
      return;
    }

    server.send(200, "text/plain", "✅ ESP reiniciando..."); // Mensagem
    delay(100);
    ESP.restart();                         // Reinicia ESP
  });

  server.begin();                            // Inicia servidor web
  Serial.println("✅ Servidor local iniciado!");
}

// --- LOOP ---
void loop() {
  unsigned long agora = millis();            // Tempo atual em ms

  // ---------- Controle do LED ----------
  switch (estadoAtual) {
    case NORMAL:
      digitalWrite(LED_PIN, HIGH);          // LED sempre aceso
      break;
    case ERRO_SENSOR:
      if (agora - ultimaTroca >= 200) {     // Pisca rápido
        ledLigado = !ledLigado;
        digitalWrite(LED_PIN, ledLigado);
        ultimaTroca = agora;
      }
      break;
    case CONECTANDO_WIFI:
      if (agora - ultimaTroca >= 500) {     // Pisca devagar
        ledLigado = !ledLigado;
        digitalWrite(LED_PIN, ledLigado);
        ultimaTroca = agora;
      }
      break;
    case INICIALIZANDO:
      digitalWrite(LED_PIN, HIGH);          // LED aceso
      break;
  }

  // ---------- Servidor Local ----------
  server.handleClient();                     // Verifica requisições do servidor

  // ---------- Reconexão Wi-Fi ----------
  if (WiFi.status() != WL_CONNECTED) {
    if (tempoDesconectado == 0) {
      tempoDesconectado = millis();
      Serial.println("⚠ Wi-Fi perdido. Tentando reconectar...");
      estadoAtual = CONECTANDO_WIFI;
    }
    WiFi.begin(ssid, password);             // Tenta reconectar
    delay(5000);
    if (millis() - tempoDesconectado > LIMITE_DESCONEXAO) { // Se desconectado >5min
      Serial.println("🔁 Muito tempo sem Wi-Fi. Reiniciando ESP...");
      ESP.restart();
    }
    return;                                 // Sai do loop
  } else {
    if (tempoDesconectado != 0) {
      Serial.println("✅ Wi-Fi reconectado!");
      tempoDesconectado = 0;
      estadoAtual = NORMAL;
    }
  }

  // ---------- Leitura dos Sensores ----------
  static unsigned long ultimoTempo = 0;     
  static unsigned long ultimoThingSpeak = 0;

  if (agora - ultimoTempo > 2000) {         // Lê sensores a cada 2 segundos
    float t = dht.readTemperature();       // Lê temperatura
    float h = dht.readHumidity();          // Lê umidade
    int ldr = analogRead(LDRPIN);          // Lê LDR

    if (!isnan(t)) temperatura = t;        // Atualiza variável global
    if (!isnan(h)) umidade = h;
    ldrValor = ldr;

    if (isnan(t) || isnan(h)) estadoAtual = ERRO_SENSOR; // Sensor com erro
    else if (WiFi.status() == WL_CONNECTED) estadoAtual = NORMAL;

    Serial.print("📊 Dados -> Temp: ");
    Serial.print(temperatura);
    Serial.print(" °C | Hum: ");
    Serial.print(umidade);
    Serial.print(" % | LDR: ");
    Serial.println(ldrValor);

    ultimoTempo = agora;
  }

  // ---------- Envio ao ThingSpeak ----------
  if (agora - ultimoThingSpeak > 15000) {   // A cada 15 segundos
    Serial.println("🚀 Enviando dados ao ThingSpeak...");
    enviarThingSpeak(temperatura, umidade, ldrValor);
    ultimoThingSpeak = agora;
  }
}

// --- DASHBOARD HTML LOCAL ---
String paginaHTML() {
  // Retorna toda a página HTML + CSS + JS da dashboard
  // Inclui gráficos, cores dinâmicas, modal de login para reinício
  String html = R"rawliteral(
<!DOCTYPE html>
<html lang="pt-BR">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>Estação Meteorológica</title>

<!-- Chart.js para gráficos -->
<script src="https://cdn.jsdelivr.net/npm/chart.js"></script>

<!-- Estilo da página -->
<style>
body { font-family:'Inter', sans-serif; text-align:center; color:#fff; transition: background 1s; margin:0; padding:0;}
h1 { margin-top:20px; margin-bottom:30px;}
.card { background: rgba(255,255,255,0.08); border-radius:15px; margin:20px auto; padding:25px; max-width:500px; box-shadow:0 8px 20px rgba(0,0,0,0.3);}
canvas { background: rgba(255,255,255,0.05); border-radius:15px; padding:15px; max-width:100%; box-shadow:0 5px 15px rgba(0,0,0,0.25);}
.valor-atual { font-size:1.8em; margin-bottom:15px; font-weight:bold;}
button { padding:12px 20px; margin:10px; font-size:16px; background:linear-gradient(90deg,#0072ff,#00c6ff); color:white; border:none; border-radius:10px; cursor:pointer; font-weight:bold; transition:0.3s ease;}
button:hover { transform:translateY(-2px); box-shadow:0 4px 15px rgba(0,198,255,0.5);}
#modalLogin { display:none; position:fixed; top:0; left:0; width:100%; height:100%; background:rgba(0,0,0,0.4); justify-content:center; align-items:center; z-index:1000; }
#modalLoginContent { background:#222; padding:20px; border-radius:10px; max-width:300px; width:90%; text-align:center; }
#modalLoginContent input { width:90%; padding:8px; margin-bottom:10px; border-radius:5px; border:none; }
#modalLoginContent button { width:45%; margin:5px; }
</style>
</head>
<body>

<h1>🌤 Estação Meteorológica</h1>
<button onclick="abrirLogin()">Reiniciar ESP</button> <!-- Botão para reinício -->

<!-- Cards com gráficos -->
<div class="card">
<h2>🌡 Temperatura</h2>
<div class="valor-atual" id="temp-atual">--</div>
<canvas id="graficoTemp"></canvas>
</div>
<div class="card">
<h2>💧 Umidade</h2>
<div class="valor-atual" id="umid-atual">--</div>
<canvas id="graficoUmid"></canvas>
</div>
<div class="card">
<h2>☀ Luminosidade</h2>
<div class="valor-atual" id="lum-atual">Média</div>
<canvas id="graficoLum"></canvas>
</div>

<!-- MODAL DE LOGIN -->
<div id="modalLogin">
  <div id="modalLoginContent">
    <h3>Login ADM</h3>
    <input id="usuario" type="text" placeholder="Usuário"><br>
    <input id="senha" type="password" placeholder="Senha"><br>
    <button onclick="enviarLogin()">Entrar</button>
    <button onclick="fecharLogin()" style="background:red;">Cancelar</button>
  </div>
</div>

<!-- ================== SCRIPT ================== -->
<script>
// ================== VARIÁVEIS DOS DADOS ==================
let tempData=[], umidData=[], lumData=[], labels=[];

// ================== CONTEXTOS DOS GRÁFICOS ==================
const ctxTemp=document.getElementById('graficoTemp').getContext('2d');
const ctxUmid=document.getElementById('graficoUmid').getContext('2d');
const ctxLum=document.getElementById('graficoLum').getContext('2d');

// ================== GRÁFICOS ==================
const graficoTemp=new Chart(ctxTemp,{
    type:'line',
    data:{labels,datasets:[{label:'Temperatura (°C)',data:tempData,borderColor:'red',backgroundColor:'rgba(255,0,0,0.2)',tension:0.4,fill:true}]},
    options:{responsive:true,animation:{duration:1000,easing:'easeOutQuart'}}
});
const graficoUmid=new Chart(ctxUmid,{
    type:'line',
    data:{labels,datasets:[{label:'Umidade (%)',data:umidData,borderColor:'blue',backgroundColor:'rgba(0,0,255,0.2)',tension:0.4,fill:true}]},
    options:{responsive:true,scales:{y:{beginAtZero:true,max:100}},animation:{duration:1000,easing:'easeOutQuart'}}
});
const graficoLum=new Chart(ctxLum,{
    type:'bar',
    data:{labels,datasets:[{label:'Luminosidade',data:lumData,backgroundColor:'yellow'}]},
    options:{responsive:true,scales:{y:{beginAtZero:true,max:3,ticks:{stepSize:1,callback:v=>['Baixa','Média','Alta'][v]}}},animation:{duration:1000,easing:'easeOutQuart'}}
});

// ================== FUNÇÃO PARA ATUALIZAR O FUNDO ==================
function atualizarFundo(temp, lum){
    let corTemp = temp < 20 ? "#1e3c72" : temp <= 26 ? "#ffae42" : "#ff0000"; // azul/médio/vermelho
    let corLum = lum === 0 ? "#2a5298" : lum === 1 ? "#00c6ff" : "#dd2476";     // azul claro/ciano/rosa
    document.body.style.background = `linear-gradient(135deg, ${corTemp}, ${corLum})`;
}

// ================== FUNÇÃO PARA ATUALIZAR DADOS ==================
function atualizarDados(){
    fetch('/data').then(r=>r.json()).then(d=>{
        const agora=new Date().toLocaleTimeString();
        if(labels.length>=20){labels.shift(); tempData.shift(); umidData.shift(); lumData.shift();} // mantém 20 pontos
        labels.push(agora);
        tempData.push(d.temperatura);
        umidData.push(d.umidade);
        let lum= d.luminosidade==="Baixa"?0:d.luminosidade==="Alta"?2:1;
        lumData.push(lum);
        document.getElementById('temp-atual').innerText=d.temperatura.toFixed(1)+" °C";
        document.getElementById('umid-atual').innerText=d.umidade.toFixed(1)+" %";
        document.getElementById('lum-atual').innerText=d.luminosidade;
        atualizarFundo(d.temperatura, lum);
        graficoTemp.update(); graficoUmid.update(); graficoLum.update();
    }).catch(err=>console.error(err));
}
setInterval(atualizarDados,2000); // Atualiza dados a cada 2 segundos

// ================== FUNÇÕES DO MODAL ==================
function abrirLogin(){document.getElementById('modalLogin').style.display='flex';document.body.style.overflow='hidden';}
function fecharLogin(){document.getElementById('modalLogin').style.display='none';document.body.style.overflow='auto';}
function enviarLogin(){
    const usuario=document.getElementById('usuario').value;
    const senha=document.getElementById('senha').value;
    if(!usuario||!senha){alert("Usuário e senha são obrigatórios!");return;}
    fetch('/reset',{headers:{'Authorization':'Basic '+btoa(usuario+":"+senha)}})
    .then(resp=>{if(resp.status===200) return resp.text(); else throw new Error("Não autorizado!");})
    .then(msg=>{alert(msg);fecharLogin();})
    .catch(err=>alert("Erro: "+err));
}
</script>
</body>
</html>
)rawliteral";
  return html; // Retorna toda a página HTML como string
}
