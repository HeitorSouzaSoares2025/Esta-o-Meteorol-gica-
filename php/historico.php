<?php
// ======== CABEÇALHO JSON ========
// Define que a resposta deste script será no formato JSON
header('Content-Type: application/json');

// Inclui a conexão com o banco de dados
include 'conexao.php';

// Garante que a conexão com o banco use UTF-8 (corrige problemas de acentuação)
$conexao->set_charset("utf8");

// Ajusta o fuso horário do servidor para GMT-3 (horário de Brasília)
$conexao->query("SET time_zone='-3:00'");


// ============================
// 1. SE FOI ENVIADA UMA DATA/HORA
// ============================
if(isset($_GET['data_hora'])){
    // O valor enviado vem no formato do input datetime-local (ex: "2025-09-28T15:00")
    // Substituímos o "T" por espaço para coincidir com o formato DATETIME do MySQL
    // Acrescentamos "%" para permitir busca por registros daquele horário específico
    $dataHora = str_replace("T", " ", $_GET['data_hora']) . '%';

    // Prepara a consulta SQL para buscar dados que comecem com a data/hora informada
    $stmt = $conexao->prepare(
        "SELECT temperatura, umidade, luminosidade, data_hora 
         FROM dados 
         WHERE data_hora LIKE ? 
         ORDER BY data_hora ASC"
    );

    // Associa o parâmetro recebido à consulta preparada
    $stmt->bind_param("s", $dataHora);

    // Executa a consulta
    $stmt->execute();

    // Obtém o resultado da consulta
    $resultado = $stmt->get_result();

    // Array que vai armazenar os registros encontrados
    $dados = [];

    // Percorre cada linha retornada e adiciona no array
    while($linha = $resultado->fetch_assoc()){
        $dados[] = [
            "data_hora" => $linha['data_hora'],
            "temperatura" => (float)$linha['temperatura'],
            "umidade" => (float)$linha['umidade'],
            "luminosidade" => (int)$linha['luminosidade']
        ];
    }

    // Se encontrou registros, retorna com status "ok"
    if(count($dados) > 0){
        echo json_encode(["status"=>"ok","dados"=>$dados]);
    } 
    // Caso contrário, retorna mensagem de erro
    else {
        echo json_encode(["status"=>"erro","mensagem"=>"Nenhum dado encontrado para essa data/hora."]);
    }

    // Fecha o statement
    $stmt->close();
} 

// ============================
// 2. SE NENHUMA DATA/HORA FOI INFORMADA
// ============================
else {
    // Busca o registro mais recente (último dado registrado)
    $sql = "SELECT temperatura, umidade, luminosidade, data_hora 
            FROM dados 
            ORDER BY id DESC LIMIT 1";

    $result = $conexao->query($sql);

    // Se encontrou pelo menos um registro
    if($result && $dado = $result->fetch_assoc()){
        echo json_encode([
            "status"=>"ok",
            "dados"=>[[
                "data_hora" => $dado['data_hora'],
                "temperatura" => (float)$dado['temperatura'],
                "umidade" => (float)$dado['umidade'],
                "luminosidade" => (int)$dado['luminosidade']
            ]]
        ]);
    } 
    // Se não encontrou nada
    else {
        echo json_encode(["status"=>"erro","mensagem"=>"Nenhum dado encontrado."]);
    }
}

// Fecha a conexão com o banco
$conexao->close();
?>
