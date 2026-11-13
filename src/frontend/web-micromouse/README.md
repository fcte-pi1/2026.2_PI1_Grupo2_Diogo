# Frontend (Vite) — Ambiente Docker

Documentação de como rodar a aplicação frontend em ambiente de desenvolvimento usando Docker e Docker Compose.

## Requisitos

- [Docker](https://www.docker.com/) instalado
- [Docker Compose](https://docs.docker.com/compose/) (já incluso no Docker Desktop)

## Estrutura de arquivos

```
.
├── Dockerfile
├── docker-compose.yml
├── package.json
└── ... (código-fonte da aplicação)
```

## Dockerfile

```dockerfile
FROM node:22-alpine

WORKDIR /app
COPY package*.json ./

RUN npm ci

COPY . .

EXPOSE 5173

CMD ["npm", "run", "dev", "--", "--host", "0.0.0.0"]
```

> ⚠️ **Importante:** o `--host 0.0.0.0` é obrigatório. Sem ele, o Vite escuta apenas em `localhost` dentro do container, e o navegador (fora do container) não consegue se conectar — resultando em `ERR_CONNECTION_REFUSED`.
>
> Alternativa: em vez de passar `--host` no `CMD`, você pode ajustar o script `dev` no `package.json`:
> ```json
> "scripts": {
>   "dev": "vite --host 0.0.0.0"
> }
> ```

## docker-compose.yml

```yaml
services:
  app:
    build:
      context: .
      dockerfile: Dockerfile
    ports:
      - "5173:5173"
    environment:
      - API_URL=http://host.docker.internal:8000
      - CHOKIDAR_USEPOLLING=true
    extra_hosts:
      - "host.docker.internal:host-gateway"
    volumes:
      - .:/app
      - /app/node_modules
    command: npm run dev
    healthcheck:
      test: ["CMD", "wget", "--quiet", "--tries=1", "--spider", "http://localhost:5173"]
      interval: 30s
      timeout: 5s
      retries: 3
      start_period: 10s
```

### O que cada parte faz

| Configuração | Função |
|---|---|
| `ports` | Mapeia a porta do container para o host (`host:container`) |
| `volumes: .:/app` | Sincroniza o código local com o container, habilitando hot-reload |
| `volumes: /app/node_modules` | Evita que o `node_modules` do host sobrescreva o instalado no container |
| `API_URL` | Endereço do backend visto de dentro do container. É usado pelo proxy do Vite (ver [Integração com o backend](#integração-com-o-backend-tela-labirinto)). |
| `extra_hosts` | Garante que o nome `host.docker.internal` (o seu PC, visto de dentro do container) também exista no Linux. No Docker Desktop (Windows/Mac) ele já vem pronto. |
| `CHOKIDAR_USEPOLLING` | Força o watcher de arquivos a checar por polling (necessário em alguns sistemas, como Windows/WSL2) |
| `healthcheck` | Verifica periodicamente se a aplicação está respondendo (não só se o container está de pé) |

## Como rodar

### Subir o ambiente

```bash
docker compose up --build
```

Depois é só acessar: **http://localhost:5173**

### Rodar em segundo plano

```bash
docker compose up -d --build
```

### Ver status (incluindo healthcheck)

```bash
docker ps
```

Na coluna `STATUS`, você verá algo como:
- `Up 30 seconds (healthy)` → aplicação respondendo normalmente
- `Up 30 seconds (unhealthy)` → aplicação subiu, mas não está respondendo nas checagens

### Ver logs

```bash
docker compose logs -f app
```

### Parar o ambiente

```bash
docker compose down
```

## Integração com o backend (tela Labirinto)

A tela **Labirinto** (`/labirinto`) mostra os dados que vêm da API do backend (`src/backend`). Como rodar o backend e quais são as rotas está no [README da API](../../backend/app/routes/README.md).

```
navegador ──► Vite no container (:5173) ──proxy /api──► host.docker.internal:8000 ──► backend no seu PC
```

### Por que um proxy

O front roda em `localhost:5173` e a API em `localhost:8000`. Para o navegador, porta diferente é site diferente, e ele bloqueia a leitura de uma resposta vinda de outro site sem permissão (CORS).

Com o proxy (`server.proxy` no `vite.config.ts`), o próprio Vite repassa para o backend tudo que começa com `/api`. Para o navegador é tudo o mesmo site, e o backend não precisa mudar nada. Em produção, front e back ficam no mesmo endereço, então o problema nem existe.

O endereço do backend vem da variável `API_URL`:

- **no Docker:** `http://host.docker.internal:8000`, definido no `docker-compose.yml`. Dentro do container, `localhost` é o próprio container, e não o seu PC;
- **rodando direto no PC** (`npm run dev`): `http://localhost:8000`, que é o valor padrão.

### Arquivos

| Arquivo | O que faz |
|---|---|
| `src/api.ts` | Uma função para cada rota (`api.sessaoAtiva()`, `api.labirinto(id)`, `api.trajetoria(id)`, `api.metricas(id)` e `api.eventos(id)`), mais os tipos das respostas. Se a API responder com erro (404, 500), a função lança um erro em vez de devolver um dado errado. |
| `src/hooks/useLabirinto.ts` | A cada 1 s (*polling*), busca a sessão ativa e, com o id dela, o labirinto, a trajetória e as métricas. Depois converte tudo para o formato da tela. Para de perguntar quando você sai da página. |
| `src/pages/LabirintoPage.tsx` | Usa o `useLabirinto()` no lugar dos dados vazios. Quando não há sessão, o tamanho da grade vem do seletor (4×4, 8×4 ou 12×4). |

### O que o hook converte

| API | Tela |
|---|---|
| `row`, `col` | `x` = coluna, `y` = linha |
| `status`: `em_execucao`, `concluido`, `interrompido` | `Em execução`, `Concluído`, `Interrompido` |
| `tempo_total_ms` | `tempoMS` (já vem em milissegundos; se vier `null`, vira 0) |
| `speed_cm_s`, `rpm`, `battery_pct` | `velocidade`, `rpm`, `bateria` |
| `size` do labirinto | `linhas` e `colunas` da grade |
| (calculada) | `direcao` do robô, comparando cada célula com a anterior |

Se a API não responder (com o backend desligado, por exemplo), a tela mostra "Sem conexão" em vez de quebrar.

### Rodando junto com o backend

1. Suba o **backend**, em `src/backend`, com um destes comandos:
   - dados parados: `.venv\Scripts\python -m uvicorn app.main:app --reload --host 0.0.0.0`
   - robô andando: `.venv\Scripts\python -m uvicorn app.mock.dev_server:app --reload --host 0.0.0.0`

   O `--host 0.0.0.0` é obrigatório. Sem ele, o backend só aceita conexões do próprio PC, e o container não consegue alcançá-lo.
2. Suba o **frontend**, nesta pasta: `docker compose up -d`.
3. Abra http://localhost:5173/labirinto.

Para conferir só o caminho até a API, abra http://localhost:5173/api/sessions/active. Deve aparecer o JSON da sessão.

## Solução de problemas

### `ERR_CONNECTION_REFUSED` ao acessar `localhost:5173`

- Confirme que o `--host 0.0.0.0` está aplicado (no `CMD` do Dockerfile ou no script `dev` do `package.json`).
- Confira os logs (`docker compose logs -f app`) — o Vite deve mostrar uma linha `Network: http://0.0.0.0:5173/` ou similar.
- Verifique se a porta mapeada no `docker-compose.yml` bate com a porta que o Vite está de fato usando.

### Healthcheck falhando (`unhealthy`)

- Confirme que o `wget` está disponível na imagem (Alpine costuma trazer por padrão). Caso não esteja, instale `curl` no Dockerfile:
  ```dockerfile
  RUN apk add --no-cache curl
  ```
  e ajuste o `test` do healthcheck para usar `curl -f` no lugar de `wget`.
- Aumente o `start_period` se a aplicação demorar mais que 10s para subir.

### Mudanças no código não aparecem (sem hot-reload)

- Confirme que o volume `.:/app` está mapeado corretamente no `docker-compose.yml`.
- Em Windows/WSL2, ative `CHOKIDAR_USEPOLLING=true` (já incluso neste setup).

### `/api/...` responde 502, ou a tela fica em "Sem conexão"

- O backend está desligado, ou foi ligado sem o `--host 0.0.0.0`. Nos logs do container (`docker compose logs -f app`) aparece `http proxy error ... ECONNREFUSED`.
- Confira se a variável chegou no container: `docker compose exec app printenv API_URL` deve mostrar `http://host.docker.internal:8000`.

### Mudei o `docker-compose.yml` e nada mudou

- O `docker compose restart` não lê variáveis novas. Use `docker compose up -d`, que recria o container com a configuração nova.

## Testes unitários do SVG

A suíte usa React Testing Library, Vitest 5 e jsdom. Use Node 22.12 ou superior (o container usa Node 22). Os testes renderizam `Percurso` com posições conhecidas e verificam o SVG e os controles; a execução é independente do backend e do banco. Também há testes do estado compartilhado e da página de telemetria.

Na pasta `src/frontend/web-micromouse`, instale as dependências e execute:

```powershell
npm ci
npm run test:unit
npm run test:coverage
```

Com o frontend já iniciado pelo Compose desta pasta, os mesmos comandos podem ser executados no container:

```powershell
docker compose exec app npm run test:unit
docker compose exec app npm run test:coverage
```

Os casos estão em `src/components/Percurso.test.tsx` e cobrem dimensões 4×4, 8×4 e 12×4, centros das células, posição e direção do robô, trajetórias vazias ou com uma posição, reprodução, pausa, retomada, seleção de passo, fim do replay, atualização do percurso e limpeza dos temporizadores. O relógio é simulado nos testes do replay para avançar os passos de 400 ms sem esperas reais.

O relatório mede o arquivo inteiro `src/components/Percurso.tsx`, incluindo os marcadores e os controles. O comando de cobertura exige pelo menos 80% de linhas, instruções, funções e ramificações desse componente. Abra `coverage/index.html` para consultar o relatório HTML; a pasta gerada está no `.gitignore`.

Para a PR, capture o terminal com o comando e o resultado e o resumo do relatório HTML. Declare que o percentual corresponde a `Percurso.tsx`.

As paredes detectadas ainda aguardam implementação no componente. O caso correspondente está registrado como `todo`; a integração entre banco, API e paredes no SVG também permanece pendente.

## Reconexão da telemetria — RNF04

O `TelemetryProvider`, montado em `App.tsx`, mantém um WebSocket para a página
e a barra lateral. A URL usa `VITE_API_URL` quando definida; caso contrário,
usa a origem do navegador e o proxy `/ws` do Vite. Conexões HTTPS usam `wss`.

Após uma queda, as novas tentativas aguardam 1, 2, 4, 8 e no máximo 10 segundos.
Cinco segundos sem mensagens também iniciam a reconexão. A última leitura fica
visível com aviso e sua idade; antes da primeira leitura, os cards exibem “—”.
O recebimento de dados válidos restaura a atualização sem recarregar a página.

Para executar o cenário em Chromium com HTTP e WebSocket simulados:

```powershell
npx.cmd playwright install chromium
npm.cmd run test:e2e
```

O Playwright inicia um servidor Vite temporário na porta 5184. O teste verifica
o início sem histórico e a retomada após queda. Os resultados ficam em
`test-results/`, pasta ignorada pelo Git.
