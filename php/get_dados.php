<?php
// ======== CABEÇALHO JSON ========
// Define que a resposta deste script será no formato JSON
header('Content-Type: application/json');

// Inclui o arquivo de conexão com o banco
include 'conexao.php';

// Define que a conexão usará UTF-8 (evita problemas com acentos)
$conexao->set_charset("utf8");

// Define o fuso horário do servidor (Brasília, GMT-3)
$conexao->query("SET time_zone='-3:00'");


// ============================
// 1. BUSCAR DADOS DO THINGSPEAK
// ============================

// ID do canal no ThingSpeak
$channelID = "";

// Chave de leitura (Read API Key) para acessar os dados do canal
$readAPIKey = "";

// URL da API do ThingSpeak para buscar os dados mais recentes
$thingspeakURL = "https://api.thingspeak.com/channels/$channelID/feeds.json?api_key=$readAPIKey&results=1";

// Faz a requisição HTTP GET para obter o JSON
$dadosJSON = file_get_contents($thingspeakURL);

// Se a requisição falhar, retorna erro e interrompe
if(!$dadosJSON){
    echo json_encode(["status"=>"erro","mensagem"=>"Não foi possível acessar o ThingSpeak"]);
    exit;
}

// Decodifica o JSON recebido em um array associativo PHP
$dados = json_decode($dadosJSON,true);

// Se não conseguir decodificar, retorna erro e interrompe
if(!$dados){
    echo json_encode(["status"=>"erro","mensagem"=>"Falha ao decodificar JSON"]);
    exit;
}


// ============================
// 2. PROCESSAR O ÚLTIMO REGISTRO
// ============================

// Obtém o último feed (registro mais recente do ThingSpeak)
$ultimo = end($dados['feeds']);

// Extrai os valores dos campos field1, field2 e field3
$temperatura = isset($ultimo['field1']) ? floatval($ultimo['field1']) : null;
$umidade     = isset($ultimo['field2']) ? floatval($ultimo['field2']) : null;
$lum         = isset($ultimo['field3']) ? intval($ultimo['field3'])   : null;

// Se algum dado estiver ausente, retorna erro e interrompe
if($temperatura===null || $umidade===null || $lum===null){
    echo json_encode(["status"=>"erro","mensagem"=>"Dados incompletos"]);
    exit;
}


// ============================
// 3. SALVAR NO BANCO DE DADOS
// ============================

// Prepara a consulta SQL para inserir os dados no banco
$stmt = $conexao->prepare(
    "INSERT INTO dados (temperatura, umidade, luminosidade, data_hora) VALUES (?,?,?,NOW())"
);

// Associa os valores obtidos às variáveis do SQL (d = double, i = int)
$stmt->bind_param("ddi", $temperatura, $umidade, $lum);

// Executa a inserção e verifica se deu certo
if(!$stmt->execute()){
    // Se der erro, retorna mensagem de falha e interrompe
    echo json_encode(["status"=>"erro","mensagem"=>"Falha ao salvar no banco: " . $stmt->error]);
    $stmt->close();
    exit;
}

// Fecha o statement (libera recursos)
$stmt->close();


// ============================
// 4. BUSCAR OS ÚLTIMOS 20 REGISTROS
// ============================

// Consulta os 20 registros mais recentes, ordenados do mais novo para o mais antigo
$result = $conexao->query(
    "SELECT temperatura, umidade, luminosidade, data_hora FROM dados ORDER BY id DESC LIMIT 20"
);

// Arrays para armazenar os dados
$temps = [];
$umids = [];
$lums = [];
$horarios = [];

// Se houver registros, percorre linha por linha
if($result && $result->num_rows > 0){
    while($d = $result->fetch_assoc()){
        $temps[]    = floatval($d['temperatura']);
        $umids[]    = floatval($d['umidade']);
        $lums[]     = intval($d['luminosidade']);
        $horarios[] = $d['data_hora'];
    }
}


// ============================
// 5. RESPONDER EM FORMATO JSON
// ============================

// Retorna os dados em ordem cronológica correta (do mais antigo para o mais novo)
echo json_encode([
    "status"        => "ok",
    "temperatura"   => array_reverse($temps),
    "umidade"       => array_reverse($umids),
    "luminosidade"  => array_reverse($lums),
    "data_hora"     => array_reverse($horarios)
]);

// Fecha a conexão com o banco
$conexao->close();
?>
