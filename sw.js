// Nome do cache atual — altere a versão (v1, v2, etc.) para forçar atualização
const CACHE_NAME = "estacao-cache-v1";

// Lista de arquivos a serem armazenados no cache para uso offline
const FILES_TO_CACHE = [
  "/",                      // Raiz do site
  "index.html",             // Página principal
  "style.css",              // Estilos
  "script.js",              // Lógica do front-end
  "chart.umd.min.js",       // Biblioteca de gráficos (Chart.js)
  "manifest.json",          // Configuração do PWA
  "icons/icon-192.png",     // Ícone 192x192
  "icons/icon-512.png"      // Ícone 512x512
];

/* --------------------------
   EVENTO: install
   - Executado quando o service worker é instalado
   - Armazena em cache todos os arquivos essenciais para uso offline
-------------------------- */
self.addEventListener("install", e => {
  e.waitUntil(
    caches.open(CACHE_NAME)          // Abre (ou cria) um cache com o nome especificado
      .then(cache => cache.addAll(FILES_TO_CACHE)) // Armazena todos os arquivos da lista
  );
  self.skipWaiting(); // Faz com que este service worker ative imediatamente sem esperar
});

/* --------------------------
   EVENTO: activate
   - Executado quando o service worker é ativado
   - Remove caches antigos (de versões anteriores)
-------------------------- */
self.addEventListener("activate", e => {
  e.waitUntil(
    caches.keys() // Obtém todos os caches existentes
      .then(keys =>
        Promise.all(
          keys.map(key => 
            key !== CACHE_NAME       // Se o cache não for o atual...
              ? caches.delete(key)   // ...remove para liberar espaço
              : null
          )
        )
      )
  );
  self.clients.claim(); // Assume imediatamente o controle das páginas abertas
});

/* --------------------------
   EVENTO: fetch
   - Intercepta todas as requisições da página
   - Tenta servir os arquivos a partir do cache
   - Se não encontrar, faz o fetch normal
   - Se estiver offline e for navegação (HTML), carrega a página inicial
-------------------------- */
self.addEventListener("fetch", e => {
  e.respondWith(
    caches.match(e.request)              // Procura o recurso no cache
      .then(response => 
        response ||                       // Se encontrou, usa o cache
        fetch(e.request).catch(() => {    // Se não, tenta baixar da internet
          if (e.request.mode === "navigate") {
            // Se for navegação e offline, mostra a página inicial
            return caches.match("index.html");
          }
        })
      )
  );
});

/* --------------------------
   EVENTO: message
   - Permite que a página envie mensagens ao SW
   - Se a mensagem for SKIP_WAITING, ativa a nova versão imediatamente
-------------------------- */
self.addEventListener("message", e => {
  if (e.data && e.data.type === "SKIP_WAITING") {
    self.skipWaiting(); // Atualiza o SW sem esperar os usuários fecharem as abas
  }
});
