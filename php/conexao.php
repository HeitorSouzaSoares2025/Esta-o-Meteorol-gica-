<?php
// ======== CONFIGURAÇÕES DO BANCO DE DADOS ========

// Nome do host do servidor MySQL (no caso, do InfinityFree)
$host = "sql113.infinityfree.com";

// Usuário do banco de dados (fornecido pelo InfinityFree)
$usuario = "if0_39887980";

// Senha do banco de dados
$senha = "NnHbZmfhqW";

// Nome do banco de dados onde os dados da estação estão armazenados
$banco = "if0_39887980_dadosestacao";


// ======== CONEXÃO COM O BANCO ========

// Cria uma nova conexão MySQL usando os dados informados
$conexao = new mysqli($host, $usuario, $senha, $banco);


// ======== VERIFICAÇÃO DA CONEXÃO ========

// Se ocorrer algum erro na conexão, o script é encerrado
// e retorna um JSON com a mensagem de erro
if ($conexao->connect_error) {
    die(json_encode([
        "status" => "erro",
        "mensagem" => "Falha na conexão: " . $conexao->connect_error
    ]));
}
?>
