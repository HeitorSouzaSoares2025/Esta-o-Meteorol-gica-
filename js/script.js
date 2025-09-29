// ================== VARIÁVEIS DOS DADOS ==================

// Arrays para armazenar os valores históricos que serão exibidos nos gráficos
let tempData = [], umidData = [], lumData = [], labels = [];

// ================== CONTEXTOS DOS GRÁFICOS ==================
// Pega o elemento <canvas> de cada gráfico e o prepara para uso com Chart.js
const ctxTemp = document.getElementById('graficoTemp').getContext('2d');
const ctxUmid = document.getElementById('graficoUmid').getContext('2d');
const ctxLum = document.getElementById('graficoLum').getContext('2d');

// ================== GRÁFICO DE TEMPERATURA ==================
const graficoTemp = new Chart(ctxTemp, {
    type: 'line', // Tipo de gráfico: linha
    data: {
        labels,    // Eixo X (tempo)
        datasets: [{
            label: 'Temperatura (°C)',
            data: tempData,                              // Valores de temperatura
            borderColor: 'red',                           // Cor da linha
            backgroundColor: 'rgba(255,0,0,0.2)',         // Cor do preenchimento
            tension: 0.4,                                 // Suaviza a linha
            fill: true                                    // Preenche a área abaixo da linha
        }]
    },
    options: {
        responsive: true,                                 // Ajusta automaticamente ao tamanho da tela
        animation: { duration: 1000, easing: 'easeOutQuart' } // Animação suave ao atualizar
    }
});

// ================== GRÁFICO DE UMIDADE ==================
const graficoUmid = new Chart(ctxUmid, {
    type: 'line', // Linha também
    data: {
        labels,
        datasets: [{
            label: 'Umidade (%)',
            data: umidData,                                // Valores de umidade
            borderColor: 'blue',                           // Cor da linha
            backgroundColor: 'rgba(0,0,255,0.2)',          // Cor do preenchimento
            tension: 0.4,
            fill: true
        }]
    },
    options: {
        responsive: true,
        scales: { y: { beginAtZero: true, max: 100 } },     // Eixo Y de 0 a 100%
        animation: { duration: 1000, easing: 'easeOutQuart' }
    }
});

// ================== GRÁFICO DE LUMINOSIDADE ==================
const graficoLum = new Chart(ctxLum, {
    type: 'bar', // Barras
    data: {
        labels,
        datasets: [{
            label: 'Luminosidade',
            data: lumData,                                  // Valores de luminosidade (0,1,2)
            backgroundColor: 'yellow'                       // Cor das barras
        }]
    },
    options: {
        responsive: true,
        scales: {
            y: {
                beginAtZero: true,
                max: 3,                                      // 0 = Baixa, 1 = Média, 2 = Alta
                ticks: {
                    stepSize: 1,                              // Intervalo entre níveis
                    callback: v => ['Baixa','Média','Alta'][v] // Mostra nomes ao invés de números
                }
            }
        },
        animation: { duration: 1000, easing: 'easeOutQuart' }
    }
});

// ================== FUNÇÃO PARA ATUALIZAR O FUNDO ==================
function atualizarFundo(temp, lum) {
    // Cor baseada na temperatura
    let corTemp = temp < 20 ? "#1e3c72" : temp <= 26 ? "#ffae42" : "#ff0000";
    // Cor baseada na luminosidade
    let corLum = lum === 0 ? "#2a5298" : lum === 1 ? "#00c6ff" : "#dd2476";

    // Aplica gradiente linear ao fundo do body
    document.body.style.background = `linear-gradient(135deg, ${corTemp}, ${corLum})`;
}

// ================== FUNÇÃO PARA ATUALIZAR DADOS AO VIVO ==================
function atualizarDados() {
    // Busca dados do PHP que recebe as medições
    fetch('php/recebe_dados.php')
        .then(r => r.json())
        .then(d => {
            const agora = new Date().toLocaleTimeString();       // Hora atual
            let temp = d.temperatura[d.temperatura.length - 1];  // Última temperatura recebida
            let hum = d.umidade[d.umidade.length - 1];           // Última umidade
            let lum = d.luminosidade[d.luminosidade.length - 1]; // Última luminosidade

            // Mantém no máximo 20 pontos nos gráficos (remoção do mais antigo)
            if (labels.length >= 20) {
                labels.shift();
                tempData.shift();
                umidData.shift();
                lumData.shift();
            }

            // Adiciona nova leitura
            labels.push(agora);
            tempData.push(temp);
            umidData.push(hum);
            lumData.push(lum);

            // Atualiza valores atuais na interface
            document.getElementById('temp-atual').innerText = temp.toFixed(1) + " °C";
            document.getElementById('umid-atual').innerText = hum.toFixed(1) + " %";
            document.getElementById('lum-atual').innerText = ["Baixa", "Média", "Alta"][lum];

            // Atualiza fundo da página com base nos dados
            atualizarFundo(temp, lum);

            // Re-renderiza os gráficos
            graficoTemp.update();
            graficoUmid.update();
            graficoLum.update();
        })
        .catch(err => console.error("Erro ao buscar dados:", err)); // Mostra erro no console
}

// Atualiza automaticamente a cada 15 segundos
setInterval(atualizarDados, 15000);

// ================== FUNÇÃO PARA CONSULTAR HISTÓRICO ==================
function consultarHistorico() {
    const dataHora = document.getElementById('dataHoraInput').value;

    // Se não foi selecionada data/hora
    if (!dataHora) {
        document.getElementById('resultado').innerText = "Selecione uma data/hora.";
        return;
    }

    // Faz requisição com data/hora escolhida
    fetch(`php/historico.php?data_hora=${encodeURIComponent(dataHora)}`)
        .then(r => r.json())
        .then(d => {
            if (d.status === "ok") {
                let ultimo = d.dados[d.dados.length - 1];
                // Mostra a última leitura do histórico
                document.getElementById('resultado').innerText =
                    `Temp: ${ultimo.temperatura.toFixed(1)} °C | Hum: ${ultimo.umidade.toFixed(1)} % | Lum: ${["Baixa","Média","Alta"][ultimo.luminosidade]}`;
            } else {
                // Mostra mensagem de erro (por ex.: sem dados no horário)
                document.getElementById('resultado').innerText = d.mensagem;
            }
        })
        .catch(err => document.getElementById('resultado').innerText = "Erro ao consultar histórico.");
}

// ================== SERVICE WORKER PARA PWA ==================
if ('serviceWorker' in navigator) {              // Verifica se o navegador suporta PWA
    window.addEventListener('load', () => {      // Quando a página terminar de carregar
        navigator.serviceWorker.register('sw.js') // Registra o Service Worker
            .then(reg => console.log("SW registrado:", reg.scope))
            .catch(err => console.log("Erro ao registrar SW:", err));
    });
}
