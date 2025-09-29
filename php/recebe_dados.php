<?php
header('Content-Type: application/json');
include 'conexao.php';
$conexao->set_charset("utf8");

// Define fuso horário
$conexao->query("SET time_zone='-3:00'");

// ===== 1. Busca dados no ThingSpeak =====
$channelID = "3073198";
$readAPIKey = "5VWJU7MLPJND89Q9";
$thingspeakURL = "https://api.thingspeak.com/channels/$channelID/feeds.json?api_key=$readAPIKey&results=1";

$dadosJSON = file_get_contents($thingspeakURL);
if(!$dadosJSON){
    echo json_encode(["status"=>"erro","mensagem"=>"Não foi possível acessar o ThingSpeak"]);
    exit;
}

$dados = json_decode($dadosJSON,true);
if(!$dados){
    echo json_encode(["status"=>"erro","mensagem"=>"Falha ao decodificar JSON"]);
    exit;
}

// Último feed
$ultimo = end($dados['feeds']);
$temperatura = isset($ultimo['field1']) ? floatval($ultimo['field1']) : null;
$umidade = isset($ultimo['field2']) ? floatval($ultimo['field2']) : null;
$lum = isset($ultimo['field3']) ? intval($ultimo['field3']) : null;

if($temperatura===null || $umidade===null || $lum===null){
    echo json_encode(["status"=>"erro","mensagem"=>"Dados incompletos"]);
    exit;
}

// ===== 2. Salva no banco =====
$stmt = $conexao->prepare("INSERT INTO dados (temperatura, umidade, luminosidade, data_hora) VALUES (?,?,?,NOW())");
$stmt->bind_param("ddi", $temperatura, $umidade, $lum);

if(!$stmt->execute()){
    echo json_encode(["status"=>"erro","mensagem"=>"Falha ao salvar no banco: " . $stmt->error]);
    $stmt->close();
    exit;
}
$stmt->close();

// ===== 2.1 Mantém apenas os 525 registros mais recentes (fila) =====
$LIMITE_MAXIMO = 525;
$conexao->query("
    DELETE FROM dados
    WHERE id NOT IN (
        SELECT id FROM (
            SELECT id FROM dados ORDER BY id DESC LIMIT $LIMITE_MAXIMO
        ) AS t
    )
");

// ===== 3. Retorna últimos 20 registros =====
$result = $conexao->query("SELECT temperatura, umidade, luminosidade, data_hora FROM dados ORDER BY id DESC LIMIT 20");

$temps = [];
$umids = [];
$lums = [];
$horarios = [];

if($result && $result->num_rows > 0){
    while($d = $result->fetch_assoc()){
        $temps[] = floatval($d['temperatura']);
        $umids[] = floatval($d['umidade']);
        $lums[] = intval($d['luminosidade']);
        $horarios[] = $d['data_hora'];
    }
}

echo json_encode([
    "status"=>"ok",
    "temperatura"=>array_reverse($temps),
    "umidade"=>array_reverse($umids),
    "luminosidade"=>array_reverse($lums),
    "data_hora"=>array_reverse($horarios)
]);

$conexao->close();
?>
