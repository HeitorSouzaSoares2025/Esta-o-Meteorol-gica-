<?php
// ======== INCLUSÃO DA CONEXÃO COM O BANCO ========

// Inclui o arquivo responsável por conectar ao banco de dados
include "conexao.php";


// ======== BUSCAR O ÚLTIMO REGISTRO ========

// Monta a consulta SQL para obter o registro mais recente (maior ID)
$sql = "SELECT * FROM dados ORDER BY id DESC LIMIT 1";

// Executa a consulta no banco
$result = $conexao->query($sql);


// ======== SE EXISTIR DADOS NO BANCO ========
if($result && $dado = $result->fetch_assoc()){
    // Começa a montar o texto do e-mail
    $mensagem = "Relatório Estação Meteorológica\n\n";

    // Concatena as informações do registro obtido
    $mensagem .= "Temperatura: ".$dado['temperatura']." °C\n";
    $mensagem .= "Umidade: ".$dado['umidade']." %\n";
    $mensagem .= "Luminosidade: ".$dado['luminosidade']."\n";
    $mensagem .= "Data/Hora: ".$dado['data_hora']."\n";

    // Define o endereço de destino do e-mail
    $destino = "";

    // Define o assunto do e-mail
    $assunto = "Relatório Estação Meteorológica";


    // ======== ENVIO DO E-MAIL ========
    // Usa a função mail() do PHP para enviar a mensagem
    if(mail($destino, $assunto, $mensagem)){
        // Se o e-mail foi enviado com sucesso
        echo json_encode([
            "status" => "ok",
            "mensagem" => "E-mail enviado!"
        ]);
    } else {
        // Se houve falha ao enviar o e-mail
        echo json_encode([
            "status" => "erro",
            "mensagem" => "Falha ao enviar e-mail"
        ]);
    }
} else {
    // Se não encontrou nenhum dado no banco
    echo json_encode([
        "status" => "erro",
        "mensagem" => "Nenhum dado disponível para enviar"
    ]);
}


// ======== FECHAMENTO DA CONEXÃO ========

// Fecha a conexão com o banco para liberar recursos
$conexao->close();
?>
